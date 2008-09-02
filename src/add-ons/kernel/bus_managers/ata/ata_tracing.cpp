/*
 * Copyright 2008, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <tracing.h>
#include <string.h>

#include "ata_tracing.h"
#include "ide_internal.h"

#if ATA_TRACING

class ATATraceEntry : public AbstractTraceEntry {
	public:
		ATATraceEntry(int bus, int device, const char *info)
		: fBus(bus)
		, fDevice(device)
		, fInfo(alloc_tracing_buffer_strcpy(info, 666, false))
		{
			Initialized();
		}

		void AddDump(TraceOutput& out)
		{
			out.Print("ata %d:%d %s", fBus, fDevice, fInfo);
		}

		int fBus;
		int fDevice;
 		char *fInfo;
};


extern "C" void 
__ata_trace_device(ide_device_info *dev, const char *fmt, ...)
{
	char info[120];
	va_list ap; 

	va_start(ap, fmt); 
	vsnprintf(info, sizeof(info), fmt, ap); 
	va_end(ap);

	new(std::nothrow) ATATraceEntry(dev->bus->path_id, dev->is_device1, info);
}


extern "C" void 
__ata_trace_bus_device(ide_bus_info *bus, int dev, const char *fmt, ...)
{
	char info[120];
	va_list ap; 

	va_start(ap, fmt); 
	vsnprintf(info, sizeof(info), fmt, ap); 
	va_end(ap);

	new(std::nothrow) ATATraceEntry(bus->path_id, dev, info);
}

#endif
