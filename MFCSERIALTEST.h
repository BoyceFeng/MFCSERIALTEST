
// MFCSERIALTEST.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CMFCSERIALTESTApp: 
// �йش����ʵ�֣������ MFCSERIALTEST.cpp
//

class CMFCSERIALTESTApp : public CWinApp
{
public:
	CMFCSERIALTESTApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CMFCSERIALTESTApp theApp;