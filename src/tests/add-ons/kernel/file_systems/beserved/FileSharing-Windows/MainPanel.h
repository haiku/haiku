#pragma once

#include "FileSharingDlg.h"
#include "PrinterSharingDlg.h"

// CMainPanel property sheet

class CMainPanel : public CPropertySheet
{
	DECLARE_DYNAMIC(CMainPanel)

public:
	CMainPanel(CWnd* pParent = NULL);   // standard constructor
	virtual ~CMainPanel();

// Dialog Data
	enum { IDD = IDD_MAINPANEL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	CFileSharingDlg m_filePanel;
	CPrinterSharingDlg m_printerPanel;

public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
};
