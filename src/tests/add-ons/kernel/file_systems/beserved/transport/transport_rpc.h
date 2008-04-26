#include "betalk.h"

int btStartPrintJob(bt_rpcinfo *info, char *printerName, char *user, char *password, char *jobName, char **jobId);
int btPrintJobData(bt_rpcinfo *info, char *printerName, char *jobId, char *data, int dataLen);
int btCommitPrintJob(bt_rpcinfo *info, char *printerName, char *jobId);
