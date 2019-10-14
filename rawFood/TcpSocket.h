// ��װ���׽���socket���Լ�һЩ���÷��� ��Ϊ�ӿڱ�����

#pragma once

#include <WinSock2.h>
#include <string>

class TcpSocket
{
public:
	// ��ʼ��mSockΪ��Чֵ
	TcpSocket();

	static std::string fromDomainToIP(std::string domain);

	// ��socket���ӵ�ָ������
	bool connectTo(std::string domain, int port);
	// �˿�socket����
	void dissconnect();
	// ��������
	bool sendData(const char* data, unsigned int size);
	// ��������
	int recvData(char* data, int size);
	// �ж��Ƿ�������������
	bool isConnected() {
		return (int)mSock != SOCKET_ERROR;
	}

	std::string mIp; // �����ӵ�IP
	int mPort; // �������ö˿�

// ��װһ��socket��sockaddr_in
private:
	SOCKET mSock;
	struct sockaddr_in mAddr;
};

