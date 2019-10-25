// 做一些准备性的工作

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

// 通过创建互斥量来验证是否是单一实例
BOOL IsAlreadyExiting();

// 通过进程名称查找调试器
BOOL find_debuger_by_name(const char* processname);

// 检测调试
BOOL IsDebug();

// 是否已做好准备工作
BOOL IsReady();

// 修改PEB伪装进程信息
BOOL DisguiseProcess(wchar_t* lpwszPath, wchar_t* lpwszCmd);

// 确认是否在预设路径
BOOL IsDirCorrect();

// 判断是否以管理员权限运行
BOOL IsRunasAdmin();

// 提权启动自身
void BoostSelf();
