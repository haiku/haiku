// ntservice.h
//
// Definitions for CNTService
//

#ifndef _NTSERVICE_H_
#define _NTSERVICE_H_

#include "ntservmsg.h"

#define SERVICE_CONTROL_USER 128

class CNTService
{
public:
	CNTService(LPCTSTR pServiceName, LPCTSTR pDisplayName);
	virtual ~CNTService();

	BOOL ParseStandardArgs(int argc, char* argv[]);
	BOOL IsInstalled();
	BOOL Install();
	BOOL Uninstall();
	void LogEvent(WORD wType, DWORD dwID, const char* pszS1 = NULL, const char* pszS2 = NULL, const char* pszS3 = NULL);
	BOOL StartService();
	void SetStatus(DWORD dwState);
	BOOL Initialize();
	virtual BOOL Run();
	virtual BOOL OnInit();
	virtual void OnStop();
	virtual void OnInterrogate();
	virtual void OnPause();
	virtual void OnContinue();
	virtual void OnShutdown();
	virtual BOOL OnUserControl(DWORD dwOpcode);
	void DebugMsg(const char* pszFormat, ...);

	// static member functions
	static void WINAPI ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);
	static void WINAPI Handler(DWORD dwOpcode);

	// data members
	char m_szServiceName[64];
	char m_szDisplayName[64];
	SERVICE_STATUS_HANDLE m_hServiceStatus;
	SERVICE_STATUS m_Status;
	BOOL m_bIsRunning;

	// This static data is used to obtain the object pointer, but unfortunately limits
	// this implementation to one CNTService class per executable.
	static CNTService* m_pThis;

private:
	HANDLE m_hEventSource;
	BOOL m_bIsWindows9x;
};

#endif // _NTSERVICE_H_
