/*
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 * 
 */

#include <Drivers.h>
#include <USB.h>
#include <ttylayer.h>

#include <stdio.h> //sprintf
#include <unistd.h> //posix file i/o - create, write, close 
#include <driver_settings.h>

 
#include "driver.h"
//#include "dano_hack.h"

#if DEBUG
bool b_log = true;
#else
bool b_log = false;
#endif
bool b_log_file = false;
bool b_log_append = false;
bool b_log_funcalls = false;
bool b_log_funcret  = false;
bool b_log_funcres  = false;
bool b_acm_support = true;

static const char *private_log_path="/boot/home/"DRIVER_NAME".log";
static sem_id loglock;

void load_setting(){
  void *settingshandle; 
  settingshandle = load_driver_settings(DRIVER_NAME); 
#if !DEBUG
  b_log = get_driver_boolean_parameter(
            settingshandle, "debug_output", b_log, true);
#endif
  b_log_file = get_driver_boolean_parameter(
            settingshandle, "debug_output_in_file", b_log_file, true);
  b_log_append = ! get_driver_boolean_parameter(
            settingshandle, "debug_output_file_rewrite", !b_log_append, true);
  b_log_funcalls = get_driver_boolean_parameter(
            settingshandle, "debug_trace_func_calls", b_log_funcalls, false);
  b_log_funcret = get_driver_boolean_parameter(
            settingshandle, "debug_trace_func_returns", b_log_funcret, false);
  b_log_funcres = get_driver_boolean_parameter(
            settingshandle, "debug_trace_func_results", b_log_funcres, false);
  b_acm_support = get_driver_boolean_parameter(
            settingshandle, "support_acm_devices", b_acm_support, true);

  unload_driver_settings(settingshandle);
}

void create_log(){
  int flags = O_WRONLY | O_CREAT | ((!b_log_append) ? O_TRUNC : 0);
  if(!b_log_file)
    return;
  close(open(private_log_path, flags, 0666));
  loglock = create_sem(1, DRIVER_NAME"-logging");
}

void usb_serial_trace(bool b_force, char *fmt, ...){
  if(!(b_force || b_log))
    return;
  {
    va_list arg_list;
    static char *prefix = "\33[32m"DRIVER_NAME":\33[0m";
    static char buf[1024];
    char *buf_ptr = buf;
    if(!b_log_file){
      strcpy(buf, prefix);
      buf_ptr += strlen(prefix);
    }

    va_start(arg_list, fmt);
    vsprintf(buf_ptr, fmt, arg_list);
    va_end(arg_list);

    if(b_log_file){
      int fd;
      acquire_sem(loglock);
      fd = open(private_log_path, O_WRONLY | O_APPEND);
      write(fd, buf, strlen(buf));
      close(fd);
      release_sem(loglock);
    }
    else
      dprintf(buf);
  }
}

void trace_ddomain(struct ddomain *dd){
  TRACE("struct ddomain\n"
        " ddrover: %08x\n"
        " bg:%d, locked:%d\n", dd->r, dd->bg, dd->locked);
}

void trace_termios(struct termios *tios){
  TRACE("struct termios\n"
        "  c_iflag:%08x\n"
        "  c_oflag:%08x\n"
        "  c_cflag:%08x\n"
        "  c_lflag:%08x\n"
        "  c_line: %08x\n"
        "  c_ixxxxx:%08x\n"
        "  c_oxxxxx:%08x\n"
        "  c_cc [%02x, %02x, %02x, %02x, %02x, %02x, "
        "%02x, %02x, %02x, %02x, %02x]\n",
           tios->c_iflag, tios->c_oflag, tios->c_cflag, tios->c_lflag, 
           tios->c_line, tios->c_ixxxxx, tios->c_oxxxxx, 
           tios->c_cc[0], tios->c_cc[1], tios->c_cc[2], tios->c_cc[3], 
           tios->c_cc[4], tios->c_cc[5], tios->c_cc[6], tios->c_cc[7], 
           tios->c_cc[8], tios->c_cc[9], tios->c_cc[10]);
}

void trace_str(struct str *str){
  TRACE("struct str\n"
        "  buffer:%08x\n"
        "  bufsize:%d\n"
        "  count:%d\n"
        "  tail:%d\n"
        "  allocated: %d\n",
           str->buffer,
           str->bufsize, str->count, str->tail,
           str->allocated); 
}

void trace_winsize(struct winsize *ws){
  TRACE("struct winsize\n"
        "  ws_row:%d\n"
        "  ws_col:%d\n"
        "  ws_xpixel:%d\n"
        "  ws_ypixel:%d\n",
           ws->ws_row, ws->ws_col, ws->ws_xpixel, ws->ws_ypixel);
}

void trace_tty(struct tty *tty){
  TRACE("\n>> STRUCT tty\n"
        "  nopen:%d, flags:%08x,\n",
        tty->nopen, tty->flags);

  TRACE("\n  ddomain dd:\n");
  trace_ddomain(&tty->dd);
  TRACE("\n  ddomain ddi:\n");
  trace_ddomain(&tty->ddi);

  TRACE("  pgid: %08x\n", tty->pgid);
  
  TRACE("\n termios t:");
  trace_termios(&tty->t);

  TRACE("  iactivity: %d, ibusy:%d\n", tty->iactivity, tty->ibusy);
                        
  TRACE("\n  str istr:\n");
  trace_str(&tty->istr);
  TRACE("\n  str rstr:\n");
  trace_str(&tty->rstr);
  TRACE("\n  str ostr:\n");
  trace_str(&tty->ostr);
                  
  TRACE("\n  winsize wsize:\n");
  trace_winsize(&tty->wsize);
        
  TRACE("  service: %08x\n", tty->service);
  TRACE("\n<< STRUCT tty\n");
}
