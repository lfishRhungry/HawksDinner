#include "ready.h"
#include "pch.h"

BOOL IsAlreadyExiting() {

	HANDLE hMutex = NULL;
	hMutex = ::CreateMutexA(NULL, FALSE, "HawkNeedsToEatDinner");
	// �����������ɹ��������Ѵ��ڻ�����������ʱ��������򲢷ǵ�һʵ��
	if (hMutex) {
		if (ERROR_ALREADY_EXISTS == ::GetLastError()) {
			return TRUE;
		}
	}
	return FALSE;
}

// �Ƿ�������׼������
BOOL IsReady() {

	if (IsAlreadyExiting()) {
		return FALSE;
	}

	return TRUE;
}
