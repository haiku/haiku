
#include <stdio.h>

#ifndef NDEBUG

  #ifndef DEBUG
  	#define DEBUG 2
  #endif

  #if DEBUG >= 1
	#define UNIMPLEMENTED()		printf("UNIMPLEMENTED %s\n",__PRETTY_FUNCTION__)
  #else
  	#define UNIMPLEMENTED()		((void)0)
  #endif

  #if DEBUG >= 2
	#define BROKEN()			printf("BROKEN %s\n",__PRETTY_FUNCTION__)
  #else
  	#define BROKEN()			((void)0)
  #endif

  #if DEBUG >= 3
	#define CALLED() 			printf("CALLED %s\n",__PRETTY_FUNCTION__)
  #else
  	#define CALLED() 			((void)0)
  #endif
	
#else

	#define UNIMPLEMENTED() 	((void)0)
	#define BROKEN()			((void)0)
	#define CALLED()			((void)0)

#endif
