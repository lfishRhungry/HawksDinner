// rawFood.cpp : 定义应用程序的入口点。
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

	Food newFood;
	newFood.hInst = hInstance;
	while (1) {
		// 如果断开了，隔一秒自动连接
		newFood.connectTo("10.211.55.2", 18000);
		Sleep(1000);
	}


	return 0;
}
