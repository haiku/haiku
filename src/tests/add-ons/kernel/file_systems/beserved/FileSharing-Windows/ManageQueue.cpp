// ManageQueue.cpp : implementation file
//

#include "stdafx.h"

#include "FileSharing.h"
#include "ManageQueue.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CManageQueue dialog


CManageQueue::CManageQueue(CWnd* pParent /*=NULL*/)
	: CDialog(CManageQueue::IDD, pParent)
{
	//{{AFX_DATA_INIT(CManageQueue)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CManageQueue::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CManageQueue)
	DDX_Control(pDX, IDC_PAUSE_JOB, m_pauseBtn);
	DDX_Control(pDX, IDC_REMOVE_JOB, m_removeBtn);
	DDX_Control(pDX, IDC_QUEUE_LIST, m_queue);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CManageQueue, CDialog)
	//{{AFX_MSG_MAP(CManageQueue)
	ON_WM_TIMER()
	ON_NOTIFY(NM_CLICK, IDC_QUEUE_LIST, OnClickQueueList)
	ON_BN_CLICKED(IDC_PAUSE_JOB, OnPauseJob)
	ON_BN_CLICKED(IDC_REMOVE_JOB, OnRemoveJob)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CManageQueue message handlers

int CManageQueue::ShowQueue(bt_printer *printer)
{
	this->printer = printer;
	return DoModal();
}

BOOL CManageQueue::OnInitDialog() 
{
	CDialog::OnInitDialog();
	SetWindowText(CString(printer->printerName) + " Queue");

	m_queue.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	m_queue.InsertColumn(0, "Document Name", LVCFMT_LEFT, 190);
	m_queue.InsertColumn(1, "Status", LVCFMT_LEFT, 90);
	m_queue.InsertColumn(2, "Submitted By", LVCFMT_LEFT, 110);
	m_queue.InsertColumn(3, "Size", LVCFMT_RIGHT, 70);
	m_queue.InsertColumn(4, "Submitted On", LVCFMT_LEFT, 170);
	m_queue.InsertColumn(5, "Submitted From", LVCFMT_LEFT, 120);

	BuildJobList();
	return TRUE;
}

void CManageQueue::OnTimer(UINT nIDEvent) 
{
	CDialog::OnTimer(nIDEvent);
}

void CManageQueue::OnClickQueueList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;

	int nItem = GetSelectedListItem(&m_queue);
	if (nItem != -1)
	{
		CString status = m_queue.GetItemText(nItem, 1);
		if (status == "Paused...")
			m_pauseBtn.SetWindowText("&Continue");
		else
			m_pauseBtn.SetWindowText("&Pause");
	}
}

void CManageQueue::OnPauseJob() 
{
	int nItem = GetSelectedListItem(&m_queue);
	if (nItem != -1)
	{
		bt_print_job *printJob = (bt_print_job *) m_queue.GetItemData(nItem);
		if (printJob)
		{
			CString status = m_queue.GetItemText(nItem, 1);
			if (status == "Paused...")
			{
				WritePrivateProfileString("PrintJob", "Status", "Scheduling...", printJob->jobFile);
				m_queue.SetItemText(nItem, 1, "Scheduling...");
				m_pauseBtn.SetWindowText("&Pause");
			}
			else
			{
				WritePrivateProfileString("PrintJob", "Status", "Paused...", printJob->jobFile);
				m_queue.SetItemText(nItem, 1, "Paused...");
				m_pauseBtn.SetWindowText("&Continue");
			}
		}
	}

	m_queue.SetFocus();
}

void CManageQueue::OnRemoveJob() 
{
	char dataFile[B_PATH_NAME_LENGTH];
	int nItem = GetSelectedListItem(&m_queue);
	if (nItem != -1)
	{
		bt_print_job *printJob = (bt_print_job *) m_queue.GetItemData(nItem);
		if (printJob)
		{
			GetPrivateProfileString("PrintJob", "DataFile", "", dataFile, sizeof(dataFile), printJob->jobFile);
			remove(dataFile);
			remove(printJob->jobFile);
		}
	}
}

void CManageQueue::OnDestroy() 
{
	CDialog::OnDestroy();

	bt_print_job *job = rootJob, *nextJob;
	while (job)
	{
		nextJob = job->next;
		free(job);
		job = nextJob;
	}
}

void CManageQueue::BuildJobList()
{
	bt_print_job *job;
	struct _finddata_t fileInfo;
	char path[B_PATH_NAME_LENGTH];
	long int finder, result;

	rootJob = NULL;

	sprintf(path, "%s\\*.job", printer->spoolDir);
	finder = result = _findfirst(path, &fileInfo);
	while (result != -1)
	{
		// Create the fully-qualified path to this print job file, then check
		// its status.
		sprintf(path, "%s\\%s", printer->spoolDir, fileInfo.name);
		job = AddPrintJob(printer, path);
		job->next = rootJob;
		rootJob = job;

		// Get the next print job.
		result = _findnext(finder, &fileInfo);
	}
}

bt_print_job *CManageQueue::AddPrintJob(bt_printer *printer, char *path)
{
	bt_print_job *job;
	struct stat st;
	char dataFile[B_PATH_NAME_LENGTH];
	char buffer[B_FILE_NAME_LENGTH];
	int nItem;

	job = new bt_print_job;
	job->next = NULL;

	strcpy(job->jobFile, path);

	GetPrivateProfileString("PrintJob", "JobName", "Untitled", job->jobName, sizeof(job->jobName), path);
	nItem = m_queue.InsertItem(0, job->jobName);
	m_queue.SetItemData(nItem, (DWORD)(LPVOID) job);

	GetPrivateProfileString("PrintJob", "Status", "", job->status, sizeof(job->status), path);
	m_queue.SetItemText(nItem, 1, job->status);

	GetPrivateProfileString("PrintJob", "User", "Unknown", job->sourceUser, sizeof(job->sourceUser), path);
	m_queue.SetItemText(nItem, 2, job->sourceUser);

	GetPrivateProfileString("PrintJob", "DataFile", "", buffer, sizeof(buffer), path);
	sprintf(dataFile, "%s\\%s", printer->spoolDir, buffer);
	if (stat(dataFile, &st) == 0)
	{
		if (st.st_size > 1024 * 1024)
			sprintf(buffer, "%.2f MB", (float) st.st_size / (1024.0 * 1024.0));
		else if (st.st_size > 1024)
			sprintf(buffer, "%d KB", st.st_size / 1024);
		else
			sprintf(buffer, "%d", st.st_size);

		m_queue.SetItemText(nItem, 3, buffer);

		struct tm *time = localtime(&st.st_ctime);
		strftime(buffer, sizeof(buffer), "%a %b %d, %Y at %I:%M %p", time);
		m_queue.SetItemText(nItem, 4, buffer);
	}

	GetPrivateProfileString("PrintJob", "Source", "Unknown", buffer, sizeof(buffer), path);
	m_queue.SetItemText(nItem, 5, buffer);
	job->sourceAddr = inet_addr(buffer);

	return job;
}
