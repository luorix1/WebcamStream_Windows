
// CLS_StreamPusher.h : PROJECT_NAME 应用程序的主头文件
//

#pragma once

#include "resource.h"		// 主符号


// CLS_StreamPusherApp: 
// 有关此类的实现，请参阅 CLS_StreamPusher.cpp
//

class CLS_StreamPusherApp : public CWinApp
{
public:
	CLS_StreamPusherApp();

// 重写
public:
	virtual BOOL InitInstance();

// 实现

	DECLARE_MESSAGE_MAP()
};

extern CLS_StreamPusherApp theApp;