//----------------------------------------------------------------------------
// Anti-Grain Geometry - Version 2.2
// Copyright (C) 2002-2004 Maxim Shemanarev (http://www.antigrain.com)
//
// Permission to copy, use, modify, sell and distribute this software 
// is granted provided this copyright notice appears in all copies. 
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//
//----------------------------------------------------------------------------
// Contact: mcseem@antigrain.com
//          mcseemagg@yahoo.com
//          http://www.antigrain.com
//----------------------------------------------------------------------------
//
// Debuging stuff for catching memory leaks and corruptions
//
//----------------------------------------------------------------------------
#ifndef AGG_DBG_NEW_INCLUDED
#define AGG_DBG_NEW_INCLUDED

#ifdef _WIN32
#include <stdio.h>
#include <stdarg.h>
#endif

//#define AGG_DBG_NEW_CHECK_ADDR

void* operator new (unsigned size, const char* file, int line);
void* operator new [] (unsigned size, const char* file, int line);
#define AGG_DBG_NEW_OPERATOR new(__FILE__, __LINE__)

void  operator delete(void *ptr) throw();
void  operator delete [] (void *ptr) throw();

namespace agg
{
    #ifdef _WIN32
    inline void printf(char* fmt, ...)
    {
       FILE* fd = fopen("stdout.txt", "at");
       static char msg[1024];
       va_list arg;
       va_start(arg, fmt);
       vsprintf(msg, fmt, arg);
       va_end(arg);
       fputs(msg, fd);
       fclose(fd);
    }
    #endif

    enum { max_dbg_new_level = 32   };

#ifdef AGG_DBG_NEW_CHECK_ADDR
    enum { max_allocations   = 4096 };
#endif
    
    // All you need to watch for memory in heap is to declare an object 
    // of this class in your main() or whatever function you need. 
    // It will report you about all bad things happend to new/delete.
    // Try not to exceed the maximal nested level of declared watchdoggies 
    // (max_dbg_new_level)
    class watchdoggy
    {
    public:
        watchdoggy(const char* file=0, int line=0, bool report_all=false);
        ~watchdoggy();
    };
}

#define AGG_WATCHDOGGY(name, report_all) \
    agg::watchdoggy name(__FILE__, __LINE__, report_all);
#endif

#ifdef new
#undef new
#endif
#define new AGG_DBG_NEW_OPERATOR

