#pragma once
#include <Atlbase.h>
#include <comdef.h>
#include <taskschd.h>
#pragma comment(lib, "taskschd.lib")

class CMyTaskSchedule
{
private:

	ITaskService* m_lpITS;
	ITaskFolder* m_lpRootFolder;

public:

	CMyTaskSchedule(void);
	~CMyTaskSchedule(void);

public:

	// delete specific task
	BOOL Delete(char* lpszTaskName);
	BOOL DeleteFolder(char* lpszFolderName);

	// create task
	BOOL NewTask(char* lpszTaskName, char* lpszProgramPath, char* lpszParameters, char* lpszAuthor = (char*)"");

	// judge if task exists
	BOOL IsExist(char* lpszTaskName);

	// judge if task effective
	BOOL IsTaskValid(char* lpszTaskName);

	// run specific task
	BOOL Run(char* lpszTaskName, char* lpszParam);

	// judge if task running
	BOOL IsEnable(char* lpszTaskName);

	// set specific task start using or not
	BOOL SetEnable(char* lpszTaskName, BOOL bEnable);

};

