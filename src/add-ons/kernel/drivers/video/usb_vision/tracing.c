/*
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 * 
 */

#include <OS.h>
#include <KernelExport.h>

#include <USB.h>
//#include <CAM.h>
#include <stdio.h> //sprintf
#include <string.h> //strcpy
#include <unistd.h> //posix file i/o - create, write, close 
#include <directories.h>
#include <driver_settings.h>

#include "usb_vision.h"
#include "tracing.h"

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

bool b_log_settings_loaded = false;

static const char *private_log_path
	= kSystemLogDirectory "/" DRIVER_NAME ".log";
static sem_id loglock;

void load_setting(void){
  if(!b_log_settings_loaded){
    void *settingshandle; 
    settingshandle = load_driver_settings(DRIVER_NAME); 
#if !DEBUG
    b_log = get_driver_boolean_parameter(settingshandle, "debug_output", b_log, true);
#endif
    b_log_file = get_driver_boolean_parameter(settingshandle, "debug_output_in_file", b_log_file, true);
    b_log_append = ! get_driver_boolean_parameter(settingshandle, "debug_output_file_rewrite", !b_log_append, true);
    b_log_funcalls = get_driver_boolean_parameter(settingshandle, "debug_trace_func_calls", b_log_funcalls, false);
    b_log_funcret = get_driver_boolean_parameter(settingshandle, "debug_trace_func_returns", b_log_funcret, false);
    b_log_funcres = get_driver_boolean_parameter(settingshandle, "debug_trace_func_results", b_log_funcres, false);
    unload_driver_settings(settingshandle);
    b_log_settings_loaded = true;
  }
}

void create_log(void){
  int flags = O_WRONLY | O_CREAT | ((!b_log_append) ? O_TRUNC : 0);
  if(!b_log_file)
    return;
  close(open(private_log_path, flags, 0666));
  loglock = create_sem(1, DRIVER_NAME"-logging");
}

void usbvision_trace(bool b_force, char *fmt, ...){
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
    
/*    {
    bigtime_t time = system_time();
    uint32 msec = time / 1000;
    uint32 sec  = msec / 1000;
    sprintf(buf_ptr, "%02d.%02d.%03d:", sec / 60, sec % 60, msec % 1000);
    buf_ptr += strlen(buf_ptr);
    }
*/
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
      dprintf("%s", buf);
  }
}

void trace_reginfo(xet_nt100x_reg *ri)
{
  int i;
  TRACE("struct set_nt100x_reg\n"
        "  reg:%02x\n"
        "  data_length:%d\n",
        ri->reg, ri->data_length);
  for(i = 0; i < ri->data_length; i++)
    TRACE("%02x ", ri->data[i]);
  TRACE("\n");    
}
