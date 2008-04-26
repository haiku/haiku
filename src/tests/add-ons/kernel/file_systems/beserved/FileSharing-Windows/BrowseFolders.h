//{{AFX_INCLUDES()
#include "folderview.h"
//}}AFX_INCLUDES
#if !defined(AFX_BROWSEFOLDERS_H__60DF2AD1_F62B_11D5_90FA_00C04F0972A7__INCLUDED_)
#define AFX_BROWSEFOLDERS_H__60DF2AD1_F62B_11D5_90FA_00C04F0972A7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BrowseFolders.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CBrowseFolders dialog

class CBrowseFolders : public CDialog
{
// Construction
public:
	CBrowseFolders(CWnd* pParent = NULL);   // standard constructor

	int SelectFolder(LPCTSTR lpszPath);
	CString GetPath();

// Dialog Data
	//{{AFX_DATA(CBrowseFolders)
	enum { IDD = IDD_BROWSER };
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBrowseFolders)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CString m_newPath;

	// Generated message map functions
	//{{AFX_MSG(CBrowseFolders)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void SetPath(LPCTSTR lpszPath);
public:
	CFolderview m_folderCtrl;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BROWSEFOLDERS_H__60DF2AD1_F62B_11D5_90FA_00C04F0972A7__INCLUDED_)
