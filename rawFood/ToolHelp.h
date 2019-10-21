#pragma once
// Module:  Toolhelp.h
// 对toolhelp系列函数的一些封装
// 主要功能有：
// 启动或禁用当前进程的指定权限
// 读取指定进程指定内存地址的指定大小数据

// 遍历系统当前进程快照
// 通过进程快照和pid寻找指定进程的PROCESSENTRY32结构信息
#pragma once


///////////////////////////////////////////////////////////////////////////////


#include "pch.h"
#include <tlhelp32.h>
#include <tchar.h>


///////////////////////////////////////////////////////////////////////////////

// 注意：C++类中函数声明后面跟上const关键字表示
// 函数内逻辑不允许改变所属对象的成员变量的值
// 这样的函数成为常成员函数


class CToolhelp {
private:
	HANDLE m_hSnapshot;

public:
	CToolhelp(DWORD dwFlags = 0, DWORD dwProcessID = 0);
	~CToolhelp();
	// 拍摄快照
	BOOL CreateSnapshot(DWORD dwFlags, DWORD dwProcessID = 0);
	// 进程快照相关
	BOOL ProcessFirst(PPROCESSENTRY32 ppe) const;
	BOOL ProcessNext(PPROCESSENTRY32 ppe) const;
	BOOL ProcessFind(DWORD dwProcessId, PPROCESSENTRY32 ppe) const;

public:
	// 启动或禁用进程的指定权限
	static BOOL EnablePrivilege(PCTSTR szPrivilege, BOOL fEnable = TRUE);
	// 读取指定进程指定内存地址的指定大小数据
	static BOOL ReadProcessMemory(DWORD dwProcessID, LPCVOID pvBaseAddress,
		PVOID pvBuffer, SIZE_T cbRead, SIZE_T* pNumberOfBytesRead = NULL);
};

///////////////////////////////////////////////////////////////////////////////
/*由于类中的函数在使用时往往需要被频繁调用
为了减少占空间的消耗 均采用内联形式*/
///////////////////////////////////////////////////////////////////////////////


inline CToolhelp::CToolhelp(DWORD dwFlags, DWORD dwProcessID) {

	m_hSnapshot = INVALID_HANDLE_VALUE; // 先将快照句柄设为非法值
	CreateSnapshot(dwFlags, dwProcessID);
}


///////////////////////////////////////////////////////////////////////////////


inline CToolhelp::~CToolhelp() {

	if (m_hSnapshot != INVALID_HANDLE_VALUE)
		CloseHandle(m_hSnapshot); // 销毁句柄
}


///////////////////////////////////////////////////////////////////////////////


inline BOOL CToolhelp::CreateSnapshot(DWORD dwFlags, DWORD dwProcessID) {

	if (m_hSnapshot != INVALID_HANDLE_VALUE) // 确保不要保留任何内核对象句柄
		CloseHandle(m_hSnapshot);

	if (dwFlags == 0) {
		m_hSnapshot = INVALID_HANDLE_VALUE; // 没有传入合法flag值
	}
	else {
		m_hSnapshot = CreateToolhelp32Snapshot(dwFlags, dwProcessID); // 根据要求拍摄快照
	}
	return(m_hSnapshot != INVALID_HANDLE_VALUE);
}


///////////////////////////////////////////////////////////////////////////////

// 启动或禁用进程的指定权限
inline BOOL CToolhelp::EnablePrivilege(PCTSTR szPrivilege, BOOL fEnable) {

	// 通过提升调试权限 使程序可以查看服务程序的信息
	BOOL fOk = FALSE;    // 假设权限提升失败
	HANDLE hToken;

	// 打开所属进程的令牌 指定我们的打开权限为 可以调整指定令牌权限 的权限
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {

		// 创建一个TOKEN_PRIVILEGES结构
		// 设置好要修改的信息
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		// 拿到需要调整的权限的LUID
		LookupPrivilegeValue(NULL, szPrivilege, &tp.Privileges[0].Luid);
		tp.Privileges[0].Attributes = fEnable ? SE_PRIVILEGE_ENABLED : 0; // 区分指定权限的开启和禁用
		// 传入结构 修改令牌权限
		AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
		fOk = (GetLastError() == ERROR_SUCCESS);

		// 关闭句柄
		CloseHandle(hToken);
	}
	return(fOk);
}


///////////////////////////////////////////////////////////////////////////////

// 读取指定进程指定内存地址的指定大小数据
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
		fOk = ProcessNext(ppe); // 跳过pid为0的系统空闲进程（system idle process）
	return(fOk);
}


inline BOOL CToolhelp::ProcessNext(PPROCESSENTRY32 ppe) const {

	BOOL fOk = Process32Next(m_hSnapshot, ppe);
	if (fOk && (ppe->th32ProcessID == 0))
		fOk = ProcessNext(ppe); // 跳过pid为0的系统空闲进程（system idle process）
	return(fOk);
}


inline BOOL CToolhelp::ProcessFind(DWORD dwProcessId, PPROCESSENTRY32 ppe)
const {

	BOOL fFound = FALSE;
	// 巧用for循环 通过pid寻找指定进程PROCESSENTRY32结构
	for (BOOL fOk = ProcessFirst(ppe); fOk; fOk = ProcessNext(ppe)) {
		fFound = (ppe->th32ProcessID == dwProcessId);
		if (fFound) break;
	}
	return(fFound);
}


///////////////////////////////////////////////////////////////////////////////
