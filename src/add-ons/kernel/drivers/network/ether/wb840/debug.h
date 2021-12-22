#ifndef __MYDEBUG_H
#define __MYDEBUG_H

#define ARGS (const char*, ...)

#ifdef DEBUG
	#define LOG(ARGS) dprintf ARGS
#else
	#define LOG(ARGS) 
#endif // DEBUG

#endif //__MYDEBUG_H
