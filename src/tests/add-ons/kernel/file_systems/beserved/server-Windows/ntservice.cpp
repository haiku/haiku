// NTService.cpp
//
// Implementation of CNTService

// Visual C++ Definitions
#include "stdafx.h"
#include "winreg.h"
#include "winsvc.h"

// Standard C Definitions
#include "stdio.h"

// Application-specific Definitions
#include "NTService.h"

CNTService* CNTService::m_pThis = NULL;


CNTService::CNTService(LPCTSTR pServiceName, LPCSTR pDisplayName)
{
	OSVERSIONINFO osInfo;

    // copy the address of the current object so we can access it from
    // the static member callback functions. 
    // WARNING: This limits the application to only one CNTService object. 
    m_pThis = this;

    // Set the default service name and version
	memset(m_szServiceName, 0, sizeof(m_szServiceName));
	memset(m_szDisplayName, 0, sizeof(m_szDisplayName));
    strncpy(m_szServiceName, pServiceName, sizeof(m_szServiceName) - 1);
    strncpy(m_szDisplayName, pDisplayName, sizeof(m_szDisplayName) - 1);
    m_hEventSource = NULL;

    // set up the initial service status 
    m_hServiceStatus = NULL;
    m_Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_Status.dwCurrentState = SERVICE_STOPPED;
    m_Status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    m_Status.dwWin32ExitCode = 0;
    m_Status.dwServiceSpecificExitCode = 0;
    m_Status.dwCheckPoint = 0;
    m_Status.dwWaitHint = 0;
    m_bIsRunning = FALSE;

	// Determine if we're running on a Windows 9x platform, including Windows Me.
	// If so, we won't be able to support Windows NT services.
	osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (GetVersionEx(&osInfo))
		m_bIsWindows9x = (osInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
}

CNTService::~CNTService()
{
    DebugMsg("CNTService::~CNTService()");

	if (!m_bIsWindows9x)
		if (m_hEventSource)
			DeregisterEventSource(m_hEventSource);
}

////////////////////////////////////////////////////////////////////////////////////////
// Default command line argument parsing

// Returns TRUE if it found an arg it recognised, FALSE if not
// Note: processing some arguments causes output to stdout to be generated.
BOOL CNTService::ParseStandardArgs(int argc, char* argv[])
{
	if (argc <= 1)
		return FALSE;

	if (m_bIsWindows9x)
		return TRUE;

	if (_stricmp(argv[1], "-version") == 0)
	{
		// Report version information
		printf("%s 1.2.5\n", m_szServiceName);
		printf("The service is %s installed.\n", IsInstalled() ? "currently" : "not");
		return TRUE;
	}
	else if (_stricmp(argv[1], "-install") == 0)
	{
        // Install the service
		if (IsInstalled())
			printf("%s is already installed.\n", m_szServiceName);
		else if (Install())
			printf("%s installed.\n", m_szServiceName);
		else
			printf("%s failed to install. Error %d\n", m_szServiceName, GetLastError());

		return TRUE;
	}
	else if (_stricmp(argv[1], "-remove") == 0)
	{
		// Uninstall the service
		if (!IsInstalled())
			printf("%s is not installed\n", m_szServiceName);
		else if (Uninstall())
		{
			// Get the executable file path
			char szFilePath[_MAX_PATH];
			::GetModuleFileName(NULL, szFilePath, sizeof(szFilePath));
			printf("%s removed. (You must delete the file (%s) yourself.)\n", m_szServiceName, szFilePath);
		}
		else printf("Could not remove %s. Error %d\n", m_szServiceName, GetLastError());

		return TRUE;
	}

	// Return FALSE here because the arguments were invalid
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////
// Install/uninstall routines

// Test if the service is currently installed
BOOL CNTService::IsInstalled()
{
	BOOL bResult = FALSE;

	if (!m_bIsWindows9x)
	{
		// Open the Service Control Manager
		SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (hSCM)
		{
			// Try to open the service
			SC_HANDLE hService = ::OpenService(hSCM, m_szServiceName, SERVICE_QUERY_CONFIG);
			if (hService)
			{
				bResult = TRUE;
				::CloseServiceHandle(hService);
			}

			::CloseServiceHandle(hSCM);
		}
	}

	return bResult;
}

BOOL CNTService::Install()
{
    HKEY hKey = NULL;
	DWORD dwData;
	char szFilePath[_MAX_PATH], szKey[256];

	// Don't attempt to install on Windows 9x.
	if (m_bIsWindows9x)
		return TRUE;

	// Open the Service Control Manager
	SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCM)
		return FALSE;

	// Get the executable file path
	GetModuleFileName(NULL, szFilePath, sizeof(szFilePath));

	// Create the service
	SC_HANDLE hService = CreateService(hSCM, m_szServiceName, m_szDisplayName, SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
									   SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, szFilePath, NULL, NULL, NULL, NULL, NULL);
	if (!hService)
	{
		CloseServiceHandle(hSCM);
		return FALSE;
	}

	// make registry entries to support logging messages
	// Add the source name as a subkey under the Application
	// key in the EventLog service portion of the registry.
	strcpy(szKey, "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\");
	strcat(szKey, m_szServiceName);
	if (RegCreateKey(HKEY_LOCAL_MACHINE, szKey, &hKey) != ERROR_SUCCESS)
	{
		CloseServiceHandle(hService);
		CloseServiceHandle(hSCM);
		return FALSE;
	}

	// Add the Event ID message-file name to the 'EventMessageFile' subkey.
	RegSetValueEx(hKey, "EventMessageFile", 0, REG_EXPAND_SZ, (CONST BYTE *) szFilePath, strlen(szFilePath) + 1);

	// Set the supported types flags.
	dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
	RegSetValueEx(hKey, "TypesSupported", 0, REG_DWORD, (CONST BYTE *) &dwData, sizeof(DWORD));
	RegCloseKey(hKey);

	LogEvent(EVENTLOG_INFORMATION_TYPE, EVMSG_INSTALLED, m_szServiceName);
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCM);
	return TRUE;
}

BOOL CNTService::Uninstall()
{
	BOOL bResult = FALSE;

	if (!m_bIsWindows9x)
	{
		// Open the Service Control Manager
		SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (!hSCM)
			return FALSE;

		SC_HANDLE hService = OpenService(hSCM, m_szServiceName, DELETE);
		if (hService)
		{
			if (DeleteService(hService))
			{
				LogEvent(EVENTLOG_INFORMATION_TYPE, EVMSG_REMOVED, m_szServiceName);
				bResult = TRUE;
			}
			else LogEvent(EVENTLOG_ERROR_TYPE, EVMSG_NOTREMOVED, m_szServiceName);

			CloseServiceHandle(hService);
		}

		CloseServiceHandle(hSCM);
	}

	return bResult;
}

///////////////////////////////////////////////////////////////////////////////////////
// Logging functions

// This function makes an entry into the application event log
void CNTService::LogEvent(WORD wType, DWORD dwID, const char* pszS1, const char* pszS2, const char* pszS3)
{
	const char* ps[3];
	int iStr = 0;

	if (!m_bIsWindows9x)
	{
		ps[0] = pszS1;
		ps[1] = pszS2;
		ps[2] = pszS3;

		for (int i = 0; i < 3; i++)
			if (ps[i] != NULL) 
				iStr++;

		// Check the event source has been registered and if
		// not then register it now
		if (!m_hEventSource)
			m_hEventSource = RegisterEventSource(NULL, m_szServiceName);

		if (m_hEventSource)
			ReportEvent(m_hEventSource, wType, 0, dwID, NULL, iStr, 0, ps, NULL);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Service startup and registration

BOOL CNTService::StartService()
{
	SERVICE_TABLE_ENTRY st[] =
	{
		{ m_szServiceName, ServiceMain },
		{ NULL, NULL }
	};

	// There is no need to start the service on Windows 9x.
	if (m_bIsWindows9x)
	{
		ServiceMain(0, NULL);
		return TRUE;
	}

	BOOL bSuccess = StartServiceCtrlDispatcher(st);
	DWORD dwError = GetLastError();
	DebugMsg("Returned from StartServiceCtrlDispatcher() with bSuccess=%d,dwError=%ld", bSuccess, dwError);
	return bSuccess;
}

// static member function (callback)
void CNTService::ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
	// Get a pointer to the C++ object
	CNTService* pService = m_pThis;

	pService->DebugMsg("Entering CNTService::ServiceMain()");

	// Register the control request handler if we're not on Windows 9x.
	if (!pService->m_bIsWindows9x)
	{
		pService->m_Status.dwCurrentState = SERVICE_START_PENDING;
		pService->m_hServiceStatus = RegisterServiceCtrlHandler(pService->m_szServiceName, Handler);
		if (pService->m_hServiceStatus == NULL) 
		{
			pService->LogEvent(EVENTLOG_ERROR_TYPE, EVMSG_CTRLHANDLERNOTINSTALLED);
			return;
		}
	}

	// Start the initialization
	if (pService->Initialize())
	{
		pService->m_bIsRunning = TRUE;
		pService->m_Status.dwWin32ExitCode = 0;
		pService->m_Status.dwCheckPoint = 0;
		pService->m_Status.dwWaitHint = 0;

		pService->Run();
	}

	// Tell the service manager we are stopped
	pService->SetStatus(SERVICE_STOPPED);
	pService->DebugMsg("Leaving CNTService::ServiceMain()");
}

///////////////////////////////////////////////////////////////////////////////////////////
// status functions

void CNTService::SetStatus(DWORD dwState)
{
	static DWORD dwCheckPoint = 1;

	if (!m_bIsWindows9x)
	{
		if (m_Status.dwCurrentState == dwState)
			m_Status.dwCheckPoint = dwCheckPoint++;

		m_Status.dwCurrentState = dwState;
		SetServiceStatus(m_hServiceStatus, &m_Status);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
// Service initialization

BOOL CNTService::Initialize()
{
	DebugMsg("Entering CNTService::Initialize()");

	// Start the initialization
	SetStatus(SERVICE_START_PENDING);

	// Perform the actual initialization
	BOOL bResult = OnInit();

	// Set final state
	m_Status.dwWin32ExitCode = GetLastError();
	m_Status.dwCheckPoint = 0;
	m_Status.dwWaitHint = 0;
	if (!bResult)
	{
		LogEvent(EVENTLOG_ERROR_TYPE, EVMSG_FAILEDINIT);
		SetStatus(SERVICE_STOPPED);
		return FALSE;    
	}

	LogEvent(EVENTLOG_INFORMATION_TYPE, EVMSG_STARTED);
	SetStatus(SERVICE_RUNNING);

	DebugMsg("Leaving CNTService::Initialize()");
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// main function to do the real work of the service

// This function performs the main work of the service. 
// When this function returns the service has stopped.
BOOL CNTService::Run()
{
	DebugMsg("Entering CNTService::Run()");

	while (m_bIsRunning)
	{
		DebugMsg("Sleeping...");
		Sleep(5000);
	}

	DebugMsg("Leaving CNTService::Run()");
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////
// Control request handlers

// static member function (callback) to handle commands from the
// service control manager
void CNTService::Handler(DWORD dwOpcode)
{
	// Get a pointer to the object
	CNTService* pService = m_pThis;

	switch (dwOpcode)
	{
		case SERVICE_CONTROL_STOP:
			pService->SetStatus(SERVICE_STOP_PENDING);
			pService->m_bIsRunning = FALSE;
			pService->OnStop();
			pService->SetStatus(SERVICE_STOPPED);
			pService->LogEvent(EVENTLOG_INFORMATION_TYPE, EVMSG_STOPPED);
			break;

		case SERVICE_CONTROL_PAUSE:
			pService->SetStatus(SERVICE_PAUSE_PENDING);
			pService->OnPause();
			pService->SetStatus(SERVICE_PAUSED);
			break;

		case SERVICE_CONTROL_CONTINUE:
			pService->SetStatus(SERVICE_CONTINUE_PENDING);
			pService->OnContinue();
			pService->SetStatus(SERVICE_RUNNING);
			break;

		case SERVICE_CONTROL_INTERROGATE:
			pService->OnInterrogate();
			break;

		case SERVICE_CONTROL_SHUTDOWN:
			pService->OnShutdown();
			break;

		default:
			if (dwOpcode >= SERVICE_CONTROL_USER) 
			{
				if (!pService->OnUserControl(dwOpcode))
					pService->LogEvent(EVENTLOG_ERROR_TYPE, EVMSG_BADREQUEST);
			}
			else pService->LogEvent(EVENTLOG_ERROR_TYPE, EVMSG_BADREQUEST);
			break;
	}

	// Report current status
	SetServiceStatus(pService->m_hServiceStatus, &pService->m_Status);
}

// Called when the service is first initialized
BOOL CNTService::OnInit()
{
	DebugMsg("CNTService::OnInit()");
	return TRUE;
}

// Called when the service control manager wants to stop the service
void CNTService::OnStop()
{
	DebugMsg("CNTService::OnStop()");
}

// called when the service is interrogated
void CNTService::OnInterrogate()
{
	DebugMsg("CNTService::OnInterrogate()");
}

// called when the service is paused
void CNTService::OnPause()
{
	DebugMsg("CNTService::OnPause()");
}

// called when the service is continued
void CNTService::OnContinue()
{
	DebugMsg("CNTService::OnContinue()");
}

// called when the service is shut down
void CNTService::OnShutdown()
{
	DebugMsg("CNTService::OnShutdown()");
}

// called when the service gets a user control message
BOOL CNTService::OnUserControl(DWORD dwOpcode)
{
	DebugMsg("CNTService::OnUserControl(%8.8lXH)", dwOpcode);
	return FALSE;
}


////////////////////////////////////////////////////////////////////////////////////////////
// Debugging support

void CNTService::DebugMsg(const char* pszFormat, ...)
{
	char buf[1024];
	sprintf(buf, "[%s](%lu): ", m_szServiceName, GetCurrentThreadId());
	va_list arglist;
	va_start(arglist, pszFormat);
	vsprintf(&buf[strlen(buf)], pszFormat, arglist);
	va_end(arglist);
	strcat(buf, "\n");
	OutputDebugString(buf);
}
