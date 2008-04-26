// ShareProperties.cpp : implementation file
//

#include "stdafx.h"

#include "FileSharing.h"
#include "ShareProperties.h"
#include "BrowseFolders.h"
#include "DomainUsers.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern bt_fileShare_t fileShares[];
extern unsigned int authServerIP;


/////////////////////////////////////////////////////////////////////////////
// CShareProperties dialog


CShareProperties::CShareProperties(CWnd* pParent /*=NULL*/)
	: CDialog(CShareProperties::IDD, pParent)
{
	//{{AFX_DATA_INIT(CShareProperties)
	m_bLinks = FALSE;
	m_bReadWrite = FALSE;
	m_shareName = _T("");
	m_sharePath = _T("");
	//}}AFX_DATA_INIT
}


void CShareProperties::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CShareProperties)
	DDX_Control(pDX, IDC_SHARE_ADD, m_addBtn);
	DDX_Control(pDX, IDC_SHARE_REMOVE, m_removeBtn);
	DDX_Control(pDX, IDC_DOMAIN_USERS, m_userList);
	DDX_Control(pDX, IDC_DOMAIN_HEADING, m_domainHeading);
	DDX_Control(pDX, IDC_OPTIONS_HEADING, m_optionsHeading);
	DDX_Control(pDX, IDC_SHARE_NAME, m_nameCtrl);
	DDX_Control(pDX, IDOK, m_okayBtn);
	DDX_Control(pDX, IDC_SHARE_HEADING, m_heading);
	DDX_Check(pDX, IDC_SHARE_LINKS, m_bLinks);
	DDX_Check(pDX, IDC_SHARE_WRITE, m_bReadWrite);
	DDX_Text(pDX, IDC_SHARE_NAME, m_shareName);
	DDX_Text(pDX, IDC_SHARE_PATH, m_sharePath);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CShareProperties, CDialog)
	//{{AFX_MSG_MAP(CShareProperties)
	ON_BN_CLICKED(IDC_SHARE_BROWSE, OnShareBrowse)
	ON_EN_CHANGE(IDC_SHARE_NAME, OnChangeShareName)
	ON_EN_CHANGE(IDC_SHARE_PATH, OnChangeSharePath)
	ON_NOTIFY(NM_CLICK, IDC_DOMAIN_USERS, OnClickDomainUsers)
	ON_BN_CLICKED(IDC_SHARE_ADD, OnShareAdd)
	ON_BN_CLICKED(IDC_SHARE_REMOVE, OnShareRemove)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CShareProperties message handlers

BOOL CShareProperties::OnInitDialog() 
{
	m_shareName = fileShares[m_nIndex].name;
	m_sharePath = fileShares[m_nIndex].path;
	m_bReadWrite = !fileShares[m_nIndex].readOnly;
	m_bLinks = FALSE;

	CDialog::OnInitDialog();
	SetWindowText(fileShares[m_nIndex].name);
	m_heading.SetWindowText(fileShares[m_nIndex].name);

	m_hBoldFont = InitializeControlFont("Tahoma", FW_BOLD, 8);
	m_heading.SendMessage(WM_SETFONT, (WPARAM) m_hBoldFont, MAKELPARAM(TRUE, 0));
	m_optionsHeading.SendMessage(WM_SETFONT, (WPARAM) m_hBoldFont, MAKELPARAM(TRUE, 0));
	m_domainHeading.SendMessage(WM_SETFONT, (WPARAM) m_hBoldFont, MAKELPARAM(TRUE, 0));

	m_userList.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	m_userList.InsertColumn(0, "User or Group", LVCFMT_LEFT, 175);
	m_userList.InsertColumn(1, "Access", LVCFMT_LEFT, 100);

	m_nameCtrl.SetSel(0, -1);
	m_nameCtrl.SetFocus();

	m_okayBtn.EnableWindow(!m_shareName.IsEmpty() && !m_sharePath.IsEmpty());
	m_addBtn.EnableWindow(authServerIP != 0);

	CBitmap user, group;
	user.LoadBitmap(IDB_USER);
	group.LoadBitmap(IDB_GROUP);

	m_images.Create(16, 16, FALSE, 2, 0);
	m_images.Add(&user, (COLORREF) 0);
	m_images.Add(&group, (COLORREF) 0);
	m_userList.SetImageList(&m_images, LVSIL_SMALL);

	AddUserList();

	return FALSE;
}

