// PrinterSharingDlg.h : header file
//

#if !defined(AFX_PRINTERSHARINGDLG_H__2CCAE8D2_2185_42AD_A50D_AEC540DE7434__INCLUDED_)
#define AFX_PRINTERSHARINGDLG_H__2CCAE8D2_2185_42AD_A50D_AEC540DE7434__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "printing.h"


/////////////////////////////////////////////////////////////////////////////
// CPrinterSharingDlg dialog

class CPrinterSharingDlg : public CPropertyPage
{
// Construction
public:
	CPrinterSharingDlg();	// standard constructor

	void WritePrinters(FILE *fp);

// Dialog Data
	//{{AFX_DATA(CPrinterSharingDlg)
	enum { IDD = IDD_PRINTERSHARING_DIALOG };
	CStatic	m_heading;
	CListCtrl	m_printerList;
	CButton	m_removeBtn;
	CButton	m_editBtn;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPrinterSharingDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void ListPrinters();
	void RefreshPrinter(int nItem);

	void initPrinters();
	void getGrant(const char *buffer);
	void getAuthenticate(const char *buffer);
	void addUserRights(char *share, char *user, int rights, bool isGroup);
	void getPrinter(const char *buffer);
	int getToken();
	bt_printer *btFindPrinter(char *printerName);

	CImageList m_images;
	HICON m_hIcon;
	HFONT m_hBoldFont;

	// Generated message map functions
	//{{AFX_MSG(CPrinterSharingDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnClickPrinters(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkPrinters(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEditPrinter();
	afx_msg void OnRemovePrinter();
	afx_msg void OnNewPrinter();
	afx_msg void OnSecurity();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#define WARN_REMOVE_PRINTER	"Removing this printer will prevent other users on your network from using it.  Are you sure you want to remove this printer?"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PRINTERSHARINGDLG_H__2CCAE8D2_2185_42AD_A50D_AEC540DE7434__INCLUDED_)
