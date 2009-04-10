/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */
#ifndef ATA_TRACING_H
#define ATA_TRACING_H

#include <tracing.h>

#define ATA_TRACE_START			0x00
#define ATA_TRACE_FLUSH			0x01
#define	ATA_TRACE_SYSLOG		0x02
#define ATA_TRACE_FLUSH_SYSLOG	0x03

#if ATA_TRACING
#define TRACE(x...)				{ \
									ata_trace_printf(ATA_TRACE_START, \
										"ata%s: ", _DebugContext()); \
									ata_trace_printf(ATA_TRACE_FLUSH, x); \
								}
#define TRACE_FUNCTION(x...)	{ \
									ata_trace_printf(ATA_TRACE_START, \
										"ata%s: %s: ", _DebugContext(), \
										__FUNCTION__); \
									ata_trace_printf(ATA_TRACE_FLUSH, x); \
								}
#else
#define TRACE(x...)				/* nothing */
#define TRACE_FUNCTION(x...)	/* nothing */
#endif

#define TRACE_ALWAYS(x...)		{ \
									ata_trace_printf(ATA_TRACE_START, \
										"ata%s: ", _DebugContext()); \
									ata_trace_printf(ATA_TRACE_FLUSH_SYSLOG, x); \
								}
#define TRACE_ERROR(x...)		{ \
									ata_trace_printf(ATA_TRACE_START, \
										"ata%s error: ", _DebugContext()); \
									ata_trace_printf(ATA_TRACE_FLUSH_SYSLOG, x); \
								}

void ata_trace_printf(uint32 flags, const char *format, ...);

inline const char *
_DebugContext()
{
	return "";
}

#endif // ATA_TRACING_H
