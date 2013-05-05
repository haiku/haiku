#ifndef _BLUETOOTH_DEBUG_SERVER_H_
#define _BLUETOOTH_DEBUG_SERVER_H_

#ifndef DEBUG
  #define DEBUG 3
#endif

#include <Debug.h>
#include <stdio.h>

#undef TRACE
#undef PRINT
#if DEBUG > 0
  inline void ERROR(const char *fmt, ...)
  { 
  	va_list ap; 
  	va_start(ap, fmt); 
  	printf("### ERROR: "); 
  	vprintf(fmt, ap); va_end(ap); 
  }
/*  
  inline void PRINT(int level, const char *fmt, ...) 
  {
  	va_list ap; 
  	if (level > DEBUG) 
  		return; 
  	va_start(ap, fmt); 
  	vprintf(fmt, ap); 
  	va_end(ap);
  }
  
  inline void PRINT(const char *fmt, ...)
  {
  	va_list ap; 
  	va_start(ap, fmt); 
  	vprintf(fmt, ap); 
  	va_end(ap);
  }*/

  #if DEBUG >= 2
	#define TRACE(a...)		printf("TRACE %s : %s\n", __PRETTY_FUNCTION__, a)
  #else
	#define TRACE(a...)		((void)0)
  #endif

  #if DEBUG >= 3
	#define END()	 		printf("ENDING %s\n",__PRETTY_FUNCTION__)
	#define CALLED() 		printf("CALLED %s\n",__PRETTY_FUNCTION__)
  #else
	#define END()			((void)0)
  	#define CALLED() 		((void)0)
  #endif
#else
	#define END()			((void)0)
	#define CALLED()		((void)0)
	#define ERROR(a...)		fprintf(stderr, a)
	#define TRACE(a...)		((void)0)
#endif

#define PRINT(l, a...)		printf(l, a)

#endif /* _BLUETOOTH_DEBUG_SERVER_H_ */
