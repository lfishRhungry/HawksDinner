// 做一些准备性的工作

#pragma once
#include "pch.h"
#include <winternl.h>

// prepare for call NtQueryInformationProcess func
typedef NTSTATUS(NTAPI* typedef_NtQueryInformationProcess)(
	IN HANDLE ProcessHandle,
	IN PROCESSINFOCLASS ProcessInformationClass,
	OUT PVOID ProcessInformation,
	IN ULONG ProcessInformationLength,
	OUT PULONG ReturnLength OPTIONAL
	);

// 通过创建互斥量来验证是否是单一实例
BOOL IsAlreadyExiting();

// 是否已做好准备工作
BOOL IsReady();

// 修改PEB伪装进程信息
BOOL DisguiseProcess(wchar_t* lpwszPath, wchar_t* lpwszCmd);
