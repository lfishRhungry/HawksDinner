// ��һЩ׼���ԵĹ���

#pragma once
#include "pch.h"
#include <winternl.h>
#include "ToolHelp.h"

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

// ͨ���������Ʋ��ҵ�����
BOOL find_debuger_by_name(const char* processname);

// ������
BOOL IsDebug();

// �Ƿ�������׼������
BOOL IsReady();

// �޸�PEBαװ������Ϣ
BOOL DisguiseProcess(wchar_t* lpwszPath, wchar_t* lpwszCmd);

// ȷ���Ƿ���Ԥ��·��
BOOL IsDirCorrect();

// �ж��Ƿ��Թ���ԱȨ������
BOOL IsRunasAdmin();

// ��Ȩ��������
void BoostSelf();
