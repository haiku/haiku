/*
 * Copyright 2008, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "tracing_config.h"

#if ATA_TRACING

	struct ide_bus_info;
	struct ide_device_info;

#ifdef __cplusplus
	extern "C" {
#endif

	void __ata_trace_device(struct ide_device_info *dev, const char *fmt, ...);
	void __ata_trace_bus_device(struct ide_bus_info *bus, int dev, const char *fmt, ...);

#ifdef __cplusplus
	}
#endif

	#define T(dev, args...)			__ata_trace_device(dev, args)
	#define T2(bus, dev, args...)	__ata_trace_bus_device(bus, dev, args)

#else
	
	#define T(x...)
	#define T2(x...)

#endif
