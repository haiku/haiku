/*
 * Copyright 2018 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		B Krishnan Iyer, krishnaniyer97@gmail.com
 */
#ifndef MMC_BUS_H
#define MMC_BUS_H


#include <new>
#include <stdio.h>
#include <string.h>

#include <lock.h>
#include <util/AutoLock.h>
#include "mmc.h"


#define MMCBUS_TRACE
#ifdef MMCBUS_TRACE
#	define TRACE(x...)		dprintf("\33[33mmmc_bus:\33[0m " x)
#else
#	define TRACE(x...)
#endif
#define TRACE_ALWAYS(x...)	dprintf("\33[33mmmc_bus:\33[0m " x)
#define ERROR(x...)			dprintf("\33[33mmmc_bus:\33[0m " x)
#define CALLED() 			TRACE("CALLED %s\n", __PRETTY_FUNCTION__)

extern device_manager_info *gDeviceManager;


class MMCBus;

class MMCBus {
public:

								MMCBus(device_node *node);
								~MMCBus();
				status_t		InitCheck();

private:
				status_t		ExecuteCommand(uint8_t command,
									uint32_t argument, uint32_t* response);
		static	status_t		WorkerThread(void*);

private:

		device_node* 			fNode;
		mmc_bus_interface* 		fController;
		void* 					fCookie;
		status_t				fStatus;
		thread_id				fWorkerThread;
};


#endif /*MMC_BUS_H*/
