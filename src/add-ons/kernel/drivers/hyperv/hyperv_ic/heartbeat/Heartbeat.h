/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HYPERV_HEARTBEAT_H_
#define _HYPERV_HEARTBEAT_H_


#include "ICBase.h"

#include "HeartbeatProtocol.h"


//#define TRACE_HYPERV_HEARTBEAT
#ifdef TRACE_HYPERV_HEARTBEAT
#	define TRACE(x...) dprintf("\33[94mhyperv_heartbeat:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define TRACE_ALWAYS(x...)	dprintf("\33[94mhyperv_heartbeat:\33[0m " x)
#define ERROR(x...)			dprintf("\33[94mhyperv_heartbeat:\33[0m " x)
#define CALLED(x...)		TRACE("CALLED %s\n", __PRETTY_FUNCTION__)

#define HYPERV_HEARTBEAT_DRIVER_MODULE_NAME "drivers/hyperv/hyperv_ic/heartbeat/driver_v1"


class Heartbeat : public ICBase {
public:
								Heartbeat(device_node* node);

protected:
			void				OnProtocolNegotiated();
			void				OnMessageReceived(hv_ic_msg* icMessage);
};


#endif // _HYPERV_HEARTBEAT_H_
