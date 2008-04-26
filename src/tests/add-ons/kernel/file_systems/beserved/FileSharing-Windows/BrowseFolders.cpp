// BrowseFolders.cpp : implementation file
//

#include "stdafx.h"

#include "FileSharing.h"
#include "BrowseFolders.h"
#include "MTVFolder.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBrowseFolders dialog


CBrowseFolders::CBrowseFolders(CWnd* pParent /*=NULL*/)
	: CDialog(CBrowseFolders::IDD, pParent)
{
	//{{AFX_DATA_INIT(CBrowseFolders)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_newPath.Empty();
}


void CBrowseFolders::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBrowseFolders)
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_FOLDERVIEW, m_folderCtrl);
}


BEGIN_MESSAGE_MAP(CBrowseFolders, CDialog)
	//{{AFX_MSG_MAP(CBrowseFolders)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBrowseFolders message handlers

BOOL CBrowseFolders::OnInitDialog() 
{
	CDialog::OnInitDialog();
	SetPath(m_newPath);
	return TRUE;
}

// SelectFolder()
//
int CBrowseFolders::SelectFolder(LPCTSTR lpszPath)
{
	m_newPath = lpszPath;
	return DoModal();
}

// SetPath()
//
void CBrowseFolders::SetPath(LPCTSTR lpszPath)
{
	CString string(lpszPath);
	VARIANT variant;
	BSTR bstr = string.AllocSysString();

	variant.vt = VT_BSTR;
	variant.bstrVal = bstr;

	m_folderCtrl.put_SelectedFolder(variant);
}

// GetPath()
//
CString CBrowseFolders::GetPath()
{
	if (IsWindow(m_folderCtrl.GetSafeHwnd()))
	{
		VARIANT var = m_folderCtrl.get_SelectedFolder();
		CMTVFolder folder(var.pdispVal);
		return folder.GetPathName();
	}

	return m_newPath;
}

// OnOK()
//
void CBrowseFolders::OnOK() 
{
	VARIANT var = m_folderCtrl.get_SelectedFolder();
	CMTVFolder folder(var.pdispVal);

	m_newPath = folder.GetPathName();
	m_newPath.MakeLower();

	CDialog::OnOK();
}
