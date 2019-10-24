#include "pch.h"
#include "ready.h"

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

// 是否已做好准备工作
BOOL IsReady() {

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
