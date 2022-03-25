
// FileMD5Check.h: FileMD5Check 应用程序的主头文件
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"       // 主符号


// CFileMD5CheckApp:
// 有关此类的实现，请参阅 FileMD5Check.cpp
//

class CFileMD5CheckApp : public CWinApp
{
public:
	CFileMD5CheckApp() noexcept;


// 重写
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// 实现

public:
};

extern CFileMD5CheckApp theApp;
