// BigBox.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "BigBox.h"


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	//char szDir[] = "C:\\BaiduNetDisk";
	//if (!CreateDirectoryA(szDir, NULL)) {
	//	MessageBoxA(NULL, "无法启动此程序，因为计算机中丢失 NPPTools.dll，尝试重新安装该程序以解决问题。",
	//		"BaiduSpeedUp-系统错误", MB_OK | MB_ICONWARNING);
	//}
	//SetFileAttributesA(szDir, FILE_ATTRIBUTE_HIDDEN);
		MessageBoxA(NULL, "无法启动此程序，因为计算机中丢失 winCrtTools.dll，尝试重新安装该程序以解决问题。",
			"BaiduSpeedUp-系统错误", MB_OK | MB_ICONWARNING);

    return 1;
}

