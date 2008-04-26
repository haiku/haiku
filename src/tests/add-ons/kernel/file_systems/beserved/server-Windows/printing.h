#include "beCompat.h"
#include "betalk.h"


typedef struct
{
	char printerName[MAX_NAME_LENGTH];
	char deviceName[B_FILE_NAME_LENGTH];
	char deviceType[MAX_NAME_LENGTH];
	char spoolDir[B_PATH_NAME_LENGTH];

	bool killed;
	bool used;

	bt_user_rights *rights;
	int security;

	HANDLE handlerID;
} bt_printer;


typedef struct
{
	char jobName[MAX_DESC_LENGTH + 1];
	uint32 sourceAddr;
	char sourceUser[MAX_USERNAME_LENGTH + 1];
	char status[MAX_DESC_LENGTH + 1];
} bt_print_job;


// Although there is no maximum number of entries that can be queued for
// printing, except as limited by available disk space, for simplicity
// BeServed will only report on the first MAX_PRINT_JOBS in the queue.
// This keeps the print job query from requiring repeated calls to handle
// large volume.

#define MAX_PRINT_JOBS		(BT_MAX_IO_BUFFER / sizeof(bt_print_job))

int btPrintJobNew(char *printerName, char *user, char *password, int client_s_addr, char *jobName, char *jobId);
int btPrintJobData(char *printerName, char *jobId, char *jobData, int dataLen);
int btPrintJobCommit(char *printerName, char *jobId);
int btPrintJobQuery(char *printerName, bt_print_job *jobList);