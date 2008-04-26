// FILE:  MyService.cpp
//
// DESCRIPTION:
// This file contains the implementation of a Windows NT service, encapsulated in a
// class derived from the CNTService class.  Contained herein are initialization
// routines, log file operations, and code for the listening threads.
//

// Visual C++ Definitions
#include "stdafx.h"
#include "afxmt.h"
#include "winreg.h"
#include "winsvc.h"

// Standard C Definitions
#include "io.h"
#include "direct.h"

// Application-specific Definitions
#include "NTServApp.h"
#include "myservice.h"


CMyService::CMyService() : CNTService("BeServed", "BeServed File and Print Services")
{
}


BOOL CMyService::OnInit()
{
	extern void BeServedStartup(CMyService *service);
	char path[_MAX_PATH], *pStr;

	// Get the executable file path
	GetModuleFileName(NULL, path, sizeof(path));
	pStr = strrchr(path, '\\');
	if (pStr)
		*pStr = 0;

	SetCurrentDirectory(path);

	BeServedStartup(this);
	return true;
}

// OnStop()
//
void CMyService::OnStop()
{
	extern void endService(int);
	endService(0);
}

// Restart()
//
void CMyService::Restart()
{
	OnStop();
	OnInit();
}

// IsRunning()
//
BOOL CMyService::IsRunning()
{
	return (m_bIsRunning);
}

// Run()
//
BOOL CMyService::Run()
{
	extern void loadShares();
	extern void startService();
	loadShares();
	startService();
	return true;
}

// Process user control requests
BOOL CMyService::OnUserControl(DWORD dwOpcode)
{
	extern void restartService();

	switch (dwOpcode)
	{
		case SERVICE_USER_CONTROL_HUP:
			restartService();
			return true;

		default:
			break;
	}

	return false;
}

void CMyService::InitLogging()
{
}

