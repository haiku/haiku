// FileSharingDlg.h : header file
//

#if !defined(AFX_FILESHARINGDLG_H__AF7E12FC_F583_11D5_90F9_00C04F0972A7__INCLUDED_)
#define AFX_FILESHARINGDLG_H__AF7E12FC_F583_11D5_90F9_00C04F0972A7__INCLUDED_

#include "printing.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CFileSharingDlg dialog

class CFileSharingDlg : public CPropertyPage
{
// Construction
public:
	CFileSharingDlg();	// standard constructor

	// Public method used to save changes.
	void WriteShares(FILE *fp);

// Dialog Data
	//{{AFX_DATA(CFileSharingDlg)
	enum { IDD = IDD_FILESHARING_DIALOG };
	CButton	m_removeBtn;
	CButton	m_editBtn;
	CStatic	m_heading;
	CListCtrl	m_shareList;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFileSharingDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;
	HFONT m_hBoldFont;
	CImageList m_images;

	// Generated message map functions
	//{{AFX_MSG(CFileSharingDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnClickShareList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkShareList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnShareEdit();
	afx_msg void OnShareRemove();
	afx_msg void OnShareNew();
	afx_msg void OnSave();
	afx_msg void OnDeploy();
	afx_msg void OnSecurity();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void	InitShares();
	void	GetFileShare(const char *buffer);
	void	GetShareProperty(const char *buffer);
	void	GetGrant(const char *buffer);
	void	GetAuthenticate(const char *buffer);
	void	AddUserRights(char *share, char *user, int rights, bool isGroup);
	void	GetPrinter(const char *buffer);
	int		GetToken();
	char *	GetSharePath(char *shareName);
	int		GetShareId(char *shareName);
	bt_printer *btFindPrinter(char *printerName);
	void	ListShares();
	void	RefreshShare(int nItem);
};


#define WARN_REMOVE_SHARE	"Removing this file share will prevent other users on your network from accessing certain files on this computer.  Are you sure you want to remove this file share?"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FILESHARINGDLG_H__AF7E12FC_F583_11D5_90F9_00C04F0972A7__INCLUDED_)
