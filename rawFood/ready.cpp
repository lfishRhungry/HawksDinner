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