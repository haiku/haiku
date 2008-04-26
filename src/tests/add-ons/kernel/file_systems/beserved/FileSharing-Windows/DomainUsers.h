#if !defined(AFX_DOMAINUSERS_H__6F5ED7A2_9C16_11D6_9EF3_00A0C965AF06__INCLUDED_)
#define AFX_DOMAINUSERS_H__6F5ED7A2_9C16_11D6_9EF3_00A0C965AF06__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DomainUsers.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDomainUsers dialog

class CDomainUsers : public CDialog
{
// Construction
public:
	CDomainUsers(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDomainUsers)
	enum { IDD = IDD_DOMAIN_USERS };
	CButton	m_okayBtn;
	CListCtrl	m_userList;
	CComboBox	m_rights;
	//}}AFX_DATA

	int ShowUsers(bool bPrinting);
	void AddUserList();
	void AddGroupList();

	CString GetUser()			{ return m_user; }
	int GetRights()				{ return m_nRights; }
	bool IsGroup()				{ return m_bGroup; }
	void SetPrinting(bool b)	{ m_bPrinting = b; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDomainUsers)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDomainUsers)
	virtual BOOL OnInitDialog();
	afx_msg void OnClickDomainUsers(NMHDR* pNMHDR, LRESULT* pResult);
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CImageList m_images;
	CString m_user;
	int m_nRights;
	bool m_bPrinting;
	bool m_bGroup;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DOMAINUSERS_H__6F5ED7A2_9C16_11D6_9EF3_00A0C965AF06__INCLUDED_)
