// PrinterSharingDlg.cpp : implementation file
//

#include "stdafx.h"

#include "ctype.h"
#include "sys/types.h"
#include "sys/stat.h"

#include "becompat.h"
#include "betalk.h"

#include "FileSharing.h"
#include "PrinterSharingDlg.h"
#include "PrinterProperties.h"
#include "printing.h"
#include "Security.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern bt_printer sharedPrinters[];
extern char authServerName[];
extern unsigned int authServerIP;

extern char tokBuffer[MAX_NAME_LENGTH + 1], *tokPtr;
extern char *keywords[];


/////////////////////////////////////////////////////////////////////////////
// CPrinterSharingDlg dialog

CPrinterSharingDlg::CPrinterSharingDlg()
	: CPropertyPage(CPrinterSharingDlg::IDD)
{
	//{{AFX_DATA_INIT(CPrinterSharingDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CPrinterSharingDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPrinterSharingDlg)
	DDX_Control(pDX, IDC_MAIN_HEADING, m_heading);
	DDX_Control(pDX, IDC_PRINTERS, m_printerList);
	DDX_Control(pDX, IDC_REMOVE_PRINTER, m_removeBtn);
	DDX_Control(pDX, IDC_EDIT_PRINTER, m_editBtn);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPrinterSharingDlg, CPropertyPage)
	//{{AFX_MSG_MAP(CPrinterSharingDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(NM_CLICK, IDC_PRINTERS, OnClickPrinters)
	ON_NOTIFY(NM_DBLCLK, IDC_PRINTERS, OnDblclkPrinters)
	ON_BN_CLICKED(IDC_EDIT_PRINTER, OnEditPrinter)
	ON_BN_CLICKED(IDC_REMOVE_PRINTER, OnRemovePrinter)
	ON_BN_CLICKED(IDC_NEW_PRINTER, OnNewPrinter)
	ON_BN_CLICKED(IDC_SECURITY, OnSecurity)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPrinterSharingDlg message handlers

BOOL CPrinterSharingDlg::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_editBtn.EnableWindow(FALSE);
	m_removeBtn.EnableWindow(FALSE);

	m_printerList.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	m_printerList.InsertColumn(0, "Printer Name", LVCFMT_LEFT, 120);
	m_printerList.InsertColumn(1, "Local Device", LVCFMT_LEFT, 120);
	m_printerList.InsertColumn(2, "Queue Path", LVCFMT_LEFT, 200);

	CBitmap printer;
	printer.LoadBitmap(IDB_USER);

	m_images.Create(16, 16, FALSE, 2, 0);
	m_images.Add(&printer, (COLORREF) 0);
	m_printerList.SetImageList(&m_images, LVSIL_SMALL);

	m_hBoldFont = InitializeControlFont("Tahoma", FW_BOLD, 8);
	m_heading.SendMessage(WM_SETFONT, (WPARAM) m_hBoldFont, MAKELPARAM(TRUE, 0));

	ListPrinters();

	return TRUE;
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CPrinterSharingDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CPropertyPage::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CPrinterSharingDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CPrinterSharingDlg::OnClickPrinters(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
	m_editBtn.EnableWindow(GetSelectedListItem(&m_printerList) != -1);
	m_removeBtn.EnableWindow(GetSelectedListItem(&m_printerList) != -1);
}

void CPrinterSharingDlg::OnDblclkPrinters(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
	OnEditPrinter();
}

void CPrinterSharingDlg::OnEditPrinter() 
{
	CPrinterProperties printer;
	int nItem = GetSelectedListItem(&m_printerList);
	if (nItem != -1)
		if (printer.ShowProperties(m_printerList.GetItemData(nItem)) == IDOK)
			RefreshPrinter(nItem);
}

