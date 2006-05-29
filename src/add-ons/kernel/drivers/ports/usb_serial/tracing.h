/*
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 * 
 */

#ifndef _USB_SERIAL_TRACING_H_ 
  #define _USB_SERIAL_TRACING_H_
  
void load_setting();
void create_log();
void usb_serial_trace(bool b_force, char *fmt, ...);

#define TRACE_ALWAYS(x...) usb_serial_trace(true, x);
#define TRACE(x...) usb_serial_trace(false, x);

extern bool b_log_funcalls;
#define TRACE_FUNCALLS(x...)\
        { if(b_log_funcalls) usb_serial_trace(false, x);}

extern bool b_log_funcret;
#define TRACE_FUNCRET(x...)\
        { if(b_log_funcret) usb_serial_trace(false, x);}

extern bool b_log_funcres;
#define TRACE_FUNCRES(func, param)\
        { if(b_log_funcres) func(param);}

void trace_ddomain(struct ddomain *dd);
void trace_termios(struct termios *tios);
void trace_str(struct str *str);
void trace_winsize(struct winsize *ws);
void trace_tty(struct tty *tty);

extern bool b_acm_support;

#endif//_USB_SERIAL_TRACING_H_ 
