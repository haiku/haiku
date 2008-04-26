#if !defined(AFX_SHAREPROPERTIES_H__AF7E1305_F583_11D5_90F9_00C04F0972A7__INCLUDED_)
#define AFX_SHAREPROPERTIES_H__AF7E1305_F583_11D5_90F9_00C04F0972A7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ShareProperties.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CShareProperties dialog

class CShareProperties : public CDialog
{
// Construction
public:
	CShareProperties(CWnd* pParent = NULL);   // standard constructor

	int ShowProperties(int nIndex);

// Dialog Data
	//{{AFX_DATA(CShareProperties)
	enum { IDD = IDD_SHARE_PROPERTIES };
	CButton	m_addBtn;
	CButton	m_removeBtn;
	CListCtrl	m_userList;
	CStatic	m_domainHeading;
	CStatic	m_optionsHeading;
	CEdit	m_nameCtrl;
	CButton	m_okayBtn;
	CStatic	m_heading;
	BOOL	m_bLinks;
	BOOL	m_bReadWrite;
	CString	m_shareName;
	CString	m_sharePath;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CShareProperties)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void	AddUserList();
	void	SaveUserRights(const char *user, int rights, bool isGroup);

	CImageList m_images;
	HFONT m_hBoldFont;

	// Generated message map functions
	//{{AFX_MSG(CShareProperties)
	virtual BOOL OnInitDialog();
	afx_msg void OnShareBrowse();
	afx_msg void OnChangeShareName();
	afx_msg void OnChangeSharePath();
	virtual void OnOK();
	afx_msg void OnClickDomainUsers(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnShareAdd();
	afx_msg void OnShareRemove();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	int m_nIndex;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SHAREPROPERTIES_H__AF7E1305_F583_11D5_90F9_00C04F0972A7__INCLUDED_)
