/*
 *	SiS 190/191 NIC Driver.
 *	Copyright (c) 2009 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */
#ifndef _SiS19X_SETTINGS_H_
#define _SiS19X_SETTINGS_H_


#include <driver_settings.h>

#include "Driver.h"
#include "Registers.h"


#ifdef _countof
#warning "_countof(...) WAS ALREADY DEFINED!!! Remove local definition!"
#undef _countof
#endif
#define _countof(array)(sizeof(array) / sizeof(array[0]))


void load_settings();
void release_settings();


void SiS19X_trace(bool force, const char *func, const char *fmt, ...);

#undef TRACE

#define TRACE(x...)			SiS19X_trace(false,	__func__, x)
#define TRACE_ALWAYS(x...)	SiS19X_trace(true,	__func__, x)

extern bool gTraceFlow;
#define TRACE_FLOW(x...)	SiS19X_trace(gTraceFlow, NULL, x)

#define TRACE_RET(result)	SiS19X_trace(false, __func__, \
	"Returns:%#010x\n", result);


#define STATISTICS	0

struct Statistics {
					Statistics();

		void		PutStatus(uint32 status);
		void		PutTxStatus(uint32 status, uint32 size);
		void		PutRxStatus(uint32 status);
		void		Trace();


			// shared
		uint32		fInterrupts;
		uint32		fLink;
		uint32		fWakeUp;
		uint32		fMagic;
		uint32		fPause;
		uint32		fTimer;
		uint32		fSoft;
			// transmit
		uint32		fTxInterrupts;
		uint32		fTxHalt;
		uint32		fTxDone;
		uint32		fTxIdle;
		uint32		fCollisions;
		uint32		fCarrier;
		uint32		fFIFO;
		uint32		fTxAbort;
		uint32		fWindow;
		uint32		fDropped;
		uint64		fTransmitted;

			// receive
		uint32		fRxInterrupts;
		uint32		fRxHalt;
		uint32		fRxDone;
		uint32		fRxIdle;
		uint32		fCRC;
		uint32		fColon;
		uint32		fNibon;
		uint32		fOverrun;
		uint32		fMIIError;
		uint32		fLimit;
		uint32		fShort;
		uint32		fRxAbort;
		uint64		fReceived;
};


#endif /*_SiS19X_SETTINGS_H_*/

