
#include <stdio.h>

#ifndef NDEBUG

  #ifndef DEBUG
  	#define DEBUG 2
  #endif

  #if DEBUG >= 1
	#define UNIMPLEMENTED()		printf("libmedia.so: UNIMPLEMENTED %s\n",__PRETTY_FUNCTION__)
  #else
  	#define UNIMPLEMENTED()		((void)0)
  #endif

  #if DEBUG >= 2
	#define BROKEN()			printf("libmedia.so: BROKEN %s\n",__PRETTY_FUNCTION__)
  #else
  	#define BROKEN()			((void)0)
  #endif

  #if DEBUG >= 3
	#define CALLED() 			printf("libmedia.so: CALLED %s\n",__PRETTY_FUNCTION__)
  #else
  	#define CALLED() 			((void)0)
  #endif
	
	#undef TRACE	
	#define TRACE \
		printf

#else

	#define UNIMPLEMENTED() 	((void)0)
	#define BROKEN()			((void)0)
	#define CALLED()			((void)0)
	#define TRACE \
		if (1) {} else printf

#endif
