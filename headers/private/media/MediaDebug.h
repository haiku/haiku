#ifndef _MEDIA_DEBUG_OLD_H_
#define _MEDIA_DEBUG_OLD_H_

#ifndef DEBUG
  #define DEBUG 0
#endif

#include <Debug.h>
#include <stdio.h>

#undef TRACE
#undef PRINT

#if DEBUG > 0

  #define UNIMPLEMENTED()		printf("UNIMPLEMENTED %s\n",__PRETTY_FUNCTION__)
  inline void ERROR(const char *fmt, ...) { va_list ap; va_start(ap, fmt); printf("### ERROR: "); vprintf(fmt, ap); va_end(ap); }
  inline void PRINT(int level, const char *fmt, ...) { va_list ap; if (level > DEBUG) return; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); }

  #define PRINT_FORMAT(_text, _fmt)	do { char _buf[300]; string_for_format((_fmt), _buf, sizeof(_buf)); printf("%s %s\n", (_text), (_buf)); } while (0)
  #define PRINT_INPUT(_text, _in) 	do { char _buf[300]; string_for_format((_in).format, _buf, sizeof(_buf)); printf("%s node(node %" B_PRId32 ", port %" B_PRId32 "); source(port %" B_PRId32 ", id %" B_PRId32 "); dest(port %" B_PRId32 ", id %" B_PRId32 "); fmt(%s); name(%s)\n", (_text), (_in).node.node, (_in).node.port, (_in).source.port, (_in).source.id, (_in).destination.port, (_in).destination.id, _buf, (_in).name); } while (0)
  #define PRINT_OUTPUT(_text, _out)	do { char _buf[300]; string_for_format((_out).format, _buf, sizeof(_buf)); printf("%s node(node %" B_PRId32 ", port %" B_PRId32 "); source(port %" B_PRId32 ", id %" B_PRId32 "); dest(port %" B_PRId32 ", id %" B_PRId32 "); fmt(%s); name(%s)\n", (_text), (_out).node.node, (_out).node.port, (_out).source.port, (_out).source.id, (_out).destination.port, (_out).destination.id, _buf, (_out).name); } while (0)
  
  #if DEBUG >= 2
	#define BROKEN()			printf("BROKEN %s\n",__PRETTY_FUNCTION__)
	#define TRACE 				printf
  #else
  	#define BROKEN()			((void)0)
	#define TRACE(a...)			((void)0)
  #endif

  #if DEBUG >= 3
	#define CALLED() 			printf("CALLED %s\n",__PRETTY_FUNCTION__)
  #else
  	#define CALLED() 			((void)0)
  #endif

#else

	#define PRINT_FORMAT(_text, _fmt)	((void)0)
	#define PRINT_INPUT(_text, _in)		((void)0)
	#define PRINT_OUTPUT(_text, _out)	((void)0)
	#define UNIMPLEMENTED() 			printf("UNIMPLEMENTED %s\n",__PRETTY_FUNCTION__)
	#define BROKEN()					((void)0)
	#define CALLED()					((void)0)
	#define PRINT(l, a...)				((void)0)
	#define ERROR(a...)					fprintf(stderr, a)
	#define TRACE(a...)					((void)0)

#endif

#endif /* _MEDIA_DEBUG_H_ */
