// ͨ�������������ļ�ɾ������͸���dll

#pragma once
#include "pch.h"
#include <stdio.h>
#include <strsafe.h>
#include "CMyTaskSchedule.h"

// �����޸�pebǰ��ԭ·��
extern char g_szCurrentDirectory[MAX_PATH];

// �����������ļ�
BOOL CreatePingBat(char* pszBatFileName);
// ִ���������ļ���ɾ������
BOOL DelSelf();
