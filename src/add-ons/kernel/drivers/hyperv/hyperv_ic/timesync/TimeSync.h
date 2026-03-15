/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HYPERV_TIMESYNC_H_
#define _HYPERV_TIMESYNC_H_


#include "ICBase.h"

#include "TimeSyncProtocol.h"


//#define TRACE_HYPERV_TIMESYNC
#ifdef TRACE_HYPERV_TIMESYNC
#	define TRACE(x...) dprintf("\33[94mhyperv_timesync:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define TRACE_ALWAYS(x...)	dprintf("\33[94mhyperv_timesync:\33[0m " x)
#define ERROR(x...)			dprintf("\33[94mhyperv_timesync:\33[0m " x)
#define CALLED(x...)		TRACE("CALLED %s\n", __PRETTY_FUNCTION__)

#define HYPERV_TIMESYNC_DRIVER_MODULE_NAME "drivers/hyperv/hyperv_ic/timesync/driver_v1"


class TimeSync : public ICBase {
public:
								TimeSync(device_node* node);

protected:
			void				OnProtocolNegotiated();
			void				OnMessageReceived(hv_ic_msg* icMessage);

private:
			void				_SyncTime(uint64 hostTime, uint64 referenceTime, uint8 flags);
};


#endif // _HYPERV_TIMESYNC_H_
