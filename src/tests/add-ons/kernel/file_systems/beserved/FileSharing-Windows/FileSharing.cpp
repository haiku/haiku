// FileSharing.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "printing.h"
#include "FileSharing.h"
#include "FolderView.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

bt_fileShare_t fileShares[128];
bt_printer sharedPrinters[BT_MAX_PRINTER_SHARES];
char authServerName[B_OS_NAME_LENGTH + 1];
unsigned int authServerIP;

/////////////////////////////////////////////////////////////////////////////
// CFileSharingApp

BEGIN_MESSAGE_MAP(CFileSharingApp, CWinApp)
	//{{AFX_MSG_MAP(CFileSharingApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFileSharingApp construction

CFileSharingApp::CFileSharingApp()
{
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CFileSharingApp object

CFileSharingApp theApp;

BOOL CFileSharingApp::InitInstance()
{
	WSADATA wsaData;
	AfxEnableControlContainer();
	WSAStartup(MAKEWORD(1, 1), &wsaData);
	return CWinApp::InitInstance();
}

int CFileSharingApp::ExitInstance()
{
	WSACleanup();
	return CWinApp::ExitInstance();
}


// ----- Library routine for font generation --------------------------------

HFONT InitializeControlFont(char *fontName, int fontWeight, int fontSize)
{
	CDC *dc;
	HFONT hFnt;
	NPLOGFONT plf = (NPLOGFONT) LocalAlloc(LPTR, sizeof(LOGFONT));

	dc = CWnd::GetDesktopWindow()->GetDC();

	plf->lfHeight = -MulDiv(fontSize, dc->GetDeviceCaps(LOGPIXELSY), 72);
	plf->lfWeight = fontWeight;
	lstrcpy((LPSTR) plf->lfFaceName, fontName);
	hFnt = CreateFontIndirect(plf);
	LocalFree((LOCALHANDLE) plf);

	CWnd::GetDesktopWindow()->ReleaseDC(dc);
	return (hFnt);
}

// GetSelectedListItem()
//
int GetSelectedListItem(CListCtrl *pListCtrl)
{
	int i;

	// Find the first selected document in the list.
	for (i = 0; i < pListCtrl->GetItemCount(); i++)
		if (pListCtrl->GetItemState(i, LVIS_SELECTED) != 0)
			break;

	// If none could be found, what are we doing here?
	if (i >= pListCtrl->GetItemCount())
		i = -1;

	return (i);
}

// PrintString()
//
void PrintString(FILE *fp, const char *str)
{
	if (strchr(str, ' '))
		fprintf(fp, "\"%s\"", str);
	else
		fprintf(fp, "%s", str);
}
