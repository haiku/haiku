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
	
#else

	#define UNIMPLEMENTED() 	((void)0)
	#define BROKEN()			((void)0)
	#define CALLED()			((void)0)
	#define FATAL				if (1) {} else printf
	#define TRACE 				if (1) {} else printf
	#define INFO				if (1) {} else printf

#endif

#endif /* _MEDIA_DEBUG_H_ */
