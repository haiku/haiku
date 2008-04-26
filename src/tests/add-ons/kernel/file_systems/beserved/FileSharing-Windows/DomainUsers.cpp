// DomainUsers.cpp : implementation file
//

#include "stdafx.h"

#include "beCompat.h"
#include "betalk.h"
#include "FileSharing.h"
#include "DomainUsers.h"
#include "rpc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDomainUsers dialog


CDomainUsers::CDomainUsers(CWnd* pParent /*=NULL*/)
	: CDialog(CDomainUsers::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDomainUsers)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_nRights = 0;
	m_user.Empty();
	m_bGroup = false;
	m_bPrinting = false;
}


void CDomainUsers::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDomainUsers)
	DDX_Control(pDX, IDOK, m_okayBtn);
	DDX_Control(pDX, IDC_DOMAIN_USERS, m_userList);
	DDX_Control(pDX, IDC_DOMAIN_RIGHTS, m_rights);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDomainUsers, CDialog)
	//{{AFX_MSG_MAP(CDomainUsers)
	ON_NOTIFY(NM_CLICK, IDC_DOMAIN_USERS, OnClickDomainUsers)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDomainUsers message handlers

int CDomainUsers::ShowUsers(bool bPrinting)
{
	SetPrinting(bPrinting);
	return DoModal();
}

BOOL CDomainUsers::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_userList.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	m_userList.InsertColumn(0, "User or Group", LVCFMT_LEFT, 175);
	m_userList.InsertColumn(1, "Full Name", LVCFMT_LEFT, 185);

	CBitmap user, group;
	user.LoadBitmap(IDB_USER);
	group.LoadBitmap(IDB_GROUP);

	m_images.Create(16, 16, FALSE, 2, 0);
	m_images.Add(&user, (COLORREF) 0);
	m_images.Add(&group, (COLORREF) 0);
	m_userList.SetImageList(&m_images, LVSIL_SMALL);

	if (m_bPrinting)
	{
		m_rights.ResetContent();
		m_rights.AddString("Print");
	}

	AddUserList();
	AddGroupList();

	return TRUE;
}

void CDomainUsers::OnClickDomainUsers(NMHDR* pNMHDR, LRESULT* pResult) 
{
	int nIndex;
	*pResult = 0;
	nIndex = GetSelectedListItem(&m_userList);
	if (nIndex >= 0)
	{
		m_rights.EnableWindow(TRUE);
		m_rights.SelectString(-1, m_bPrinting ? "Print" : "Read-only");
		m_okayBtn.EnableWindow(TRUE);
	}
	else
	{
		m_rights.EnableWindow(FALSE);
		m_okayBtn.EnableWindow(FALSE);
	}
}

void CDomainUsers::OnOK() 
{
	CString rights;
	int nItem = GetSelectedListItem(&m_userList);
	m_user = m_userList.GetItemText(nItem, 0);

	m_bGroup = m_userList.GetItemData(nItem) ? true : false;

	m_nRights = m_bPrinting ? BT_RIGHTS_PRINT : BT_RIGHTS_READ;
	nItem = m_rights.GetCurSel();
	m_rights.GetLBText(nItem, rights);
	if (rights.CompareNoCase("Read-write") == 0)
		m_nRights |= BT_RIGHTS_WRITE;

	CDialog::OnOK();
}

void CDomainUsers::AddUserList()
{
	extern unsigned int authServerIP;
	int32 *dir = NULL;
	char user[MAX_USERNAME_LENGTH], fullName[MAX_DESC_LENGTH];
	int nIndex;

	bt_outPacket *outPacket = btRPCPutHeader(BT_CMD_READUSERS, 1, 4);
	btRPCPutArg(outPacket, B_INT32_TYPE, &dir, sizeof(int32));
	bt_inPacket *inPacket = btRPCSimpleCall(authServerIP, BT_BESURE_PORT, outPacket);
	if (inPacket)
	{
		int error, count = 0;

		do
		{
			error = btRPCGetInt32(inPacket);
			if (error == B_OK)
			{
				memset(user, 0, sizeof(user));
				memset(fullName, 0, sizeof(fullName));
				btRPCGetString(inPacket, user, sizeof(user));
				btRPCGetString(inPacket, fullName, sizeof(fullName));

				nIndex = m_userList.InsertItem(0, user, 0);
				m_userList.SetItemText(nIndex, 1, fullName);
				m_userList.SetItemData(nIndex, 0);
			}
			else break;
		} while (++count < 80);

		free(inPacket->buffer);
		free(inPacket);
	}
}

void CDomainUsers::AddGroupList()
{
	extern unsigned int authServerIP;
	int32 *dir = NULL;
	char group[MAX_USERNAME_LENGTH];
	int nIndex;

	bt_outPacket *outPacket = btRPCPutHeader(BT_CMD_READGROUPS, 1, 4);
	btRPCPutArg(outPacket, B_INT32_TYPE, &dir, sizeof(int32));
	bt_inPacket *inPacket = btRPCSimpleCall(authServerIP, BT_BESURE_PORT, outPacket);
	if (inPacket)
	{
		int error, count = 0;

		do
		{
			error = btRPCGetInt32(inPacket);
			if (error == B_OK)
			{
				memset(group, 0, sizeof(group));
				btRPCGetString(inPacket, group, sizeof(group));
				nIndex = m_userList.InsertItem(0, group, 1);
				m_userList.SetItemData(nIndex, 1);
			}
			else break;
		} while (++count < 80);

		free(inPacket->buffer);
		free(inPacket);
	}
}
