// 做一些准备性的工作

#pragma once
#include "pch.h"

// 通过创建互斥量来验证是否是单一实例
BOOL IsAlreadyExiting();

// 是否已做好准备工作
BOOL IsReady();
