// MainPanel.cpp : implementation file
//

#include "stdafx.h"
#include "winsvc.h"

#include "FileSharing.h"
#include "MainPanel.h"


#define SERVICE_CONTROL_USER			128
#define SERVICE_USER_CONTROL_HUP		SERVICE_CONTROL_USER + 1

char settingsFile[_MAX_PATH];


// CMainPanel dialog

IMPLEMENT_DYNAMIC(CMainPanel, CPropertySheet)
CMainPanel::CMainPanel(CWnd* pParent /*=NULL*/)
	: CPropertySheet("BeServed File and Print Services", pParent), m_filePanel()
{
	AddPage(&m_filePanel);
	AddPage(&m_printerPanel);
}

CMainPanel::~CMainPanel()
{
}

void CMainPanel::DoDataExchange(CDataExchange* pDX)
{
	CPropertySheet::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CMainPanel, CPropertySheet)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CMainPanel message handlers

BOOL CMainPanel::OnInitDialog()
{
	HKEY hRegKey;
	DWORD dwType = 0, dwSize;
	char *p;

	// Get service executable folder.
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\BeServed", 0, KEY_READ, &hRegKey) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx(hRegKey, (LPTSTR) "ImagePath", NULL, &dwType, NULL, &dwSize) == ERROR_SUCCESS)
			RegQueryValueEx(hRegKey, (LPTSTR) "ImagePath", NULL, &dwType, (LPBYTE) settingsFile, &dwSize);

		RegCloseKey(hRegKey);
	}

	if ((p = strrchr(settingsFile, '\\')) != NULL)
		*p = 0;

	strcat(settingsFile, "\\BeServed-Settings");

	BOOL bResult = CPropertySheet::OnInitDialog();
	return bResult;
}

// OnBnClickedOk()
//
void CMainPanel::OnBnClickedOk()
{
	extern unsigned authServerIP;
	extern char authServerName[];
	SC_HANDLE hManager, hService;
	SERVICE_STATUS status;
	FILE *fp;

	fp = fopen(settingsFile, "w");
	if (fp)
	{
		if (authServerIP)
			fprintf(fp, "authenticate with %s\n", authServerName);

		m_filePanel.WriteShares(fp);
		m_printerPanel.WritePrinters(fp);

		fclose(fp);
	}

	hManager = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
	if (hManager)
	{
		hService = OpenService(hManager, "BeServed", SERVICE_USER_DEFINED_CONTROL);
		if (hService)
		{
			ControlService(hService, SERVICE_USER_CONTROL_HUP, &status);
			CloseServiceHandle(hService);
		}

		CloseServiceHandle(hManager);
	}

	EndDialog(IDOK);
}
