// PrinterProperties.cpp : implementation file
//

#include "stdafx.h"
#include "winspool.h"

#include "FileSharing.h"
#include "PrinterProperties.h"
#include "BrowseFolders.h"
#include "DomainUsers.h"
#include "ManageQueue.h"
#include "printing.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern bt_printer sharedPrinters[];
extern unsigned int authServerIP;


/////////////////////////////////////////////////////////////////////////////
// CPrinterProperties dialog


CPrinterProperties::CPrinterProperties(CWnd* pParent /*=NULL*/)
	: CDialog(CPrinterProperties::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPrinterProperties)
	m_printerName = _T("");
	m_queuePath = _T("");
	//}}AFX_DATA_INIT
}


void CPrinterProperties::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPrinterProperties)
	DDX_Control(pDX, IDC_PRINTER_TYPE, m_printerType);
	DDX_Control(pDX, IDC_PRINTER_NAME, m_nameCtrl);
	DDX_Control(pDX, IDOK, m_okayBtn);
	DDX_Control(pDX, IDC_PRINTER_REMOVE, m_removeBtn);
	DDX_Control(pDX, IDC_PRINTER_HEADING, m_heading);
	DDX_Control(pDX, IDC_DOMAIN_HEADING, m_domainHeading);
	DDX_Control(pDX, IDC_PRINTER_DEVICE, m_device);
	DDX_Control(pDX, IDC_PRINTER_ADD, m_addBtn);
	DDX_Control(pDX, IDC_DOMAIN_USERS, m_userList);
	DDX_Text(pDX, IDC_PRINTER_NAME, m_printerName);
	DDX_Text(pDX, IDC_QUEUE_PATH, m_queuePath);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_PRINTER_MODEL, m_printerModel);
}


