#ifndef _MEDIA_DEBUG_H_
#define _MEDIA_DEBUG_H_

#include <Debug.h>
#include <stdio.h>

#undef TRACE	

#ifndef DEBUG
  #define DEBUG 0
#endif

#ifndef NDEBUG

  #if DEBUG >= 1
	#define UNIMPLEMENTED()		printf("UNIMPLEMENTED %s\n",__PRETTY_FUNCTION__)
	#define FATAL				printf
  #else
  	#define UNIMPLEMENTED()		((void)0)
	#define FATAL				if (1) {} else printf
  #endif

  #if DEBUG >= 2
	#define BROKEN()			printf("BROKEN %s\n",__PRETTY_FUNCTION__)
	#define TRACE 				printf
  #else
  	#define BROKEN()			((void)0)
	#define TRACE 				if (1) {} else printf
  #endif

  #if DEBUG >= 3
	#define CALLED() 			printf("CALLED %s\n",__PRETTY_FUNCTION__)
	#define INFO				printf
  #else
  	#define CALLED() 			((void)0)
	#define INFO				if (1) {} else printf
  #endif

  #if DEBUG >= 1
	#define PRINT_FORMAT(_text, _fmt)	do { char _buf[100]; string_for_format((_fmt), _buf, sizeof(_buf)); printf("%s %s\n", (_text), (_buf)); } while (0)
	#define PRINT_INPUT(_text, _in) 	do { char _buf[100]; string_for_format((_in).format, _buf, sizeof(_buf)); printf("%s node(node %ld, port %ld); source(port %ld, id %ld); dest(port %ld, id %ld); fmt(%s); name(%s)\n", (_text), (_in).node.node, (_in).node.port, (_in).source.port, (_in).source.id, (_in).destination.port, (_in).destination.id, _buf, (_in).name); } while (0)
	#define PRINT_OUTPUT(_text, _out)	do { char _buf[100]; string_for_format((_out).format, _buf, sizeof(_buf)); printf("%s node(node %ld, port %ld); source(port %ld, id %ld); dest(port %ld, id %ld); fmt(%s); name(%s)\n", (_text), (_out).node.node, (_out).node.port, (_out).source.port, (_out).source.id, (_out).destination.port, (_out).destination.id, _buf, (_out).name); } while (0)
  #else
	#define PRINT_FORMAT(_text, _fmt)	((void)0)
	#define PRINT_INPUT(_text, _in)		((void)0)
	#define PRINT_OUTPUT(_text, _out)	((void)0)
  #endif

#else

	#define PRINT_FORMAT(_text, _fmt)	((void)0)
	#define PRINT_INPUT(_text, _in)		((void)0)
	#define PRINT_OUTPUT(_text, _out)	((void)0)
	#define UNIMPLEMENTED() 	((void)0)
	#define BROKEN()			((void)0)
	#define CALLED()			((void)0)
	#define FATAL				if (1) {} else printf
	#define TRACE 				if (1) {} else printf
	#define INFO				if (1) {} else printf

#endif

#endif /* _MEDIA_DEBUG_H_ */
