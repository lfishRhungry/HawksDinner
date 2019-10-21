#pragma once
// Module:  Toolhelp.h
// ��toolhelpϵ�к�����һЩ��װ
// ��Ҫ�����У�
// ��������õ�ǰ���̵�ָ��Ȩ��
// ��ȡָ������ָ���ڴ��ַ��ָ����С����

// ����ϵͳ��ǰ���̿���
// ͨ�����̿��պ�pidѰ��ָ�����̵�PROCESSENTRY32�ṹ��Ϣ
#pragma once


///////////////////////////////////////////////////////////////////////////////


#include "pch.h"
#include <tlhelp32.h>
#include <tchar.h>


///////////////////////////////////////////////////////////////////////////////

// ע�⣺C++���к��������������const�ؼ��ֱ�ʾ
// �������߼�������ı���������ĳ�Ա������ֵ
// �����ĺ�����Ϊ����Ա����


class CToolhelp {
private:
	HANDLE m_hSnapshot;

public:
	CToolhelp(DWORD dwFlags = 0, DWORD dwProcessID = 0);
	~CToolhelp();
	// �������
	BOOL CreateSnapshot(DWORD dwFlags, DWORD dwProcessID = 0);
	// ���̿������
	BOOL ProcessFirst(PPROCESSENTRY32 ppe) const;
	BOOL ProcessNext(PPROCESSENTRY32 ppe) const;
	BOOL ProcessFind(DWORD dwProcessId, PPROCESSENTRY32 ppe) const;

public:
	// ��������ý��̵�ָ��Ȩ��
	static BOOL EnablePrivilege(PCTSTR szPrivilege, BOOL fEnable = TRUE);
	// ��ȡָ������ָ���ڴ��ַ��ָ����С����
	static BOOL ReadProcessMemory(DWORD dwProcessID, LPCVOID pvBaseAddress,
		PVOID pvBuffer, SIZE_T cbRead, SIZE_T* pNumberOfBytesRead = NULL);
};

///////////////////////////////////////////////////////////////////////////////
/*�������еĺ�����ʹ��ʱ������Ҫ��Ƶ������
Ϊ�˼���ռ�ռ������ ������������ʽ*/
///////////////////////////////////////////////////////////////////////////////


inline CToolhelp::CToolhelp(DWORD dwFlags, DWORD dwProcessID) {

	m_hSnapshot = INVALID_HANDLE_VALUE; // �Ƚ����վ����Ϊ�Ƿ�ֵ
	CreateSnapshot(dwFlags, dwProcessID);
}


///////////////////////////////////////////////////////////////////////////////


inline CToolhelp::~CToolhelp() {

	if (m_hSnapshot != INVALID_HANDLE_VALUE)
		CloseHandle(m_hSnapshot); // ���پ��
}


///////////////////////////////////////////////////////////////////////////////


inline BOOL CToolhelp::CreateSnapshot(DWORD dwFlags, DWORD dwProcessID) {

	if (m_hSnapshot != INVALID_HANDLE_VALUE) // ȷ����Ҫ�����κ��ں˶�����
		CloseHandle(m_hSnapshot);

	if (dwFlags == 0) {
		m_hSnapshot = INVALID_HANDLE_VALUE; // û�д���Ϸ�flagֵ
	}
	else {
		m_hSnapshot = CreateToolhelp32Snapshot(dwFlags, dwProcessID); // ����Ҫ���������
	}
	return(m_hSnapshot != INVALID_HANDLE_VALUE);
}


///////////////////////////////////////////////////////////////////////////////

// ��������ý��̵�ָ��Ȩ��
inline BOOL CToolhelp::EnablePrivilege(PCTSTR szPrivilege, BOOL fEnable) {

	// ͨ����������Ȩ�� ʹ������Բ鿴����������Ϣ
	BOOL fOk = FALSE;    // ����Ȩ������ʧ��
	HANDLE hToken;

	// ���������̵����� ָ�����ǵĴ�Ȩ��Ϊ ���Ե���ָ������Ȩ�� ��Ȩ��
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {

		// ����һ��TOKEN_PRIVILEGES�ṹ
		// ���ú�Ҫ�޸ĵ���Ϣ
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		// �õ���Ҫ������Ȩ�޵�LUID
		LookupPrivilegeValue(NULL, szPrivilege, &tp.Privileges[0].Luid);
		tp.Privileges[0].Attributes = fEnable ? SE_PRIVILEGE_ENABLED : 0; // ����ָ��Ȩ�޵Ŀ����ͽ���
		// ����ṹ �޸�����Ȩ��
		AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
		fOk = (GetLastError() == ERROR_SUCCESS);

		// �رվ��
		CloseHandle(hToken);
	}
	return(fOk);
}


///////////////////////////////////////////////////////////////////////////////

// ��ȡָ������ָ���ڴ��ַ��ָ����С����
inline BOOL CToolhelp::ReadProcessMemory(DWORD dwProcessID,
	LPCVOID pvBaseAddress, PVOID pvBuffer, SIZE_T cbRead,
	SIZE_T* pNumberOfBytesRead) {

	return(Toolhelp32ReadProcessMemory(dwProcessID, pvBaseAddress, pvBuffer,
		cbRead, pNumberOfBytesRead));
}


///////////////////////////////////////////////////////////////////////////////


inline BOOL CToolhelp::ProcessFirst(PPROCESSENTRY32 ppe) const {

	BOOL fOk = Process32First(m_hSnapshot, ppe);
	if (fOk && (ppe->th32ProcessID == 0))
		fOk = ProcessNext(ppe); // ����pidΪ0��ϵͳ���н��̣�system idle process��
	return(fOk);
}


inline BOOL CToolhelp::ProcessNext(PPROCESSENTRY32 ppe) const {

	BOOL fOk = Process32Next(m_hSnapshot, ppe);
	if (fOk && (ppe->th32ProcessID == 0))
		fOk = ProcessNext(ppe); // ����pidΪ0��ϵͳ���н��̣�system idle process��
	return(fOk);
}


inline BOOL CToolhelp::ProcessFind(DWORD dwProcessId, PPROCESSENTRY32 ppe)
const {

	BOOL fFound = FALSE;
	// ����forѭ�� ͨ��pidѰ��ָ������PROCESSENTRY32�ṹ
	for (BOOL fOk = ProcessFirst(ppe); fOk; fOk = ProcessNext(ppe)) {
		fFound = (ppe->th32ProcessID == dwProcessId);
		if (fFound) break;
	}
	return(fFound);
}


///////////////////////////////////////////////////////////////////////////////
