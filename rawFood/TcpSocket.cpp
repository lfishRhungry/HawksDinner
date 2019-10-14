#include "pch.h"
#include "TcpSocket.h"

TcpSocket::TcpSocket() : mSock(SOCKET_ERROR)
{

}

std::string TcpSocket::fromDomainToIP(std::string domain)
{
	struct hostent* ht = gethostbyname(domain.data());
	if (!ht) {
		OutputDebugStringA("Failed to get host:");
		OutputDebugStringA(domain.data());
		OutputDebugStringA("\r\n");

		return std::string();
	}

	// ����192.168.2.14 �����Ƿֶ���� 192. + 168. + 2. + 14. �����ȥ����β��һ����
	std::string ip;
	for (int i = 0; i < ht->h_length; i++) {
		char szTmp[10];
		::sprintf_s(szTmp, sizeof(szTmp), "%d.", (unsigned char)ht->h_addr_list[0][i]);
		ip.append(szTmp);
	}
	ip.erase(ip.length() - 1);
	return ip;
}

bool TcpSocket::connectTo(std::string domain, int port)
{
	// �����ӵ�socket
	if ((int)mSock != SOCKET_ERROR) {
		OutputDebugStringA("Socket is using\r\n");

			return false;
	}

	mPort = port;

	mIp = fromDomainToIP(domain);

	// �����׽��� 0��ʾ�Ȳ�ָ��Э��
	if ((int)(mSock = socket(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR) {
		OutputDebugStringA("Failed to create socket ");
		OutputDebugStringA(mIp.data());
		OutputDebugStringA("\r\n");

		return false;
	}
	// ���õ�ַ�Ͷ˿�
	mAddr.sin_family = AF_INET;
	mAddr.sin_addr.S_un.S_addr = inet_addr(mIp.data());
	mAddr.sin_port = htons(mPort);

	// ����
	if (connect(mSock, (SOCKADDR*)&mAddr, sizeof(mAddr)) == SOCKET_ERROR) {
		OutputDebugStringA("Failed to connect to ");
		OutputDebugStringA(mIp.data());
		OutputDebugStringA("\r\n");

		dissconnect();
		return false;
	}

	OutputDebugStringA("Connect to success\r\n");


	return true;
}

void TcpSocket::dissconnect()
{
	if ((int)mSock != SOCKET_ERROR) {
		closesocket(mSock);
		mSock = SOCKET_ERROR;

		OutputDebugStringA("Closed socket from ");
		OutputDebugStringA(mIp.data());
		OutputDebugStringA("\r\n");

	}
}

bool TcpSocket::sendData(const char* data, unsigned int size)
{
	// ���뽨�������˲ſ��Է�������
	if ((int)mSock == SOCKET_ERROR) {
		OutputDebugStringA("Socket do not allowed to send data without connected\r\n");

		return false;
	}

	int ret = SOCKET_ERROR;
	// �Զ���ÿ�η���������С���ֽڣ�
	// ����һ�η��͹�������
	const unsigned int packetLen = 800;
	if (size <= packetLen) {
		ret = send(mSock, data, size, 0);

		if (ret == SOCKET_ERROR) {
			OutputDebugStringA("Failed to send data to ");
			OutputDebugStringA(mIp.data());
			OutputDebugStringA("\r\n");

			dissconnect();
		}
	}
	else {
		unsigned int pos = 0;
		while (pos < size) {
			unsigned int sendSize = pos + packetLen > size ? size - pos : packetLen;

			ret = send(mSock, data + pos, sendSize, 0);

			if (ret == SOCKET_ERROR) {
				OutputDebugStringA("Failed to send data to ");
				OutputDebugStringA(mIp.data());
				OutputDebugStringA("\r\n");

				dissconnect();
				break;
			}

			pos += packetLen;
		}
	}

	return ret != SOCKET_ERROR ? true : false;
}

int TcpSocket::recvData(char* data, int size)
{
	if ((int)mSock == SOCKET_ERROR) {
		OutputDebugStringA("Socket do not allowed to reveive data without connected\r\n");

		return -1;
	}

	int ret = recv(mSock, data, size, 0);

	if (ret == SOCKET_ERROR || ret == 0) {
		OutputDebugStringA("Failed to receive data from ");
		OutputDebugStringA(mIp.data());
		OutputDebugStringA("\r\n");

		dissconnect();
	}

	return ret;
}

