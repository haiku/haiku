#define PRINTDRIVER

#include "stdafx.h"

#include "beCompat.h"
#include "betalk.h"
#include "authentication.h"
#include "printing.h"

#include "string.h"
#include "winspool.h"
#include "stdio.h"


extern void btMakePath(char *path, char *dir, char *file);
extern bt_printer *btFindPrinter(char *printerName);

DWORD WINAPI printerThread(LPVOID data);
void CheckPrintJobStatus(bt_printer *printer, char *printJob);
bool PrintFile(bt_printer *printer, char *jobName, char *filename);


// printerThread()
//
DWORD WINAPI printerThread(LPVOID data)
{
	bt_printer *printer = (bt_printer *) data;
	struct _finddata_t fileInfo;
	char search[B_PATH_NAME_LENGTH], path[B_PATH_NAME_LENGTH];
	long int result, finder;

	if (!data)
		return 0;

	// Create the search path for all .job files in the spool folder.
	btMakePath(search, printer->spoolDir, "*.job");

	while (!printer->killed)
	{
		// Find the first print job.
		finder = result = _findfirst(search, &fileInfo);
		while (result != -1)
		{
			// Create the fully-qualified path to this print job file, then check
			// its status.
			btMakePath(path, printer->spoolDir, fileInfo.name);
			CheckPrintJobStatus(printer, path);

			// Get the next print job.
			result = _findnext(finder, &fileInfo);
		}

		// Close our job search and wait awhile.
		_findclose(finder);
		Sleep(2000);
	}

	return 0;
}

// CheckPrintJobStatus()
//
void CheckPrintJobStatus(bt_printer *printer, char *printJob)
{
	char dataPath[B_PATH_NAME_LENGTH], dataFile[B_FILE_NAME_LENGTH];
	char jobName[B_FILE_NAME_LENGTH], status[50];

	GetPrivateProfileString("PrintJob", "Status", "", status, sizeof(status), printJob);

	if (stricmp(status, "Scheduling...") == 0)
	{
		// Move status from scheduling to printing.
		WritePrivateProfileString("PrintJob", "Status", "Printing...", printJob);

		// Get the name of the spooled file and print it.
		GetPrivateProfileString("PrintJob", "JobName", "", jobName, sizeof(jobName), printJob);
		GetPrivateProfileString("PrintJob", "DataFile", "", dataFile, sizeof(dataFile), printJob);
		btMakePath(dataPath, printer->spoolDir, dataFile);
		PrintFile(printer, jobName, dataPath);

		// Remove the job and data files.
		remove(dataPath);
		remove(printJob);
	}
}

// PrintFile()
//
bool PrintFile(bt_printer *printer, char *jobName, char *fileName)
{
	HANDLE hPrinter;
	DOC_INFO_1 docInfo;
	DWORD dwJob, bytesSent;
	size_t bytes;

	// Allocate a buffer for print data.
	char *buffer = (char *) malloc(BT_MAX_IO_BUFFER + 1);
	if (!buffer)
		return false;

	// Open the file for reading
	FILE *fp = fopen(fileName, "rb");
	if (!fp)
	{
		free(buffer);
		return false;
	}

	// Need a handle to the printer.
	if (OpenPrinter(printer->deviceName, &hPrinter, NULL))
	{
		// Fill in the structure with info about this "document."
		docInfo.pDocName = jobName;
		docInfo.pOutputFile = NULL;
		docInfo.pDatatype = "RAW";

		// Inform the spooler the document is beginning.
		if ((dwJob = StartDocPrinter(hPrinter, 1, (unsigned char *) &docInfo)) != 0)
		{
			// Start a page.
			if (StartPagePrinter(hPrinter))
			{
				// Send the data to the printer.
				// Print the file 8K at a time, until we are finished or shut down.
				while (!printer->killed)
				{
					bytes = fread(buffer, 1, BT_MAX_IO_BUFFER, fp);
					if (bytes <= 0)
						break;

					if (!WritePrinter(hPrinter, buffer, bytes, &bytesSent))
						break;
				}

				EndPagePrinter(hPrinter);
			}

			EndDocPrinter(hPrinter);
		}

		ClosePrinter(hPrinter);
	}

	if (ferror(fp))
		clearerr(fp);

	fclose(fp);
	free(buffer);
	return true;
}

