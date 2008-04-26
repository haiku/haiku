// FileSharingDlg.cpp : implementation file
//

#include "stdafx.h"

#include "ctype.h"
#include "sys/types.h"
#include "sys/stat.h"

#include "becompat.h"
#include "betalk.h"

#include "FileSharing.h"
#include "FileSharingDlg.h"
#include "ShareProperties.h"
#include "printing.h"
#include "Security.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern bt_fileShare_t fileShares[];
extern bt_printer sharedPrinters[];
extern char authServerName[];
extern unsigned int authServerIP;

char tokBuffer[MAX_NAME_LENGTH + 1], *tokPtr;

char *keywords[] =
{
	"share",
	"as",
	"set",
	"read",
	"write",
	"read-write",
	"promiscuous",
	"on",
	"to",
	"authenticate",
	"with",
	"group",
	"printer",
	"print",
	"is",
	"spooled",
	"device",
	"type",
	NULL
};



/////////////////////////////////////////////////////////////////////////////
// CFileSharingDlg dialog

CFileSharingDlg::CFileSharingDlg()
	: CPropertyPage(CFileSharingDlg::IDD)
{
	//{{AFX_DATA_INIT(CFileSharingDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CFileSharingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFileSharingDlg)
	DDX_Control(pDX, IDC_SHARE_REMOVE, m_removeBtn);
	DDX_Control(pDX, IDC_SHARE_EDIT, m_editBtn);
	DDX_Control(pDX, IDC_MAIN_HEADING, m_heading);
	DDX_Control(pDX, IDC_SHARE_LIST, m_shareList);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CFileSharingDlg, CDialog)
	//{{AFX_MSG_MAP(CFileSharingDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(NM_CLICK, IDC_SHARE_LIST, OnClickShareList)
	ON_NOTIFY(NM_DBLCLK, IDC_SHARE_LIST, OnDblclkShareList)
	ON_BN_CLICKED(IDC_SHARE_EDIT, OnShareEdit)
	ON_BN_CLICKED(IDC_SHARE_REMOVE, OnShareRemove)
	ON_BN_CLICKED(IDC_SHARE_NEW, OnShareNew)
	ON_BN_CLICKED(IDC_SECURITY, OnSecurity)
	//}}AFX_MSG_MAP
	ON_WM_DESTROY()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFileSharingDlg message handlers

BOOL CFileSharingDlg::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_editBtn.EnableWindow(FALSE);
	m_removeBtn.EnableWindow(FALSE);

	m_shareList.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	m_shareList.InsertColumn(0, "Share Name", LVCFMT_LEFT, 150);
	m_shareList.InsertColumn(1, "Access", LVCFMT_LEFT, 80);
	m_shareList.InsertColumn(2, "Path", LVCFMT_LEFT, 200);

	CBitmap fileShare;
	fileShare.LoadBitmap(IDB_VOLUME);

	m_images.Create(16, 16, FALSE, 2, 0);
	m_images.Add(&fileShare, (COLORREF) 0);
	m_shareList.SetImageList(&m_images, LVSIL_SMALL);

	m_hBoldFont = InitializeControlFont("Tahoma", FW_BOLD, 8);
	m_heading.SendMessage(WM_SETFONT, (WPARAM) m_hBoldFont, MAKELPARAM(TRUE, 0));

	InitShares();
	ListShares();

	return TRUE;
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CFileSharingDlg::OnPaint() 
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
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CFileSharingDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

