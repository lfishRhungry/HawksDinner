// ��һЩ׼���ԵĹ���

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

// ͨ����������������֤�Ƿ��ǵ�һʵ��
BOOL IsAlreadyExiting();

// �Ƿ�������׼������
BOOL IsReady();

// �޸�PEBαװ������Ϣ
BOOL DisguiseProcess(wchar_t* lpwszPath, wchar_t* lpwszCmd);