// btPrintJobNew()
//
int btPrintJobNew(char *printerName, char *user, char *password, int client_s_addr, char *jobName, char *jobId)
{
	char printJob[B_PATH_NAME_LENGTH], dataFile[B_PATH_NAME_LENGTH], addr[40];
	bt_printer *printer = btFindPrinter(printerName);
	bt_user_rights *ur;
	char *groups[MAX_GROUPS_PER_USER];
	int i, rights;
	bool authenticated = false;

	// Create a new print job by writing the <jobId>.job file and by
	// creating the <jobId>.prn file.
	if (printer)
	{
		SYSTEMTIME st;
		int docId = 0;

		// Verify the user has permission to print.
		if (printer->security != BT_AUTH_NONE)
		{
			// Authenticate the user with name/password.
			authenticated = authenticateUser(user, password);
			if (!authenticated)
				return BEOS_EACCES;

			// Does the authenticated user have any rights on this file share?
			rights = 0;
			for (ur = printer->rights; ur; ur = ur->next)
				if (!ur->isGroup && stricmp(ur->user, user) == 0)
					rights |= ur->rights;

			// Does the authenticated user belong to any groups that have any rights on this
			// file share?
			for (i = 0; i < MAX_GROUPS_PER_USER; i++)
				groups[i] = NULL;

			getUserGroups(user, groups);
			for (ur = printer->rights; ur; ur = ur->next)
				if (ur->isGroup)
					for (i = 0; i < MAX_GROUPS_PER_USER; i++)
						if (groups[i] && stricmp(ur->user, groups[i]) == 0)
						{
							rights |= ur->rights;
							break;
						}

			// Free the memory occupied by the group list.
			for (i = 0; i < MAX_GROUPS_PER_USER; i++)
				if (groups[i])
					free(groups[i]);

			// If no rights have been granted, deny access.
			if (!(rights & BT_RIGHTS_PRINT))
				return BEOS_EACCES;
		}

		// Create a new job ID.
		do
		{
			GetSystemTime(&st);
			sprintf(jobId, "%x-%03d-%d", time(NULL), st.wMilliseconds, docId++);
			sprintf(printJob, "%s\\%s.job", printer->spoolDir, jobId);
		} while (access(printJob, 0) == 0);

		// Write the job information file.
		sprintf(dataFile, "%s.prn", jobId);
		WritePrivateProfileString("PrintJob", "JobName", jobName, printJob);
		WritePrivateProfileString("PrintJob", "Status", "Queueing...", printJob);
		WritePrivateProfileString("PrintJob", "DataFile", dataFile, printJob);
		WritePrivateProfileString("PrintJob", "User", user, printJob);

		uint8 *_s_addr = (uint8 *) &client_s_addr;
		sprintf(addr, "%d.%d.%d.%d", _s_addr[0], _s_addr[1], _s_addr[2], _s_addr[3]);
		WritePrivateProfileString("PrintJob", "Source", addr, printJob);

		// Now create the empty data file.
		sprintf(dataFile, "%s\\%s.prn", printer->spoolDir, jobId);
//		_creat(dataFile, _S_IWRITE);
		HANDLE hFile = CreateFile(dataFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
		CloseHandle(hFile);

		return B_OK;
	}

	return BEOS_ENOENT;
}

// btPrintJobData()
//
int btPrintJobData(char *printerName, char *jobId, char *jobData, int dataLen)
{
	char printJob[B_PATH_NAME_LENGTH];
	bt_printer *printer = btFindPrinter(printerName);
	FILE *fp;

	if (printer)
	{
		sprintf(printJob, "%s\\%s.job", printer->spoolDir, jobId);
		if (access(printJob, 0) == 0)
		{
			sprintf(printJob, "%s\\%s.prn", printer->spoolDir, jobId);
			fp = fopen(printJob, "ab");
			if (fp)
			{
				fwrite(jobData, 1, dataLen, fp);
				fclose(fp);
				return B_OK;
			}

			return BEOS_EACCES;
		}
	}

	return BEOS_ENOENT;
}

// btPrintJobCommit()
//
int btPrintJobCommit(char *printerName, char *jobId)
{
	char printJob[B_PATH_NAME_LENGTH];
	bt_printer *printer = btFindPrinter(printerName);

	if (printer)
	{
		sprintf(printJob, "%s\\%s.job", printer->spoolDir, jobId);
		if (access(printJob, 0) == 0)
		{
			WritePrivateProfileString("PrintJob", "Status", "Scheduling...", printJob);
			return B_OK;
		}
	}

	return BEOS_ENOENT;
}