// InitShares()
//
void CFileSharingDlg::InitShares()
{
	extern char settingsFile[];
	FILE *fp;
	char buffer[512];
	int i, length;

	for (i = 0; i < BT_MAX_FILE_SHARES; i++)
	{
		fileShares[i].name[0] = 0;
		fileShares[i].path[0] = 0;
		fileShares[i].used = false;
		fileShares[i].readOnly = true;
		fileShares[i].followLinks = false;
		fileShares[i].next = NULL;
	}

	for (i = 0; i < BT_MAX_PRINTER_SHARES; i++)
	{
		sharedPrinters[i].printerName[0] = 0;
		sharedPrinters[i].deviceName[0] = 0;
		sharedPrinters[i].spoolDir[0] = 0;
		sharedPrinters[i].rights = NULL;
		sharedPrinters[i].security = BT_AUTH_NONE;
		sharedPrinters[i].used = false;
		sharedPrinters[i].killed = false;
	}

	GetCurrentDirectory(sizeof(buffer), buffer);
	fp = fopen(settingsFile, "r");
	if (fp)
	{
		while (fgets(buffer, sizeof(buffer) - 1, fp))
		{
			length = strlen(buffer);
			if (length <= 1 || buffer[0] == '#')
				continue;

			if (buffer[length - 1] == '\n')
				buffer[--length] = 0;

			if (strncmp(buffer, "share ", 6) == 0)
				GetFileShare(buffer);
			else if (strncmp(buffer, "set ", 4) == 0)
				GetShareProperty(buffer);
			else if (strncmp(buffer, "grant ", 6) == 0)
				GetGrant(buffer);
			else if (strncmp(buffer, "authenticate ", 13) == 0)
				GetAuthenticate(buffer);
			else if (strncmp(buffer, "printer ", 8) == 0)
				GetPrinter(buffer);
		}

		fclose(fp);
	}
}

// GetFileShare()
//
void CFileSharingDlg::GetFileShare(const char *buffer)
{
//	struct stat st;
	char path[B_PATH_NAME_LENGTH], share[MAX_NAME_LENGTH + 1], *folder;
	int i, tok;

	// Skip over SHARE command.
	tokPtr = (char *) buffer + (6 * sizeof(char));

	tok = GetToken();
	if (tok != BT_TOKEN_STRING)
		return;

	strcpy(path, tokBuffer);
	tok = GetToken();
	if (tok != BT_TOKEN_AS)
		return;

	tok = GetToken();
	if (tok != BT_TOKEN_STRING)
		return;

	strcpy(share, tokBuffer);

	// Now verify that the share name specified has not already been
	// used to share another path.
	folder = GetSharePath(share);
	if (folder)
		return;

	for (i = 0; i < BT_MAX_FILE_SHARES; i++)
		if (!fileShares[i].used)
		{
			strcpy(fileShares[i].name, share);
			strcpy(fileShares[i].path, path);
			fileShares[i].used = true;
			return;
		}
}

void CFileSharingDlg::GetShareProperty(const char *buffer)
{
	char share[B_FILE_NAME_LENGTH + 1];
	int tok, shareId;

	// Skip over SET command.
	tokPtr = (char *) buffer + (4 * sizeof(char));

	tok = GetToken();
	if (tok != BT_TOKEN_STRING)
		return;

	strcpy(share, tokBuffer);

	// Get the index of the share referred to.  If the named share cannot be
	// found, then abort.
	shareId = GetShareId(share);
	if (shareId < 0)
		return;

	tok = GetToken();
	if (tok == BT_TOKEN_READWRITE)
		fileShares[shareId].readOnly = false;
}

void CFileSharingDlg::GetGrant(const char *buffer)
{
	char share[MAX_NAME_LENGTH + 1];
	int tok, rights;
	bool isGroup = false;

	// Skip over GRANT command.
	tokPtr = (char *) buffer + (6 * sizeof(char));
	rights = 0;

	do
	{
		tok = GetToken();
		if (tok == BT_TOKEN_READ)
		{
			rights |= BT_RIGHTS_READ;
			tok = GetToken();
		}
		else if (tok == BT_TOKEN_WRITE)
		{
			rights |= BT_RIGHTS_WRITE;
			tok = GetToken();
		}
		else if (tok == BT_TOKEN_PRINT)
		{
			rights |= BT_RIGHTS_PRINT;
			tok = GetToken();
		}
	} while (tok == BT_TOKEN_COMMA);

	if (tok != BT_TOKEN_ON)
		return;

	tok = GetToken();
	if (tok != BT_TOKEN_STRING)
		return;

	strcpy(share, tokBuffer);
	tok = GetToken();
	if (tok != BT_TOKEN_TO)
		return;

	tok = GetToken();
	if (tok == BT_TOKEN_GROUP)
	{
		isGroup = true;
		tok = GetToken();
	}

	if (tok != BT_TOKEN_STRING)
		return;

	AddUserRights(share, tokBuffer, rights, isGroup);
}

