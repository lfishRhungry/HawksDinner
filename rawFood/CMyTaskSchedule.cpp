#include "pch.h"
#include "CMyTaskSchedule.h"

CMyTaskSchedule::CMyTaskSchedule(void)
{
	m_lpITS = NULL;
	m_lpRootFolder = NULL;
	// init COM
	HRESULT hr = ::CoInitialize(NULL);
	if (FAILED(hr))
	{
		return;
	}
	// create an instance of Task Service
	hr = ::CoCreateInstance(CLSID_TaskScheduler,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_ITaskService,
		(LPVOID*)(&m_lpITS));
	if (FAILED(hr))
	{
		return;
	}
	// connect to Task Service
	hr = m_lpITS->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
	if (FAILED(hr))
	{
		return;
	}
	// get ptr of Root Task Folder pointing the last registing task
	hr = m_lpITS->GetFolder(_bstr_t("\\"), &m_lpRootFolder);
	if (FAILED(hr))
	{
		return;
	}
}

CMyTaskSchedule::~CMyTaskSchedule(void)
{
	if (m_lpITS)
	{
		m_lpITS->Release();
	}
	if (m_lpRootFolder)
	{
		m_lpRootFolder->Release();
	}
	// uninstall COM
	::CoUninitialize();
}

BOOL CMyTaskSchedule::Delete(char* lpszTaskName)
{
	if (NULL == m_lpRootFolder)
	{
		return FALSE;
	}
	CComVariant variantTaskName(NULL);
	variantTaskName = lpszTaskName;
	HRESULT hr = m_lpRootFolder->DeleteTask(variantTaskName.bstrVal, 0);
	if (FAILED(hr))
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CMyTaskSchedule::DeleteFolder(char* lpszFolderName)
{
	if (NULL == m_lpRootFolder)
	{
		return FALSE;
	}
	CComVariant variantFolderName(NULL);
	variantFolderName = lpszFolderName;
	HRESULT hr = m_lpRootFolder->DeleteFolder(variantFolderName.bstrVal, 0);
	if (FAILED(hr))
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CMyTaskSchedule::NewTask(char* lpszTaskName, char* lpszProgramPath, char* lpszParameters, char* lpszAuthor)
{
	if (NULL == m_lpRootFolder)
	{
		return FALSE;
	}
	// delete the same task at first
	Delete(lpszTaskName);
	// create ITaskDefinition object to create task
	ITaskDefinition* pTaskDefinition = NULL;
	HRESULT hr = m_lpITS->NewTask(0, &pTaskDefinition);
	if (FAILED(hr))
	{
		return FALSE;
	}

	/* set registry info */
	IRegistrationInfo* pRegInfo = NULL;
	CComVariant variantAuthor(NULL);
	variantAuthor = lpszAuthor;
	hr = pTaskDefinition->get_RegistrationInfo(&pRegInfo);
	if (FAILED(hr))
	{
		return FALSE;
	}
	// set author info
	hr = pRegInfo->put_Author(variantAuthor.bstrVal);
	pRegInfo->Release();

	/* set logging type and privilege of running */
	IPrincipal* pPrincipal = NULL;
	hr = pTaskDefinition->get_Principal(&pPrincipal);
	if (FAILED(hr))
	{
		return FALSE;
	}
	// set logging type
	hr = pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
	// set privilege of running
	// supreme privilege
	hr = pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);
	pPrincipal->Release();

	/* set other info */
	ITaskSettings* pSettting = NULL;
	hr = pTaskDefinition->get_Settings(&pSettting);
	if (FAILED(hr))
	{
		return FALSE;
	}
	// set other info
	hr = pSettting->put_StopIfGoingOnBatteries(VARIANT_FALSE);
	hr = pSettting->put_DisallowStartIfOnBatteries(VARIANT_FALSE);
	hr = pSettting->put_AllowDemandStart(VARIANT_TRUE);
	hr = pSettting->put_StartWhenAvailable(VARIANT_FALSE);
	hr = pSettting->put_MultipleInstances(TASK_INSTANCES_PARALLEL);
	pSettting->Release();

	/* create action */
	IActionCollection* pActionCollect = NULL;
	hr = pTaskDefinition->get_Actions(&pActionCollect);
	if (FAILED(hr))
	{
		return FALSE;
	}
	IAction* pAction = NULL;
	// creating executing
	hr = pActionCollect->Create(TASK_ACTION_EXEC, &pAction);
	pActionCollect->Release();

	/* set dir of exe and parameters */
	CComVariant variantProgramPath(NULL);
	CComVariant variantParameters(NULL);
	IExecAction* pExecAction = NULL;
	hr = pAction->QueryInterface(IID_IExecAction, (PVOID*)(&pExecAction));
	if (FAILED(hr))
	{
		pAction->Release();
		return FALSE;
	}
	pAction->Release();
	// set dir of exe and parameters
	variantProgramPath = lpszProgramPath;
	variantParameters = lpszParameters;
	pExecAction->put_Path(variantProgramPath.bstrVal);
	pExecAction->put_Arguments(variantParameters.bstrVal);
	pExecAction->Release();

	/* create trigger and achieve auto-starting */
	ITriggerCollection* pTriggers = NULL;
	hr = pTaskDefinition->get_Triggers(&pTriggers);
	if (FAILED(hr))
	{
		return FALSE;
	}
	// create trigger
	ITrigger* pTrigger = NULL;
	hr = pTriggers->Create(TASK_TRIGGER_LOGON, &pTrigger);
	if (FAILED(hr))
	{
		return FALSE;
	}

	/* regist the task */
	IRegisteredTask* pRegisteredTask = NULL;
	CComVariant variantTaskName(NULL);
	variantTaskName = lpszTaskName;
	hr = m_lpRootFolder->RegisterTaskDefinition(variantTaskName.bstrVal,
		pTaskDefinition,
		TASK_CREATE_OR_UPDATE,
		_variant_t(),
		_variant_t(),
		TASK_LOGON_INTERACTIVE_TOKEN,
		_variant_t(""),
		&pRegisteredTask);
	if (FAILED(hr))
	{
		pTaskDefinition->Release();
		return FALSE;
	}
	pTaskDefinition->Release();
	pRegisteredTask->Release();

	return TRUE;
}

BOOL CMyTaskSchedule::IsExist(char* lpszTaskName)
{
	if (NULL == m_lpRootFolder)
	{
		return FALSE;
	}
	HRESULT hr = S_OK;
	CComVariant variantTaskName(NULL);
	CComVariant variantEnable(NULL);
	variantTaskName = lpszTaskName;
	IRegisteredTask* pRegisteredTask = NULL;
	// get specific task
	hr = m_lpRootFolder->GetTask(variantTaskName.bstrVal, &pRegisteredTask);
	if (FAILED(hr) || (NULL == pRegisteredTask))
	{
		return FALSE;
	}
	pRegisteredTask->Release();

	return TRUE;
}

BOOL CMyTaskSchedule::IsTaskValid(char* lpszTaskName)
{
	if (NULL == m_lpRootFolder)
	{
		return FALSE;
	}
	HRESULT hr = S_OK;
	CComVariant variantTaskName(NULL);
	CComVariant variantEnable(NULL);
	variantTaskName = lpszTaskName;
	IRegisteredTask* pRegisteredTask = NULL;
	// get specific task
	hr = m_lpRootFolder->GetTask(variantTaskName.bstrVal, &pRegisteredTask);
	if (FAILED(hr) || (NULL == pRegisteredTask))
	{
		return FALSE;
	}
	// get status of task
	TASK_STATE taskState;
	hr = pRegisteredTask->get_State(&taskState);
	if (FAILED(hr))
	{
		pRegisteredTask->Release();
		return FALSE;
	}
	pRegisteredTask->Release();
	// disabled task
	if (TASK_STATE_DISABLED == taskState)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CMyTaskSchedule::Run(char* lpszTaskName, char* lpszParam)
{
	if (NULL == m_lpRootFolder)
	{
		return FALSE;
	}
	HRESULT hr = S_OK;
	CComVariant variantTaskName(NULL);
	CComVariant variantParameters(NULL);
	variantTaskName = lpszTaskName;
	variantParameters = lpszParam;

	// get task
	IRegisteredTask* pRegisteredTask = NULL;
	hr = m_lpRootFolder->GetTask(variantTaskName.bstrVal, &pRegisteredTask);
	if (FAILED(hr) || (NULL == pRegisteredTask))
	{
		return FALSE;
	}
	// run
	hr = pRegisteredTask->Run(variantParameters, NULL);
	if (FAILED(hr))
	{
		pRegisteredTask->Release();
		return FALSE;
	}
	pRegisteredTask->Release();

	return TRUE;
}

BOOL CMyTaskSchedule::IsEnable(char* lpszTaskName)
{
	if (NULL == m_lpRootFolder)
	{
		return FALSE;
	}
	HRESULT hr = S_OK;
	CComVariant variantTaskName(NULL);
	CComVariant variantEnable(NULL);
	variantTaskName = lpszTaskName;
	IRegisteredTask* pRegisteredTask = NULL;
	// get the task
	hr = m_lpRootFolder->GetTask(variantTaskName.bstrVal, &pRegisteredTask);
	if (FAILED(hr) || (NULL == pRegisteredTask))
	{
		return FALSE;
	}
	// get if starting using
	pRegisteredTask->get_Enabled(&variantEnable.boolVal);
	pRegisteredTask->Release();
	if (ATL_VARIANT_FALSE == variantEnable.boolVal)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CMyTaskSchedule::SetEnable(char* lpszTaskName, BOOL bEnable)
{
	if (NULL == m_lpRootFolder)
	{
		return FALSE;
	}
	HRESULT hr = S_OK;
	CComVariant variantTaskName(NULL);
	CComVariant variantEnable(NULL);
	variantTaskName = lpszTaskName;
	variantEnable = bEnable;                            // if start-using
	IRegisteredTask* pRegisteredTask = NULL;
	// get the task
	hr = m_lpRootFolder->GetTask(variantTaskName.bstrVal, &pRegisteredTask);
	if (FAILED(hr) || (NULL == pRegisteredTask))
	{
		return FALSE;
	}
	// set if starting using
	pRegisteredTask->put_Enabled(variantEnable.boolVal);
	pRegisteredTask->Release();

	return TRUE;
}
