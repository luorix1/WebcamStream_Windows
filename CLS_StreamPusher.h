
// CLS_StreamPusher.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#include "resource.h"		// ������


// CLS_StreamPusherApp: 
// �йش����ʵ�֣������ CLS_StreamPusher.cpp
//

class CLS_StreamPusherApp : public CWinApp
{
public:
	CLS_StreamPusherApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CLS_StreamPusherApp theApp;