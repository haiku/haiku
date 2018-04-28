
#include <stdio.h>

#ifndef NDEBUG

  #if DEBUG >= 3
	#define CALLED() 			printf("CALLED %s\n",__PRETTY_FUNCTION__)
  #else
  	#define CALLED() 			((void)0)
  #endif
	
#else

	#define CALLED()			((void)0)

#endif
