// ��װ���׽���socket���Լ�һЩ���÷��� ��Ϊ�ӿڱ�����

#pragma once

#include <WinSock2.h>
#include <string>
#include <iostream>

class TcpSocket
{
public:
	TcpSocket();

	static std::string fromDomainToIP(std::string domain);

	bool connectTo(std::string domain, int port);
	void dissconnect();
	bool sendData(const char* data, unsigned int size);
	int recvData(char* data, int size);

	bool isConnected() {
		return (int)mSock != SOCKET_ERROR;
	}

	std::string mIp;
	int mPort;

private:
	SOCKET mSock;
	struct sockaddr_in mAddr;
};