void CPrinterSharingDlg::OnRemovePrinter() 
{
	int nItem = GetSelectedListItem(&m_printerList);
	if (nItem >= 0)
		if (MessageBox(WARN_REMOVE_PRINTER, sharedPrinters[nItem].printerName, MB_YESNO | MB_ICONQUESTION) == IDYES)
		{
			sharedPrinters[nItem].used = false;
			m_printerList.DeleteItem(nItem);
		}
}

void CPrinterSharingDlg::OnNewPrinter() 
{
	CPrinterProperties printer;
	struct stat st;
	int nIndex;

	for (nIndex = 0; nIndex < BT_MAX_PRINTER_SHARES; nIndex++)
		if (!sharedPrinters[nIndex].used)
			break;

	if (nIndex == BT_MAX_PRINTER_SHARES)
		return;

	strcpy(sharedPrinters[nIndex].printerName, "Untitled");
	strcpy(sharedPrinters[nIndex].spoolDir, "c:\\temp");
	if (printer.ShowProperties(nIndex) == IDOK)
	{
		int nItem = m_printerList.InsertItem(0, sharedPrinters[nIndex].printerName, 0);
		m_printerList.SetItemText(nItem, 1, sharedPrinters[nIndex].deviceName);
		m_printerList.SetItemText(nItem, 2, stat(sharedPrinters[nIndex].spoolDir, &st) == 0 ? sharedPrinters[nIndex].spoolDir : "--- Invalid Path ---");
		m_printerList.SetItemData(nItem, nIndex);
		sharedPrinters[nIndex].used = true;
	}
}

void CPrinterSharingDlg::OnSecurity() 
{
	CSecurity security;
	security.DoModal();
}

// ListPrinters()
//
void CPrinterSharingDlg::ListPrinters()
{
	struct stat st;
	int i, nItem;

	for (i = 0; i < BT_MAX_PRINTER_SHARES; i++)
		if (sharedPrinters[i].used)
		{
			nItem = m_printerList.InsertItem(0, sharedPrinters[i].printerName, 0);
			m_printerList.SetItemText(nItem, 1, sharedPrinters[i].deviceName);
			m_printerList.SetItemText(nItem, 2, stat(sharedPrinters[i].spoolDir, &st) == 0 ? sharedPrinters[i].spoolDir : "--- Invalid Path ---");
			m_printerList.SetItemData(nItem, i);
		}
}

void CPrinterSharingDlg::RefreshPrinter(int nItem)
{
	struct stat st;
	int nIndex = m_printerList.GetItemData(nItem);
	m_printerList.SetItemText(nItem, 0, sharedPrinters[nItem].printerName);
	m_printerList.SetItemText(nItem, 1, sharedPrinters[nItem].deviceName);
	m_printerList.SetItemText(nItem, 2, stat(sharedPrinters[nItem].spoolDir, &st) == 0 ? sharedPrinters[nItem].spoolDir : "--- Invalid Path ---");
}

// WritePrinters()
//
void CPrinterSharingDlg::WritePrinters(FILE *fp)
{
	bt_user_rights *ur;
	int i;

	if (fp)
	{
		for (i = 0; i < BT_MAX_PRINTER_SHARES; i++)
			if (sharedPrinters[i].used)
			{
				fprintf(fp, "printer ");
				PrintString(fp, sharedPrinters[i].printerName);
				fprintf(fp, " is type ");
				PrintString(fp, sharedPrinters[i].deviceType);
				fprintf(fp, " device ");
				PrintString(fp, sharedPrinters[i].deviceName);
				fprintf(fp, " spooled to ");
				PrintString(fp, sharedPrinters[i].spoolDir);
				fputc('\n', fp);

				for (ur = sharedPrinters[i].rights; ur; ur = ur->next)
					if (ur->rights & BT_RIGHTS_PRINT)
					{
						fprintf(fp, "grant print on ");
						PrintString(fp, sharedPrinters[i].printerName);
						fprintf(fp, " to ");
						if (ur->isGroup)
							fprintf(fp, "group ");
						fprintf(fp, "%s\n", ur->user);
					}
			}
	}
}
