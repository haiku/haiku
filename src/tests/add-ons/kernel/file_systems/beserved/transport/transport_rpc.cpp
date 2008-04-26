#include "betalk.h"
#include "beserved_rpc.h"


// btStartPrintJob()
//
int btStartPrintJob(bt_rpcinfo *info, char *printerName, char *user, char *password, char *jobName, char **jobId)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_PRINTJOB_NEW, 4, strlen(printerName) + strlen(jobName) + strlen(user) + BT_AUTH_TOKEN_LENGTH);
	btRPCPutArg(outPacket, B_STRING_TYPE, printerName, strlen(printerName));
	btRPCPutArg(outPacket, B_STRING_TYPE, user, strlen(user));
	btRPCPutArg(outPacket, B_STRING_TYPE, password, BT_AUTH_TOKEN_LENGTH);
	btRPCPutArg(outPacket, B_STRING_TYPE, jobName, strlen(jobName));
	call = btRPCInvoke(info, outPacket, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
		{
			error = btRPCGetInt32(inPacket);
			if (error == B_OK)
				*jobId = btRPCGetNewString(inPacket);
		}

		btRPCRemoveCall(call);
	}

	return error;
}

// btPrintJobData()
//
int btPrintJobData(bt_rpcinfo *info, char *printerName, char *jobId, char *data, int dataLen)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_PRINTJOB_DATA, 4, strlen(printerName) + strlen(jobId) + dataLen + 4);
	btRPCPutArg(outPacket, B_STRING_TYPE, printerName, strlen(printerName));
	btRPCPutArg(outPacket, B_STRING_TYPE, jobId, strlen(jobId));
	btRPCPutArg(outPacket, B_STRING_TYPE, data, dataLen);
	btRPCPutArg(outPacket, B_INT32_TYPE, &dataLen, sizeof(dataLen));
	call = btRPCInvoke(info, outPacket, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
			error = btRPCGetInt32(inPacket);

		btRPCRemoveCall(call);
	}

	return error;
}

// btCommitPrintJob()
//
int btCommitPrintJob(bt_rpcinfo *info, char *printerName, char *jobId)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_PRINTJOB_COMMIT, 2, strlen(printerName) + strlen(jobId));
	btRPCPutArg(outPacket, B_STRING_TYPE, printerName, strlen(printerName));
	btRPCPutArg(outPacket, B_STRING_TYPE, jobId, strlen(jobId));
	call = btRPCInvoke(info, outPacket, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
			error = btRPCGetInt32(inPacket);

		btRPCRemoveCall(call);
	}

	return error;
}
