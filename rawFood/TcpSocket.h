// 封装了套接字socket的以及一些常用方法 作为接口被调用

#pragma once

#include <WinSock2.h>
#include <string>

class TcpSocket
{
public:
	// 初始化mSock为无效值
	TcpSocket();

	static std::string fromDomainToIP(std::string domain);

	// 将socket连接到指定域名
	bool connectTo(std::string domain, int port);
	// 端口socket连接
	void dissconnect();
	// 发送数据
	bool sendData(const char* data, unsigned int size);
	// 接受数据
	int recvData(char* data, int size);
	// 判断是否正常连接正常
	bool isConnected() {
		return (int)mSock != SOCKET_ERROR;
	}

	std::string mIp; // 所连接的IP
	int mPort; // 用来设置端口

// 封装一个socket和sockaddr_in
private:
	SOCKET mSock;
	struct sockaddr_in mAddr;
};

