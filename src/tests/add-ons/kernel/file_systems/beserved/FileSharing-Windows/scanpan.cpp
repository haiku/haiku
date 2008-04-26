// ScanPan.cpp

#include "stdafx.h"
#include "FileSharing.h"
#include "MainPanel.h"

LONG CScanPanel::OnInquire(UINT uAppNum,NEWCPLINFO *pInfo)
{
	// Fill in the data
	pInfo->dwSize = sizeof(NEWCPLINFO); // important
	pInfo->dwFlags = 0;
	pInfo->dwHelpContext = 0;
	pInfo->lData = 0;
	pInfo->hIcon = ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME));
	strcpy(pInfo->szName, "BeServed File Sharing");
	strcpy(pInfo->szInfo, "BeServed");
	strcpy(pInfo->szHelpFile,"");
	return 0; // OK (don't send CPL_INQUIRE msg)
}

LONG CScanPanel::OnDblclk(HWND hwndCPl,UINT uAppNum,LONG lData)
{
	// Create the dialog box using the parent window handle
	CMainPanel dlg(CWnd::FromHandle(hwndCPl));
	dlg.DoModal();
	return(0);
}
