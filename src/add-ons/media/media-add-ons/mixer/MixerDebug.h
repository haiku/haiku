/*
 * Copyright 2002 David Shipman,
 * Copyright 2003-2007 Marcus Overhagen
 * Copyright 2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MIXER_DEBUG_H_
#define _MIXER_DEBUG_H_


#ifndef DEBUG
#	define DEBUG 0
#endif

#include <Debug.h>
#include <stdio.h>


#undef TRACE
#undef PRINT


#if DEBUG > 0
  inline void ERROR(const char *fmt, ...) { va_list ap; va_start(ap, fmt); printf("### ERROR: "); vprintf(fmt, ap); va_end(ap); }
  inline void PRINT(int level, const char *fmt, ...) { va_list ap; if (level > DEBUG) return; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); }

#	define PRINT_FORMAT(_text, _fmt)	do { char _buf[300]; string_for_format((_fmt), _buf, sizeof(_buf)); printf("%s %s\n", (_text), (_buf)); } while (0)
#	define PRINT_INPUT(_text, _in)		do { char _buf[300]; string_for_format((_in).format, _buf, sizeof(_buf)); printf("%s node(node %ld, port %ld); source(port %ld, id %ld); dest(port %ld, id %ld); fmt(%s); name(%s)\n", (_text), (_in).node.node, (_in).node.port, (_in).source.port, (_in).source.id, (_in).destination.port, (_in).destination.id, _buf, (_in).name); } while (0)
#	define PRINT_OUTPUT(_text, _out)	do { char _buf[300]; string_for_format((_out).format, _buf, sizeof(_buf)); printf("%s node(node %ld, port %ld); source(port %ld, id %ld); dest(port %ld, id %ld); fmt(%s); name(%s)\n", (_text), (_out).node.node, (_out).node.port, (_out).source.port, (_out).source.id, (_out).destination.port, (_out).destination.id, _buf, (_out).name); } while (0)
#	define PRINT_CHANNEL_MASK(fmt)		do { char s[200]; StringForChannelMask(s, (fmt).u.raw_audio.channel_mask); printf(" channel_mask 0x%08lX %s\n", (fmt).u.raw_audio.channel_mask, s); } while (0)

#	if DEBUG >= 2
#		define TRACE 				printf
#	else
#		define TRACE(a...)			((void)0)
#	endif
#else
#	define PRINT_FORMAT(_text, _fmt)	((void)0)
#	define PRINT_INPUT(_text, _in)		((void)0)
#	define PRINT_OUTPUT(_text, _out)	((void)0)
#	define PRINT_CHANNEL_MASK(fmt)		((void)0)
#	define PRINT(l, a...)				((void)0)
#	define ERROR(a...)					((void)0)
#	define TRACE(a...)					((void)0)
#endif

#endif /* _MIXER_DEBUG_H_ */
