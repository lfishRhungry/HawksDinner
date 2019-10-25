// 通过创建批处理文件删除自身和辅助dll

#pragma once
#include "pch.h"
#include <stdio.h>
#include <strsafe.h>
#include "CMyTaskSchedule.h"

// 保存修改peb前的原路径
extern char g_szCurrentDirectory[MAX_PATH];

// 生产批处理文件
BOOL CreatePingBat(char* pszBatFileName);
// 执行批处理文件并删除自身
BOOL DelSelf();
