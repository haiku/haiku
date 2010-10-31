/*
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 * 
 */

#ifndef _USBVISION_TRACING_H_
#define _USBVISION_TRACING_H_
  
void load_setting(void);
void create_log(void);
void usbvision_trace(bool b_force, char *fmt, ...);

#define TRACE_ALWAYS(x...) usbvision_trace(true, x);
#define TRACE(x...) usbvision_trace(false, x);

extern bool b_log_funcalls;
#define TRACE_FUNCALLS(x...)\
        { if(b_log_funcalls) usbvision_trace(false, x);}

extern bool b_log_funcret;
#define TRACE_FUNCRET(x...)\
        { if(b_log_funcret) usbvision_trace(false, x);}

extern bool b_log_funcres;
#define TRACE_FUNCRES(func, param)\
        { if(b_log_funcres) func(param);}

void trace_reginfo(xet_nt100x_reg *reginfo);

#endif /*_USBVISION_TRACING_H_*/ 
