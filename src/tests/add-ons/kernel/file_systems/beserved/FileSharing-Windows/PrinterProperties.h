#include "afxwin.h"
#if !defined(AFX_PRINTERPROPERTIES_H__1ED970A1_8FE9_431D_9F26_5E797DCB8C62__INCLUDED_)
#define AFX_PRINTERPROPERTIES_H__1ED970A1_8FE9_431D_9F26_5E797DCB8C62__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PrinterProperties.h : header file
//

#define PRT_TYPE_LASERJET		"HP LaserJet Compatible"
#define PRT_TYPE_INKJET			"HP InkJet Compatible"
#define PRT_TYPE_POSTSCRIPT		"Postscript Printer"
#define PRT_TYPE_EPSON_STYLUS	"Epson Stylus"


/////////////////////////////////////////////////////////////////////////////
// CPrinterProperties dialog

class CPrinterProperties : public CDialog
{
// Construction
public:
	CPrinterProperties(CWnd* pParent = NULL);   // standard constructor
	int ShowProperties(int nIndex);

// Dialog Data
	//{{AFX_DATA(CPrinterProperties)
	enum { IDD = IDD_PRINTER_PROPERTIES };
	CComboBox	m_printerType;
	CEdit	m_nameCtrl;
	CButton	m_removeBtn;
	CButton m_okayBtn;
	CStatic	m_heading;
	CStatic	m_domainHeading;
	CComboBox	m_device;
	CButton	m_addBtn;
	CListCtrl	m_userList;
	CString	m_printerName;
	CString	m_queuePath;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPrinterProperties)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void AddUserList();
	void BuildPrinterList();
	void SaveUserRights(const char *user, int rights, bool isGroup);

	CImageList m_images;
	HFONT m_hBoldFont;
	int m_nIndex;

	// Generated message map functions
	//{{AFX_MSG(CPrinterProperties)
	virtual BOOL OnInitDialog();
	afx_msg void OnClickDomainUsers(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnPrinterAdd();
	afx_msg void OnPrinterRemove();
	afx_msg void OnManage();
	afx_msg void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedPrinterBrowse();
	afx_msg void OnCbnSelchangePrinterType();
	CComboBox m_printerModel;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PRINTERPROPERTIES_H__1ED970A1_8FE9_431D_9F26_5E797DCB8C62__INCLUDED_)
