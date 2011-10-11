/*
 *	SiS 190/191 NIC Driver.
 *	Copyright (c) 2009 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */


#include "Settings.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <lock.h> // for mutex


bool gTraceOn = false;
bool gTruncateLogFile = false;
bool gTraceFlow = false;
bool gAddTimeStamp = true;
static char *gLogFilePath = NULL;

mutex gLogLock;


//
//	Logging, tracing and settings
//

static
void create_log()
{
	if (gLogFilePath == NULL)
		return;

	int flags = O_WRONLY | O_CREAT | ((gTruncateLogFile) ? O_TRUNC : 0);
	close(open(gLogFilePath, flags, 0666));

	mutex_init(&gLogLock, DRIVER_NAME"-logging");
}


void load_settings()
{
	void *handle = load_driver_settings(DRIVER_NAME);
	if (handle == 0)
		return;

	gTraceOn = get_driver_boolean_parameter(
			handle, "trace", gTraceOn, true);

	gTruncateLogFile = get_driver_boolean_parameter(
			handle,	"truncate_logfile", gTruncateLogFile, true);

	gTraceFlow = get_driver_boolean_parameter(
			handle, "trace_flow", gTraceFlow, true);

	gAddTimeStamp = get_driver_boolean_parameter(
			handle, "add_timestamp", gAddTimeStamp, true);

	const char * logFilePath = get_driver_parameter(
			handle, "logfile", NULL, "/var/log/"DRIVER_NAME".log");
	if (logFilePath != NULL) {
		gLogFilePath = strdup(logFilePath);
	}

	unload_driver_settings(handle);

	create_log();
}


void release_settings()
{
	if (gLogFilePath != NULL) {
		mutex_destroy(&gLogLock);
		free(gLogFilePath);
	}
}


void SiS19X_trace(bool force, const char* func, const char *fmt, ...)
{
	if (!(force || gTraceOn)) {
		return;
	}

	va_list arg_list;
	static const char *prefix = DRIVER_NAME":";
	static char buffer[1024];
	char *buf_ptr = buffer;
	if (gLogFilePath == NULL) {
		strcpy(buffer, prefix);
		buf_ptr += strlen(prefix);
	}

	if (gAddTimeStamp) {
		bigtime_t time = system_time();
		uint32 msec = time / 1000;
		uint32 sec  = msec / 1000;
		sprintf(buf_ptr, "%02ld.%02ld.%03ld:",
			sec / 60, sec % 60, msec % 1000);
		buf_ptr += strlen(buf_ptr);
	}

	if (func	!= NULL) {
		sprintf(buf_ptr, "%s::", func);
		buf_ptr += strlen(buf_ptr);
	}

	va_start(arg_list, fmt);
	vsprintf(buf_ptr, fmt, arg_list);
	va_end(arg_list);

	if (gLogFilePath == NULL) {
		dprintf(buffer);
		return;
	}

	mutex_lock(&gLogLock);
	int fd = open(gLogFilePath, O_WRONLY | O_APPEND);
	write(fd, buffer, strlen(buffer));
	close(fd);
	mutex_unlock(&gLogLock);
}


//
//	Rx/Tx traffic statistic harvesting
//

Statistics::Statistics()
{
	memset(this, 0, sizeof(Statistics));
}


void
Statistics::PutStatus(uint32 status)
{
	fInterrupts++;
	if (status & (INT_TXDONE /*| INT_TXIDLE*/)) fTxInterrupts++;
	if (status & (INT_RXDONE /*| INT_RXIDLE*/)) fRxInterrupts++;
	
	if (status & INT_TXHALT)	fTxHalt++;
	if (status & INT_RXHALT)	fRxHalt++;
	if (status & INT_TXDONE)	fTxDone++;
	if (status & INT_TXIDLE)	fTxIdle++;
	if (status & INT_RXDONE)	fRxDone++;
	if (status & INT_RXIDLE)	fRxIdle++;
	if (status & INT_LINK)		fLink++;
	if (status & INT_WAKEF)		fWakeUp++;
	if (status & INT_MAGICP)	fMagic++;
	if (status & INT_PAUSEF)	fPause++;
	if (status & INT_TIMER)		fTimer++;
	if (status & INT_SOFT)		fSoft++;
}


void
Statistics::PutTxStatus(uint32 status, uint32 size)
{
	if (status & TDS_CRS)	fCarrier++;
	if (status & TDS_FIFO)	fFIFO++;
	if (status & TDS_ABT)	fTxAbort++;
	if (status & TDS_OWC)	fWindow++;
	if ((status & txErrorStatusBits) == 0) {
		fCollisions += (status & TDS_COLLS) - 1;
		fTransmitted += (size & TxDescriptorSize);
	}
}


void
Statistics::PutRxStatus(uint32 status)
{
	if (!(status & RDS_CRCOK)) fCRC++;
	if (status & RDS_COLON) fColon++;
	if (status & RDS_NIBON) fNibon++;
	if (status & RDS_OVRUN) fOverrun++;
	if (status & RDS_MIIER) fMIIError++;
	if (status & RDS_LIMIT) fLimit++;
	if (status & RDS_SHORT) fShort++;
	if (status & RDS_ABORT) fRxAbort++;
	if ((status & rxErrorStatusBits) == 0) {
		fReceived += (status & RDS_SIZE) - 4; // exclude CRC?
	}
}


void Statistics::Trace()
{
	TRACE("Ints:%d;Lnk:%d;WkUps:%d;Mgic:%d;Pause:%d;Tmr:%d;Sft:%d\n",
			fInterrupts, fLink, fWakeUp, fMagic, fPause, fTimer, fSoft);
	
	TRACE("TX:Ints:%d;Bts:%llu;Drop:%d;Hlts:%d;Done:%d;Idle:%d;"
		"Colls:%d;Carr:%d;FIFO:%d;Abrt:%d;Wndw:%d;\n",
			fTxInterrupts, fTransmitted, fDropped, fTxHalt, fTxDone, 
			fTxIdle, fCollisions, fCarrier, fFIFO, fTxAbort, fWindow);

	TRACE("RX:Ints:%d;Bts:%llu;Hlts:%d;Done:%d;Idle:%d;CRC:%d;Cln:%d;"
		"Nibon:%d;Ovrrn:%d;MIIErr:%d;Lmt:%d;Shrt:%d;Abrt:%d\n",
			fRxInterrupts, fReceived, fRxHalt, fRxDone, fRxIdle, fCRC, fColon, 
			fNibon, fOverrun, fMIIError, fLimit, fShort, fRxAbort);
}

