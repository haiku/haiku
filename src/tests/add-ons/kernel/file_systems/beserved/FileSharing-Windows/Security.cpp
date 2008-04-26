// Security.cpp : implementation file
//

#include "stdafx.h"
#include "winsock2.h"

#include "FileSharing.h"
#include "Security.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSecurity dialog


CSecurity::CSecurity(CWnd* pParent /*=NULL*/)
	: CDialog(CSecurity::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSecurity)
	m_server = _T("");
	//}}AFX_DATA_INIT
}


void CSecurity::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSecurity)
	DDX_Control(pDX, IDOK, m_okayBtn);
	DDX_Control(pDX, IDC_SECURITY_TYPE, m_type);
	DDX_Control(pDX, IDC_SECURITY_SERVER, m_serverCtrl);
	DDX_Text(pDX, IDC_SECURITY_SERVER, m_server);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSecurity, CDialog)
	//{{AFX_MSG_MAP(CSecurity)
	ON_EN_CHANGE(IDC_SECURITY_SERVER, OnChangeSecurityServer)
	ON_CBN_SELCHANGE(IDC_SECURITY_TYPE, OnSelchangeSecurityType)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSecurity message handlers

BOOL CSecurity::OnInitDialog() 
{
	extern char authServerName[];
	extern unsigned int authServerIP;

	CDialog::OnInitDialog();

	if (authServerIP)
	{
		m_type.SelectString(-1, "BeSure Authentication Server");
		m_server = authServerName;
		UpdateData(FALSE);
		m_serverCtrl.EnableWindow(TRUE);
	}
	else
		m_type.SelectString(-1, "No Authentication Required");

	m_type.SetFocus();

	return FALSE;
}

void CSecurity::OnChangeSecurityServer() 
{
	UpdateData(TRUE);
	m_okayBtn.EnableWindow(!m_server.IsEmpty());
}

void CSecurity::OnSelchangeSecurityType() 
{
	CString typeName;
	int nType = m_type.GetCurSel();

	m_type.GetLBText(nType, typeName);
	if (typeName.CompareNoCase("No Authentication Required") == 0)
	{
		m_serverCtrl.EnableWindow(FALSE);
		m_okayBtn.EnableWindow(TRUE);
	}
	else
	{
		m_serverCtrl.EnableWindow(TRUE);
		UpdateData(TRUE);
		m_okayBtn.EnableWindow(!m_server.IsEmpty());
	}
}

void CSecurity::OnOK() 
{
	extern char authServerName[];
	extern unsigned int authServerIP;
	struct hostent *ent;

	CDialog::OnOK();

	strcpy(authServerName, m_server);
	ent = gethostbyname(authServerName);
	if (ent == NULL)
	{
		unsigned long addr = inet_addr(authServerName);
		authServerIP = ntohl(addr);
	}
	else
		authServerIP = ntohl(*((unsigned int *) ent->h_addr));
}
