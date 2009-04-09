/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */
#ifndef ATA_TRACING_H
#define ATA_TRACING_H

//#define TRACE_ATA
#ifdef TRACE_ATA
#define TRACE(x...)				{ \
									dprintf("ata%s: ", _DebugContext()); \
									dprintf(x); \
								}
#define TRACE_FUNCTION(x...)	{ \
									dprintf("ata%s: %s: ", _DebugContext(), \
										__FUNCTION__); \
									dprintf(x); \
								}
#else
#define TRACE(x...)				/* nothing */
#define TRACE_FUNCTION(x...)	/* nothing */
#endif

#define TRACE_ALWAYS(x...)		{ \
									dprintf("ata%s: ", _DebugContext()); \
									dprintf(x); \
								}
#define TRACE_ERROR(x...)		{ \
									dprintf("ata%s error: ", _DebugContext()); \
									dprintf(x); \
								}

inline const char *
_DebugContext()
{
	return "";
}

#endif // ATA_TRACING_H
