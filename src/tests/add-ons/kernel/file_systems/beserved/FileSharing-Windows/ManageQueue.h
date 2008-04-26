#if !defined(AFX_MANAGEQUEUE_H__CF35E7B7_4D1E_4FC2_AA9C_5308F6334777__INCLUDED_)
#define AFX_MANAGEQUEUE_H__CF35E7B7_4D1E_4FC2_AA9C_5308F6334777__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "printing.h"

// ManageQueue.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// CManageQueue dialog

class CManageQueue : public CDialog
{
// Construction
public:
	CManageQueue(CWnd* pParent = NULL);   // standard constructor
	int ShowQueue(bt_printer *printer);

// Dialog Data
	//{{AFX_DATA(CManageQueue)
	enum { IDD = IDD_MANAGE_QUEUE };
	CButton	m_pauseBtn;
	CButton	m_removeBtn;
	CListCtrl	m_queue;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CManageQueue)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void BuildJobList();
	bt_print_job *AddPrintJob(bt_printer *printer, char *path);

	bt_printer *printer;
	bt_print_job *rootJob;

	// Generated message map functions
	//{{AFX_MSG(CManageQueue)
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnClickQueueList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnPauseJob();
	afx_msg void OnRemoveJob();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MANAGEQUEUE_H__CF35E7B7_4D1E_4FC2_AA9C_5308F6334777__INCLUDED_)