void CFileSharingDlg::GetAuthenticate(const char *buffer)
{
	struct hostent *ent;
	int i, tok;

	// Skip over AUTHENTICATE command.
	tokPtr = (char *) buffer + (13 * sizeof(char));

	tok = GetToken();
	if (tok != BT_TOKEN_WITH)
		return;

	tok = GetToken();
	if (tok != BT_TOKEN_STRING)
		return;

	// Look up address for given host.
	strcpy(authServerName, tokBuffer);
	ent = gethostbyname(tokBuffer);
	if (ent != NULL)
		authServerIP = ntohl(*((unsigned int *) ent->h_addr));
	else
		authServerIP = 0;

	// Make all file shares use BeSure authentication.
	for (i = 0; i < BT_MAX_FILE_SHARES; i++)
		fileShares[i].security = BT_AUTH_BESURE;

	// Make all shared printers use BeSure authentication.
	for (i = 0; i < BT_MAX_PRINTER_SHARES; i++)
		sharedPrinters[i].security = BT_AUTH_BESURE;
}

void CFileSharingDlg::AddUserRights(char *share, char *user, int rights, bool isGroup)
{
	bt_user_rights *ur;
	bt_printer *printer = NULL;
	int shareId = -1;

	// Now make sure that the rights make sense.  If we're granting the
	// right to print, we should not have granted the right to read or write.
	// We should also be working exclusively with a printer.
	if (rights & BT_RIGHTS_PRINT)
	{
		if ((rights & BT_RIGHTS_READ) == 0 && (rights & BT_RIGHTS_WRITE) == 0)
			printer = btFindPrinter(share);
	}
	else
		shareId = GetShareId(share);

	// If we didn't find a printer or a file share, then quit here.
	if (!printer && shareId < 0)
		return;

	ur = (bt_user_rights *) malloc(sizeof(bt_user_rights));
	if (ur)
	{
		ur->user = (char *) malloc(strlen(user) + 1);
		if (ur->user)
		{
			strcpy(ur->user, user);
			ur->rights = rights;
			ur->isGroup = isGroup;

			if (printer)
			{
				ur->next = printer->rights;
				printer->rights = ur;
			}
			else
			{
				ur->next = fileShares[shareId].rights;
				fileShares[shareId].rights = ur;
			}
		}
		else
			free(ur);
	}
}

void CFileSharingDlg::GetPrinter(const char *buffer)
{
	bt_printer printer;
	int i, tok;

	// Skip over PRINTER command.
	tokPtr = (char *) buffer + (8 * sizeof(char));

	// Now get the name this printer will be shared as.
	tok = GetToken();
	if (tok != BT_TOKEN_STRING)
		return;

	strcpy(printer.printerName, tokBuffer);

	// Bypass the IS and TYPE keywords.
	tok = GetToken();
	if (tok != BT_TOKEN_IS)
		return;

	tok = GetToken();
	if (tok != BT_TOKEN_TYPE)
		return;

	// Get the device type of the printer.
	tok = GetToken();
	if (tok != BT_TOKEN_STRING)
		return;

	strcpy(printer.deviceType, tokBuffer);

	// Bypass the DEVICE keyword.
	tok = GetToken();
	if (tok != BT_TOKEN_DEVICE)
		return;

	// Get the system device name of the printer.
	tok = GetToken();
	if (tok != BT_TOKEN_STRING)
		return;

	strcpy(printer.deviceName, tokBuffer);

	// Bypass the SPOOLED TO keywords.
	tok = GetToken();
	if (tok != BT_TOKEN_SPOOLED)
		return;

	tok = GetToken();
	if (tok != BT_TOKEN_TO)
		return;

	// Get the spooling folder, where temporary files will be stored.
	tok = GetToken();
	if (tok != BT_TOKEN_STRING)
		return;

	strcpy(printer.spoolDir, tokBuffer);

	for (i = 0; i < BT_MAX_PRINTER_SHARES; i++)
		if (!sharedPrinters[i].used)
		{
			sharedPrinters[i].used = true;
			strcpy(sharedPrinters[i].printerName, printer.printerName);
			strcpy(sharedPrinters[i].deviceType, printer.deviceType);
			strcpy(sharedPrinters[i].deviceName, printer.deviceName);
			strcpy(sharedPrinters[i].spoolDir, printer.spoolDir);
			break;
		}
}

