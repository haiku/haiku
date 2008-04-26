#if !defined(AFX_SECURITY_H__6F5ED7A3_9C16_11D6_9EF3_00A0C965AF06__INCLUDED_)
#define AFX_SECURITY_H__6F5ED7A3_9C16_11D6_9EF3_00A0C965AF06__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Security.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSecurity dialog

class CSecurity : public CDialog
{
// Construction
public:
	CSecurity(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSecurity)
	enum { IDD = IDD_SECURITY };
	CButton	m_okayBtn;
	CComboBox	m_type;
	CEdit	m_serverCtrl;
	CString	m_server;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSecurity)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSecurity)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeSecurityServer();
	afx_msg void OnSelchangeSecurityType();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SECURITY_H__6F5ED7A3_9C16_11D6_9EF3_00A0C965AF06__INCLUDED_)