void CShareProperties::OnShareBrowse() 
{
	CBrowseFolders browser;
	CWaitCursor wait;
	if (browser.SelectFolder(m_sharePath) == IDOK)
	{
		m_sharePath = browser.GetPath();
		UpdateData(FALSE);
	}
}

void CShareProperties::OnChangeShareName() 
{
	UpdateData(TRUE);
	m_okayBtn.EnableWindow(!m_shareName.IsEmpty());
}

void CShareProperties::OnChangeSharePath() 
{
	UpdateData(TRUE);
	m_okayBtn.EnableWindow(!m_sharePath.IsEmpty());
}

void CShareProperties::OnOK() 
{
	CDialog::OnOK();

	strcpy(fileShares[m_nIndex].name, m_shareName);
	strcpy(fileShares[m_nIndex].path, m_sharePath);
	fileShares[m_nIndex].readOnly = !m_bReadWrite;
	fileShares[m_nIndex].followLinks = m_bLinks ? 1 : 0;
}

int CShareProperties::ShowProperties(int nIndex)
{
	m_nIndex = nIndex;
	return DoModal();
}

void CShareProperties::AddUserList()
{
	bt_user_rights *ur;
	int nItem;

	if (fileShares[m_nIndex].security != BT_AUTH_NONE)
		for (ur = fileShares[m_nIndex].rights; ur; ur = ur->next)
		{
			char access[50];
			access[0] = 0;
			if (ur->rights & BT_RIGHTS_READ)
				strcat(access, "Read ");
			if (ur->rights & BT_RIGHTS_WRITE)
				strcat(access, "Write");

			nItem = m_userList.InsertItem(0, ur->user, ur->isGroup ? 1 : 0);
			m_userList.SetItemText(nItem, 1, access);
		}
}

void CShareProperties::OnClickDomainUsers(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
	m_removeBtn.EnableWindow(GetSelectedListItem(&m_userList) != -1);
}

void CShareProperties::OnShareAdd() 
{
	CDomainUsers users;
	char access[50];
	int nItem;

	if (users.DoModal() == IDOK)
	{
		CString user = users.GetUser();
		int nRights = users.GetRights();
		bool isGroup = users.IsGroup();

		SaveUserRights(LPCTSTR(user), nRights, isGroup);

		access[0] = 0;
		if (nRights & BT_RIGHTS_READ)
			strcat(access, "Read ");
		if (nRights & BT_RIGHTS_WRITE)
			strcat(access, "Write");

		nItem = m_userList.InsertItem(0, user, isGroup ? 1 : 0);
		m_userList.SetItemText(nItem, 1, access);
	}
}

void CShareProperties::OnShareRemove() 
{
	bt_user_rights *ur, *lastUR;
	int nItem = GetSelectedListItem(&m_userList);
	CString user = m_userList.GetItemText(nItem, 0);

	lastUR = NULL;
	for (ur = fileShares[m_nIndex].rights; ur; ur = ur->next)
		if (strcmp(ur->user, user) == 0)
		{
			if (lastUR)
				lastUR->next = ur->next;
			else
				fileShares[m_nIndex].rights = ur->next;

			free(ur);
			m_userList.DeleteItem(nItem);
			break;
		}
		else
			lastUR = ur;
}

void CShareProperties::SaveUserRights(const char *user, int rights, bool isGroup)
{
	bt_user_rights *ur;

	if (user == NULL || rights == 0)
		return;

	for (ur = fileShares[m_nIndex].rights; ur; ur = ur->next)
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
			ur->next = fileShares[m_nIndex].rights;
			fileShares[m_nIndex].rights = ur;
		}
		else
			free(ur);
	}
}