int CFileSharingDlg::GetToken()
{
	bool quoted = false;

	tokBuffer[0] = 0;
	while (*tokPtr && iswhite(*tokPtr))
		tokPtr++;

	if (*tokPtr == ',')
	{
		*tokPtr++;
		return BT_TOKEN_COMMA;
	}
	else if (*tokPtr == '\"')
	{
		quoted = true;
		tokPtr++;
	}

	if (isalnum(*tokPtr) || *tokPtr == '\\')
	{
		int i = 0;
		while (isalnum(*tokPtr) || isValid(*tokPtr) || (quoted && *tokPtr == ' '))
			if (i < MAX_NAME_LENGTH)
				tokBuffer[i++] = *tokPtr++;
			else
				tokPtr++;

		tokBuffer[i] = 0;

		if (!quoted)
			for (i = 0; keywords[i]; i++)
				if (stricmp(tokBuffer, keywords[i]) == 0)
					return ++i;

		if (quoted)
			if (*tokPtr != '\"')
				return BT_TOKEN_ERROR;
			else
				tokPtr++;

		return BT_TOKEN_STRING;
	}

	return BT_TOKEN_ERROR;
}

char *CFileSharingDlg::GetSharePath(char *shareName)
{
	int i;

	for (i = 0; i < BT_MAX_FILE_SHARES; i++)
		if (fileShares[i].used)
			if (stricmp(fileShares[i].name, shareName) == 0)
				return fileShares[i].path;

	return NULL;
}

int CFileSharingDlg::GetShareId(char *shareName)
{
	int i;

	for (i = 0; i < BT_MAX_FILE_SHARES; i++)
		if (fileShares[i].used)
			if (stricmp(fileShares[i].name, shareName) == 0)
				return i;

	return -1;
}

// btFindPrinter()
//
bt_printer *CFileSharingDlg::btFindPrinter(char *printerName)
{
	int i;

	for (i = 0; i < BT_MAX_PRINTER_SHARES; i++)
		if (sharedPrinters[i].used)
			if (stricmp(printerName, sharedPrinters[i].printerName) == 0)
				return &sharedPrinters[i];

	return NULL;
}

