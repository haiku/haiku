/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */

#include "ATATracing.h"

#include <stdarg.h>

#include <algorithm>


static char sTraceBuffer[256];
static uint32 sTraceBufferOffset = 0;


void
ata_trace_printf(uint32 flags, const char *format, ...)
{
	if (sTraceBufferOffset < sizeof(sTraceBuffer)) {
		va_list arguments;
		va_start(arguments, format);
		size_t totalBytes = vsnprintf(sTraceBuffer + sTraceBufferOffset,
			sizeof(sTraceBuffer) - sTraceBufferOffset, format, arguments);
		sTraceBufferOffset += std::min(totalBytes,
			(size_t)(sizeof(sTraceBuffer) - sTraceBufferOffset - 1));
		va_end(arguments);
	}

	if (flags & ATA_TRACE_FLUSH) {
#if ATA_TRACING
		ktrace_printf(sTraceBuffer);
#endif
		if (flags & ATA_TRACE_SYSLOG)
			dprintf("%s", sTraceBuffer);

		sTraceBufferOffset = 0;
	}
}
