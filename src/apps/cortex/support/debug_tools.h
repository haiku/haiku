// debug_tools.h

#ifndef __CORTEX_DEBUGTOOLS_H__
#define __CORTEX_DEBUGTOOLS_H__

// export definition
#define EXPORTFN	__declspec(dllexport)

#include <Debug.h>
#include <cstdio>

#if !NDEBUG
#define FPRINTF fprintf
#else
#define FPRINTF
#endif

#define D_PRINTF			FPRINTF

#include <iostream>

#include <stdexcept>

#include <Point.h>
#include <Rect.h>

std::string point_to_string(const BPoint& p);
std::string rect_to_string(const BRect& r);
std::string id_to_string(uint32 n);

void show_alert(char* pFormat, ...); //nyi

#endif /* __CORTEX_DEBUGTOOLS_H__ */
