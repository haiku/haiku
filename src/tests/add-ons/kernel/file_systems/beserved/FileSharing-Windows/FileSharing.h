// scanDlg.h : header file
//

#if !defined(AFX_FileSharing_H__INCLUDED_)
#define AFX_FileSharing_H__INCLUDED_

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif
#ifdef IDC_STATIC
#undef IDC_STATIC
#endif
#include "resource.h"
#include "ScanPan.h"
#include "FileSharingDlg.h"

#ifndef _BETALK_H_
#include "beCompat.h"
#include "betalk.h"
#endif


#define iswhite(c)				((c)==' ' || (c)=='\t')

#define BT_MAX_FILE_SHARES		128
#define BT_MAX_PRINTER_SHARES	16

typedef struct fileShare
{
	char path[B_PATH_NAME_LENGTH];
	char name[B_FILE_NAME_LENGTH];
	bool used;
	bool readOnly;
	bool followLinks;
	bt_user_rights *rights;
	int security;
	struct fileShare *next;
} bt_fileShare_t;

HFONT InitializeControlFont(char *fontName, int fontWeight, int fontSize);
int GetSelectedListItem(CListCtrl *pListCtrl);
void PrintString(FILE *fp, const char *str);

/////////////////////////////////////////////////////////////////////////////
// CFileSharingApp
// See FileSharing.cpp for the implementation of this class
//

class CFileSharingApp : public CWinApp
{
public:
	CFileSharingApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFileSharingApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CFileSharingApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CScanPanel	m_Control;
	virtual BOOL InitInstance();
	virtual int ExitInstance();
};

/////////////////////////////////////////////////////////////////////////////

#endif // !defined(AFX_SCANDLG_H__INCLUDED_)