BEGIN_MESSAGE_MAP(CPrinterProperties, CDialog)
	//{{AFX_MSG_MAP(CPrinterProperties)
	ON_NOTIFY(NM_CLICK, IDC_DOMAIN_USERS, OnClickDomainUsers)
	ON_BN_CLICKED(IDC_PRINTER_ADD, OnPrinterAdd)
	ON_BN_CLICKED(IDC_PRINTER_REMOVE, OnPrinterRemove)
	ON_BN_CLICKED(IDC_MANAGE, OnManage)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_PRINTER_BROWSE, OnBnClickedPrinterBrowse)
	ON_CBN_SELCHANGE(IDC_PRINTER_TYPE, OnCbnSelchangePrinterType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPrinterProperties message handlers

int CPrinterProperties::ShowProperties(int nIndex)
{
	m_nIndex = nIndex;
	return DoModal();
}

BOOL CPrinterProperties::OnInitDialog() 
{
	m_printerName = sharedPrinters[m_nIndex].printerName;
	m_queuePath = sharedPrinters[m_nIndex].spoolDir;

	CDialog::OnInitDialog();
	SetWindowText(sharedPrinters[m_nIndex].printerName);
	m_heading.SetWindowText(sharedPrinters[m_nIndex].printerName);

	m_hBoldFont = InitializeControlFont("Tahoma", FW_BOLD, 8);
	m_heading.SendMessage(WM_SETFONT, (WPARAM) m_hBoldFont, MAKELPARAM(TRUE, 0));
	m_domainHeading.SendMessage(WM_SETFONT, (WPARAM) m_hBoldFont, MAKELPARAM(TRUE, 0));

	m_userList.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	m_userList.InsertColumn(0, "User or Group", LVCFMT_LEFT, 175);
	m_userList.InsertColumn(1, "Access", LVCFMT_LEFT, 100);

	m_nameCtrl.SetSel(0, -1);
	m_nameCtrl.SetFocus();

	m_okayBtn.EnableWindow(!m_printerName.IsEmpty() && !m_queuePath.IsEmpty());
	m_addBtn.EnableWindow(authServerIP != 0);

	CBitmap user, group;
	user.LoadBitmap(IDB_USER);
	group.LoadBitmap(IDB_GROUP);

	m_images.Create(16, 16, FALSE, 2, 0);
	m_images.Add(&user, (COLORREF) 0);
	m_images.Add(&group, (COLORREF) 0);
	m_userList.SetImageList(&m_images, LVSIL_SMALL);

	AddUserList();
	BuildPrinterList();
	m_device.SelectString(-1, sharedPrinters[m_nIndex].deviceName);

	m_printerType.AddString(PRT_TYPE_LASERJET);
	m_printerType.AddString(PRT_TYPE_INKJET);
	m_printerType.AddString(PRT_TYPE_POSTSCRIPT);
	m_printerType.AddString(PRT_TYPE_EPSON_STYLUS);

	if (m_printerType.SelectString(-1, sharedPrinters[m_nIndex].deviceType) == CB_ERR)
		m_printerType.SelectString(-1, PRT_TYPE_LASERJET);

	OnCbnSelchangePrinterType();
	return FALSE;
}

void CPrinterProperties::AddUserList()
{
	bt_user_rights *ur;
	int nItem;

	if (sharedPrinters[m_nIndex].security != BT_AUTH_NONE)
		for (ur = sharedPrinters[m_nIndex].rights; ur; ur = ur->next)
		{
			char access[50];
			access[0] = 0;
			if (ur->rights & BT_RIGHTS_PRINT)
				strcpy(access, "Print");

			nItem = m_userList.InsertItem(0, ur->user, ur->isGroup ? 1 : 0);
			m_userList.SetItemText(nItem, 1, access);
		}
}

void CPrinterProperties::BuildPrinterList()
{
	PRINTER_INFO_1 netPrinters[BT_MAX_PRINTER_SHARES];
	PRINTER_INFO_5 printers[BT_MAX_PRINTER_SHARES];
	DWORD bytesCopied, structsCopied;
	int i;

	if (EnumPrinters(PRINTER_ENUM_NAME, NULL, 5, (LPBYTE) printers, sizeof(printers), &bytesCopied, &structsCopied))
		for (i = 0; i < structsCopied; i++)
			m_device.AddString(printers[i].pPrinterName);

	if (EnumPrinters(PRINTER_ENUM_CONNECTIONS, NULL, 1, (LPBYTE) netPrinters, sizeof(netPrinters), &bytesCopied, &structsCopied))
		for (i = 0; i < structsCopied; i++)
			m_device.AddString(netPrinters[i].pName);
}

void CPrinterProperties::OnClickDomainUsers(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
	m_removeBtn.EnableWindow(GetSelectedListItem(&m_userList) != -1);
}

void CPrinterProperties::OnPrinterAdd() 
{
	CDomainUsers users;
	char access[50];
	int nItem;

	if (users.ShowUsers(true) == IDOK)
	{
		CString user = users.GetUser();
		int nRights = users.GetRights();
		bool isGroup = users.IsGroup();

		SaveUserRights(LPCTSTR(user), nRights, isGroup);

		access[0] = 0;
		if (nRights & BT_RIGHTS_PRINT)
			strcpy(access, "Print");

		nItem = m_userList.InsertItem(0, user, isGroup ? 1 : 0);
		m_userList.SetItemText(nItem, 1, access);
	}
}

void CPrinterProperties::OnPrinterRemove() 
{
	bt_user_rights *ur, *lastUR;
	int nItem = GetSelectedListItem(&m_userList);
	CString user = m_userList.GetItemText(nItem, 0);

	lastUR = NULL;
	for (ur = sharedPrinters[m_nIndex].rights; ur; ur = ur->next)
		if (strcmp(ur->user, user) == 0)
		{
			if (lastUR)
				lastUR->next = ur->next;
			else
				sharedPrinters[m_nIndex].rights = ur->next;

			free(ur);
			m_userList.DeleteItem(nItem);
			break;
		}
		else
			lastUR = ur;
}

void CPrinterProperties::SaveUserRights(const char *user, int rights, bool isGroup)
{
	bt_user_rights *ur;

	if (user == NULL || rights == 0)
		return;

	for (ur = sharedPrinters[m_nIndex].rights; ur; ur = ur->next)
		if (strcmp(ur->user, user) == 0)
		{
			ur->rights = rights;
			return;
		}

	ur = (bt_user_rights *) malloc(sizeof(bt_user_rights));
	if (ur)
	{
		ur->user = (char *) malloc(strlen(user) + 1);
		if (ur->user)
		{
			strcpy(ur->user, user);
			ur->rights = rights;
			ur->isGroup = isGroup;
			ur->next = sharedPrinters[m_nIndex].rights;
			sharedPrinters[m_nIndex].rights = ur;
		}
		else
			free(ur);
	}
}

void CPrinterProperties::OnOK() 
{
	CString itemName;
	int item;

	CDialog::OnOK();

	strcpy(sharedPrinters[m_nIndex].printerName, m_printerName);
	strcpy(sharedPrinters[m_nIndex].spoolDir, m_queuePath);

	item = m_device.GetCurSel();
	m_device.GetLBText(item, itemName);
	strcpy(sharedPrinters[m_nIndex].deviceName, itemName);

	item = m_printerType.GetCurSel();
	m_printerType.GetLBText(item, itemName);
	strcpy(sharedPrinters[m_nIndex].deviceType, itemName);
}

void CPrinterProperties::OnManage() 
{
	CManageQueue queueDlg;
	queueDlg.ShowQueue(&sharedPrinters[m_nIndex]);
}

void CPrinterProperties::OnBnClickedPrinterBrowse()
{
	CBrowseFolders browser;
	CWaitCursor wait;
	if (browser.SelectFolder(m_queuePath) == IDOK)
	{
		m_queuePath = browser.GetPath();
		UpdateData(FALSE);
	}
}

void CPrinterProperties::OnCbnSelchangePrinterType()
{
	CString itemName;
	int item;

	UpdateData(TRUE);
	m_printerModel.ResetContent();

	item = m_printerType.GetCurSel();
	m_printerType.GetLBText(item, itemName);
	if (itemName.CompareNoCase(PRT_TYPE_LASERJET) == 0)
		m_printerModel.EnableWindow(FALSE);
	else
	{
		m_printerModel.EnableWindow(TRUE);

	}
}