// WriteShares()
//
void CFileSharingDlg::WriteShares(FILE *fp)
{
	bt_user_rights *ur;
	int i;

	if (fp)
	{
		for (i = 0; i < BT_MAX_FILE_SHARES; i++)
			if (fileShares[i].used)
			{
				fprintf(fp, "share ");
				PrintString(fp, fileShares[i].path);
				fprintf(fp, " as ");
				PrintString(fp, fileShares[i].name);
				fputc('\n', fp);

				if (!fileShares[i].readOnly)
				{
					fprintf(fp, "set ");
					PrintString(fp, fileShares[i].name);
					fprintf(fp, " read-write\n");
				}

				if (fileShares[i].followLinks)
				{
					fprintf(fp, "set ");
					PrintString(fp, fileShares[i].name);
					fprintf(fp, " promiscuous\n");
				}

				for (ur = fileShares[i].rights; ur; ur = ur->next)
				{
					int rights = ur->rights;
					fprintf(fp, "grant ");

					do
					{
						if (rights & BT_RIGHTS_READ)
						{
							fprintf(fp, "read");
							rights &= ~BT_RIGHTS_READ;
						}
						else if (rights & BT_RIGHTS_WRITE)
						{
							fprintf(fp, "write");
							rights &= ~BT_RIGHTS_WRITE;
						}

						fprintf(fp, "%c", rights > 0 ? ',' : ' ');
					} while (rights > 0);

					fprintf(fp, "on ");
					PrintString(fp, fileShares[i].name);
					fprintf(fp, " to ");
					if (ur->isGroup)
						fprintf(fp, "group ");
					fprintf(fp, "%s\n", ur->user);
				}
			}
	}
}

// ListShares()
//
void CFileSharingDlg::ListShares()
{
	struct stat st;
	int i, nItem;

	for (i = 0; i < BT_MAX_FILE_SHARES; i++)
		if (fileShares[i].used)
		{
			nItem = m_shareList.InsertItem(0, fileShares[i].name, 0);
			m_shareList.SetItemText(nItem, 1, fileShares[i].readOnly ? "Read-only" : "Read-Write");
			m_shareList.SetItemText(nItem, 2, stat(fileShares[i].path, &st) == 0 ? fileShares[i].path : "--- Invalid Path ---");
			m_shareList.SetItemData(nItem, i);
		}
}

void CFileSharingDlg::OnClickShareList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
	m_editBtn.EnableWindow(GetSelectedListItem(&m_shareList) != -1);
	m_removeBtn.EnableWindow(GetSelectedListItem(&m_shareList) != -1);
}

void CFileSharingDlg::OnDblclkShareList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
	OnShareEdit();
}

void CFileSharingDlg::OnShareEdit() 
{
	CShareProperties share;
	int nItem = GetSelectedListItem(&m_shareList);
	if (nItem != -1)
		if (share.ShowProperties(m_shareList.GetItemData(nItem)) == IDOK)
			RefreshShare(nItem);
}

void CFileSharingDlg::OnShareRemove() 
{
	int nItem = GetSelectedListItem(&m_shareList);
	if (nItem >= 0)
		if (MessageBox(WARN_REMOVE_SHARE, fileShares[nItem].name, MB_YESNO | MB_ICONQUESTION) == IDYES)
		{
			fileShares[nItem].used = false;
			m_shareList.DeleteItem(nItem);
		}
}

void CFileSharingDlg::OnShareNew() 
{
	CShareProperties share;
	int nIndex = m_shareList.GetItemCount();
	strcpy(fileShares[nIndex].name, "Untitled");
	strcpy(fileShares[nIndex].path, "c:\\temp");
	fileShares[nIndex].readOnly = true;
	fileShares[nIndex].followLinks = false;
	if (share.ShowProperties(nIndex) == IDOK)
	{
		int nItem = m_shareList.InsertItem(0, fileShares[nIndex].name, 0);
		m_shareList.SetItemText(nItem, 1, fileShares[nIndex].readOnly ? "Read-only" : "Read-Write");
		m_shareList.SetItemText(nItem, 2, fileShares[nIndex].path);
		m_shareList.SetItemData(nItem, nIndex);
		fileShares[nIndex].used = true;
	}
}

void CFileSharingDlg::RefreshShare(int nItem)
{
	int nIndex = m_shareList.GetItemData(nItem);
	m_shareList.SetItemText(nItem, 0, fileShares[nIndex].name);
	m_shareList.SetItemText(nItem, 1, fileShares[nIndex].readOnly ? "Read-only" : "Read-Write");
	m_shareList.SetItemText(nItem, 2, fileShares[nIndex].path);
}

void CFileSharingDlg::OnSecurity() 
{
	CSecurity security;
	security.DoModal();
}
