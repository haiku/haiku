#ifndef __DEBUG_H
#define __DEBUG_H

#include <KernelExport.h>

#define DEBUG

#ifdef DEBUG
	#define LOG dprintf
#else
	#define LOG(a...)
#endif

#endif
