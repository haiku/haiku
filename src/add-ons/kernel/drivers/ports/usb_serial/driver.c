/*
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 * 
 */

#include <KernelExport.h>
#include <Drivers.h>
#include <USB.h>
#include <ttylayer.h>
#include <malloc.h>
#include <string.h> /* strerror */
#include <stdlib.h> /* strtol */
#include <stdio.h>  /* sprintf */

#include "driver.h"
#include "acm.h"
#include "prolific.h"
#include "ftdi.h"
 
/* driver callbacks forward declarations */
static status_t usb_serial_open (const char *name, uint32 flags, void** cookie);
static status_t usb_serial_read (void* cookie,
                                 off_t position, void *buf, size_t* num_bytes);
static status_t usb_serial_write (void* cookie, off_t position,
                                  const void* buffer, size_t* num_bytes);
static status_t usb_serial_select (void* cookie, uint8 event, uint32 ref, selectsync *_sync);
static status_t usb_serial_deselect (void* cookie, uint8 event, selectsync *_sync);
static status_t usb_serial_control (void* cookie, uint32 op, void* arg, size_t len);
static status_t usb_serial_close (void* cookie);
static status_t usb_serial_free (void* cookie);
/* USB notify callbacks forward declarations */
status_t usb_serial_device_added(const usb_device *dev, void **cookie);
status_t usb_serial_device_removed(void *cookie);

/* The base name of drivers created in /dev/ports/ directory */
#define BASENAME_LEN 0x09 /*must be synchronized with below !!!*/
static const char *basename = "ports/usb%u";

/* function pointers for the device hooks entry points */
device_hooks usb_serial_hooks = {
  usb_serial_open,            /* -> open entry point */
  usb_serial_close,           /* -> close entry point */
  usb_serial_free,            /* -> free cookie */
  usb_serial_control,         /* -> control entry point */
  usb_serial_read,            /* -> read entry point */
  usb_serial_write,            /* -> write entry point */
#if defined(B_BEOS_VERSION_DANO) || defined(__HAIKU__)
  usb_serial_select,           /* -> select entry point */
  usb_serial_deselect         /* -> deselect entry point */
#endif
  /* readv / read_pages ??? */
  /* writev */
};
/* USB notify hooks */
static usb_notify_hooks notify_hooks = {
    &usb_serial_device_added,
    &usb_serial_device_removed
}; 
/* kernel modules */
usb_module_info *usb_m; /* usb layer */
tty_module_info *tty_m; /* tty layer */

struct tty usb_serial_tty[DEVICES_COUNT]; 
struct ddomain usb_serial_dd;
/* hardware devices, supported by this driver.
   See description field for human readable names */
usb_serial_hw usb_serial_hw_devices[] = {
  {0, 0, add_acm_device, reset_acm_device,
         set_line_coding_acm, set_control_line_state_acm,
         on_read_acm, on_write_acm, on_close_acm,
         "CDC ACM compatible device"},
         
  {VND_PROLIFIC, PROD_PROLIFIC_RSAQ2,
           add_prolific_device, reset_prolific_device,
           set_line_coding_acm, set_control_line_state_acm,
           on_read_acm, on_write_acm, on_close_acm,
           "PL2303 Serial adapter (IODATA USB-RSAQ2)"},
  {VND_IODATA,   PROD_IODATA_USBRSAQ,
           add_prolific_device, reset_prolific_device,
           set_line_coding_acm, set_control_line_state_acm, 
           on_read_acm, on_write_acm, on_close_acm,
           "I/O Data USB serial adapter USB-RSAQ1"},
  {VND_ATEN,     PROD_ATEN_UC232A,
           add_prolific_device, reset_prolific_device,
           set_line_coding_acm, set_control_line_state_acm, 
           on_read_acm, on_write_acm, on_close_acm,
           "Aten Serial adapter"},
  {VND_TDK,      PROD_TDK_UHA6400,
           add_prolific_device, reset_prolific_device,
           set_line_coding_acm, set_control_line_state_acm, 
           on_read_acm, on_write_acm, on_close_acm,
           "TDK USB-PHS Adapter UHA6400"},
  {VND_RATOC,    PROD_RATOC_REXUSB60,
           add_prolific_device, reset_prolific_device,
           set_line_coding_acm, set_control_line_state_acm, 
           on_read_acm, on_write_acm, on_close_acm,
           "Ratoc USB serial adapter REX-USB60"},
  {VND_PROLIFIC, PROD_PROLIFIC_PL2303,
           add_prolific_device, reset_prolific_device,
           set_line_coding_acm, set_control_line_state_acm, 
           on_read_acm, on_write_acm, on_close_acm,
           "PL2303 Serial adapter (ATEN/IOGEAR UC232A)"},
  {VND_ELECOM,   PROD_ELECOM_UCSGT,
           add_prolific_device, reset_prolific_device,
           set_line_coding_acm, set_control_line_state_acm, 
           on_read_acm, on_write_acm, on_close_acm,
           "Elecom UC-SGT"}, 
  {VND_SOURCENEXT,   PROD_SOURCENEXT_KEIKAI8,
           add_prolific_device, reset_prolific_device,
           set_line_coding_acm, set_control_line_state_acm, 
           on_read_acm, on_write_acm, on_close_acm,
           "SOURCENEXT KeikaiDenwa 8"}, 
  {VND_SOURCENEXT,   PROD_SOURCENEXT_KEIKAI8_CHG,
           add_prolific_device, reset_prolific_device,
           set_line_coding_acm, set_control_line_state_acm, 
           on_read_acm, on_write_acm, on_close_acm,
           "SOURCENEXT KeikaiDenwa 8 with charger"}, 
  {VND_HAL,   PROD_HAL_IMR001,
           add_prolific_device, reset_prolific_device,
           set_line_coding_acm, set_control_line_state_acm, 
           on_read_acm, on_write_acm, on_close_acm,
           "HAL Corporation Crossam2+USB"}, 

  {VND_FTDI,   PROD_8U100AX,
           add_ftdi_device, reset_ftdi_device,
           set_line_coding_ftdi, set_control_line_state_ftdi, 
           on_read_ftdi, on_write_ftdi, on_close_ftdi,
           "FTDI 8U100AX serial converter"}, 
  {VND_FTDI,   PROD_8U232AM,
           add_ftdi_device, reset_ftdi_device,
           set_line_coding_ftdi, set_control_line_state_ftdi,
           on_read_ftdi, on_write_ftdi, on_close_ftdi,
           "FTDI 8U232AM serial converter"}, 
};
/* supported devices*/
usb_support_descriptor *supported_devices;
#if 0
usb_support_descriptor supported_devices[] = {
  {USB_DEV_CLASS_COMM, 0, 0, 0, 0},
  {0, 0, 0, VND_PROLIFIC, PROD_PROLIFIC_RSAQ2 },
  {0, 0, 0, VND_IODATA,   PROD_IODATA_USBRSAQ },
  {0, 0, 0, VND_ATEN,     PROD_ATEN_UC232A },
  {0, 0, 0, VND_TDK,      PROD_TDK_UHA6400 },
  {0, 0, 0, VND_RATOC,    PROD_RATOC_REXUSB60 },
  {0, 0, 0, VND_PROLIFIC, PROD_PROLIFIC_PL2303 },
  {0, 0, 0, VND_ELECOM,   PROD_ELECOM_UCSGT },
  
  {0, 0, 0, VND_FTDI, PROD_8U100AX },
  {0, 0, 0, VND_FTDI, PROD_8U232AM },
};
#endif
/* main devices table locking semaphore */
sem_id usb_serial_lock = -1;
/* array of pointers to device objects */
usb_serial_device *usb_serial_devices[DEVICES_COUNT];
/* the names of "ports" */
char * usb_serial_names[DEVICES_COUNT + 1];
/* speed constants, supported by this driver. */
const uint32 serial_tty_speed[] = {
    0x00000000, //B0
    0x00000032, //B50
    0x0000004B, //B75
    0x0000006E, //B110
    0x00000086, //B134
    0x00000096, //B150
    0x000000C8, //B200
    0x0000012C, //B300
    0x00000258, //B600
    0x000004B0, //B1200
    0x00000708, //B1800
    0x00000960, //B2400
    0x000012C0, //B4800
    0x00002580, //B9600
    0x00004B00, //B19200
    0x00009600, //B38400
    0x0000E100, //B57600
    0x0001C200, //B115200
    0x00038400, //B230400
    0x00070800, //460800
    0x000E1000, //921600
};

/* init_hardware - called once the first time the driver is loaded */
status_t init_hardware (void){
  TRACE("init_hardware\n"); /*special case - no file-logging activated now*/
  return B_OK;
}

/* init_driver - optional function - called every time the driver
   is loaded. */
status_t init_driver (void){
  int i;
  status_t status = B_OK;
  load_setting();
  create_log();

  TRACE_FUNCALLS("init_driver\n");
  
  if((status = get_module(B_TTY_MODULE_NAME, (module_info **)&tty_m)) == B_OK){
    if((status = get_module(B_USB_MODULE_NAME, (module_info **)&usb_m)) == B_OK){
      if(tty_m && usb_m){
        for(i = 0; i < DEVICES_COUNT; i++)
          usb_serial_devices[i] = 0;
          
        usb_serial_names[0] = NULL;  
        load_driver_symbols(DRIVER_NAME);

        /* XXX: needs more error checking! */

        // build the list of usb supported devices from our own hw list,
        // so it's always up to date.
        supported_devices = malloc(sizeof(usb_support_descriptor) * SIZEOF(usb_serial_hw_devices));
        // ACM devices
        supported_devices[0].dev_class = USB_DEV_CLASS_COMM;
        supported_devices[0].dev_subclass = 0;
        supported_devices[0].dev_protocol = 0;
        supported_devices[0].vendor = 0;
        supported_devices[0].product = 0;
        // other devices
        for (i = 1; i < SIZEOF(usb_serial_hw_devices); i++){
          supported_devices[i].dev_class = 0;
          supported_devices[i].dev_subclass = 0;
          supported_devices[i].dev_protocol = 0;
          supported_devices[i].vendor = usb_serial_hw_devices[i].vendor_id;
          supported_devices[i].product = usb_serial_hw_devices[i].product_id;
        }

        (*usb_m->register_driver)(DRIVER_NAME, supported_devices,
                                        SIZEOF(usb_serial_hw_devices), DRIVER_NAME);
        (*usb_m->install_notify)(DRIVER_NAME, &notify_hooks);
  
        usb_serial_lock = create_sem(1, DRIVER_NAME"_devices_table_lock");
      }else{
        status = B_ERROR;
        TRACE_ALWAYS("init_driver failed: tty_m:%08x usb_m:%08x", tty_m, usb_m);
      } 
      if (status != B_OK)
        put_module(B_USB_MODULE_NAME);
    }else
      TRACE_ALWAYS("init_driver failed:%lx cannot get a module %s", status,
                                                               B_USB_MODULE_NAME);
    if (status != B_OK)
      put_module(B_TTY_MODULE_NAME);
  }else
    TRACE_ALWAYS("init_driver failed:%lx cannot get a module %s", status,
                                                               B_TTY_MODULE_NAME);
  
  TRACE_FUNCRET("init_driver returns:%08x\n", status);
  return status;
}

/* uninit_driver - optional function - called every time the driver
   is unloaded */
void uninit_driver (void){
  int i;
  TRACE_FUNCALLS("uninit_driver\n");

  (*usb_m->uninstall_notify)(DRIVER_NAME);
  free(supported_devices);
  acquire_sem(usb_serial_lock);
  
  for(i = 0; i < DEVICES_COUNT; i++)
    if(usb_serial_devices[i]){
      delete_area(usb_serial_devices[i]->buffers_area);
      delete_sem(usb_serial_devices[i]->done_read);
      delete_sem(usb_serial_devices[i]->done_write);
      free(usb_serial_devices[i]);
      usb_serial_devices[i] = 0;
    }
  
  release_sem_etc(usb_serial_lock, 1, B_DO_NOT_RESCHEDULE);
  delete_sem(usb_serial_lock);
  
  for(i = 0; usb_serial_names[i]; i++)
    free(usb_serial_names[i]);

  put_module(B_USB_MODULE_NAME);
  put_module(B_TTY_MODULE_NAME);
}

void usb_serial_device_notify_in(void *cookie,
                                 uint32 status, void *data, uint32 actual_len){
  TRACE_FUNCALLS("usb_serial_device_notify_in:cookie:%08x status:%08x data:%08x len:%d\n", cookie, status, data, actual_len);
  if(cookie){
    usb_serial_device *usd = ((usb_serial_device_info *)cookie)->device;
    usd->actual_len_read = actual_len;
    usd->dev_status_read = status;
    release_sem(usd->done_read);
  }
} 

void usb_serial_device_notify_out(void *cookie, uint32 status,
                                  void *data, uint32 actual_len){
  TRACE_FUNCALLS("usb_serial_device_notify_out:cookie:%08x status:%08x "
                 "data:%08x len:%d\n", cookie, status, data, actual_len);
  if(cookie){
    usb_serial_device *usd = ((usb_serial_device_info *)cookie)->device;
    usd->actual_len_write = actual_len;
    usd->dev_status_write = status;
    release_sem(usd->done_write);
  }
} 

status_t usb_serial_device_thread(void *param){
  status_t status = B_ERROR;
  usb_serial_device_info *usdi = (usb_serial_device_info *)param;
  usb_serial_device *usd = usdi->device;
  TRACE_FUNCALLS("usb_serial_device_thread:%08x\n", param);

  while(true){
    size_t i, to_read;
    char *buf;
    struct ddrover *ddr;
    usd->actual_len_read = 0;
    usd->dev_status_read = 0;
    status = (*usb_m->queue_bulk)(usd->pipe_read, usd->read_buffer,
                        usd->read_buffer_size, usb_serial_device_notify_in, usdi);
    if(status != B_OK){
      TRACE_ALWAYS("usb_serial_device_thread : queue_bulk error : %lx\n", status);
      break;
    }

    status = acquire_sem_etc(usd->done_read, 1, B_CAN_INTERRUPT, 0);
    if(status != B_OK){
      TRACE_ALWAYS("usb_serial_device_thread : acquire_sem_etc : "
                     "error %08lx (%s)\n", status, strerror(status));
      break;
    }

    if(usd->dev_status_read){
      TRACE_ALWAYS("usb_serial_device_thread : dev_status error !!!\n");
      break;
    }
    
    buf = usd->read_buffer;
    to_read = usd->actual_len_read;
    (*usd->hw->on_read)(usd, &buf, &to_read);

    if(!to_read)
      continue;

    ddr = (*tty_m->ddrstart)(NULL);
    if(!ddr){
      TRACE_ALWAYS("usb_serial_device_thread : ddrstart problem !!!\n");
      status = B_NO_MEMORY;
      break;
    }

    (*tty_m->ttyilock)(usd->tty, ddr, true);
    for(i = 0; i < to_read; i++){
      (*tty_m->ttyin)(usd->tty, ddr, buf[i]);
    }
    (*tty_m->ttyilock)(usd->tty, ddr, false);
    (*tty_m->ddrdone)(ddr);
  }

  TRACE_FUNCRET("usb_serial_device_thread returns %08x\n", status);
  return status;
}

void usb_serial_setmodes(usb_serial_device *usd){
  usb_serial_line_coding line_coding;
  struct termios tios;

  int newctrl, baud_index;
  TRACE_FUNCALLS("usb_serial_setmodes:%08x\n", usd);

  memcpy(&tios, &usd->tty->t, sizeof(struct termios)); 
  newctrl = usd->ctrlout;

  TRACE_FUNCRES(trace_termios, &tios);
  TRACE("newctrl:%08x\n", newctrl);

  baud_index = tios.c_cflag & CBAUD;
  if(baud_index > SIZEOF(serial_tty_speed))
    baud_index = SIZEOF(serial_tty_speed) - 1;
    
  line_coding.speed = serial_tty_speed[baud_index];
  line_coding.stopbits = ( tios.c_cflag & CSTOPB ) ? LC_STOP_BIT_2 : LC_STOP_BIT_1;

  if(PARENB & tios.c_cflag){
    line_coding.parity = LC_PARITY_EVEN;

    if(PARODD & tios.c_cflag){
      line_coding.parity = LC_PARITY_ODD;
    }
  }else
    line_coding.parity = LC_PARITY_NONE;

  line_coding.databits = (tios.c_cflag & CS8) ? 8 : 7;

  if(line_coding.speed == 0){
    newctrl &= 0xfffffffe;
    line_coding.speed = usd->line_coding.speed;
  }else
    newctrl = CLS_LINE_DTR;

  if(usd->ctrlout != newctrl ){
    usd->ctrlout = newctrl;
    TRACE("newctrl send to modem:%08x\n", newctrl);
    (*usd->hw->set_control_line_state)(usd, newctrl);
  }
  
  if(memcmp (&line_coding, &usd->line_coding, sizeof(usb_serial_line_coding))){
    usd->line_coding.speed = line_coding.speed;
    usd->line_coding.stopbits = line_coding.stopbits;
    usd->line_coding.databits = line_coding.databits;
    usd->line_coding.parity = line_coding.parity;
    TRACE("send to modem:speed %d sb:%08x db:%08x parity:%08x\n",
                                usd->line_coding.speed, usd->line_coding.stopbits,
                                usd->line_coding.databits, usd->line_coding.parity);
    (*usd->hw->set_line_coding)(usd, &usd->line_coding);
  }
}

bool usb_serial_service(struct tty *ptty, struct ddrover *ddr, uint flags){
  int i;
  bool bret = false;
  usb_serial_device *usd;
  TRACE_FUNCALLS("usb_serial_service:%08x ddr:%08x flags:%08x\n", ptty, ddr, flags);
  
  for(i = 0; i < DEVICES_COUNT; i++){
    if(usb_serial_devices[i]){
      if(ptty == usb_serial_devices[i]->tty){
        usd = usb_serial_devices[i];
        break;
      }
    }
  }

  if(usd){
    if(flags <= TTYGETSIGNALS){ 
      switch(flags){
      case TTYENABLE:
          (*tty_m->ttyhwsignal)(ptty, ddr, TTYHWDCD, false);
          (*tty_m->ttyhwsignal)(ptty, ddr, TTYHWCTS, true);
          
          usd->ctrlout = CLS_LINE_DTR | CLS_LINE_RTS;
          (*usd->hw->set_control_line_state)(usd, usd->ctrlout);
          break;
          
      case TTYDISABLE:
          
          (*tty_m->ttyhwsignal)(ptty, ddr, TTYHWDCD, false);
          
          usd->ctrlout = 0x0;
          (*usd->hw->set_control_line_state)(usd, usd->ctrlout);
          break;
          
      case TTYISTOP:
          (*tty_m->ttyhwsignal)(ptty, ddr, TTYHWCTS, false);
          break;
          
      case TTYIRESUME:
          (*tty_m->ttyhwsignal)(ptty, ddr, TTYHWCTS, true);
          break;
          
      case TTYGETSIGNALS:
          (*tty_m->ttyhwsignal)(ptty, ddr, TTYHWDCD, true);
          (*tty_m->ttyhwsignal)(ptty, ddr, TTYHWCTS, true);
          (*tty_m->ttyhwsignal)(ptty, ddr, TTYHWDSR, false);
          (*tty_m->ttyhwsignal)(ptty, ddr, TTYHWRI, false);
          break;
          
      case TTYSETMODES:
          usb_serial_setmodes(usd);
          break;

      case TTYOSTART:
      case TTYOSYNC:
      case TTYSETBREAK:
      case TTYCLRBREAK:
      case TTYSETDTR:
      case TTYCLRDTR:
          break;
      }
    }    
    bret = true;
  }
  
  TRACE_FUNCRET("usb_serial_service returns %d\n", bret);
  return bret;
}

void usb_serial_notify_interrupt(void *cookie, uint32 status,
                                 void *data, uint32 actual_len){
  TRACE_FUNCALLS("usb_serial_notify_interrupt : %p %lx %p %ld\n",
                                     cookie, status, data, actual_len);
  if(cookie){
    usb_serial_device *usd = ((usb_serial_device_info *)cookie)->device;
    usd->actual_len_interrupt = actual_len; 
    usd->dev_status_interrupt = status; 
  }
} 
    
/* usb_serial_open - handle open() calls */
static status_t usb_serial_open (const char *name, uint32 flags, void** cookie){
  int i;
  status_t status = ENODEV;
  TRACE_FUNCALLS("usb_serial_open:%s flags:%d cookie:%08x\n",
                                                   name, flags, cookie);
  
  for(i=0;i<DEVICES_COUNT;i++)
    TRACE("%08x\n",usb_serial_devices[i]);
  
  *cookie = NULL;
  i = strtol(name + BASENAME_LEN, NULL, 10);
  if(i >= 0 && i < DEVICES_COUNT){
    acquire_sem(usb_serial_lock);
    if(usb_serial_devices[i]){
      usb_serial_device_info *usdi = malloc(sizeof(usb_serial_device_info));
      if(usdi){
        struct ddrover *ddr;
        memset(usdi, 0, sizeof(usb_serial_device_info));
        *cookie = usdi;
        
        TRACE("cookie in open:%08x\n", *cookie);
        
        usdi->device = usb_serial_devices[i];
        
        (*tty_m->ttyinit)(&usb_serial_tty[i], true);
        
        usdi->ttyfile.tty = &usb_serial_tty[i];
        usdi->ttyfile.flags = flags;
        usb_serial_devices[i]->tty = &usb_serial_tty[i];
       
        (*usdi->device->hw->reset_device)(usdi->device);       
       
        TRACE("Opened:%s %08x\n", usdi->device->hw->descr, usdi->device->dev);
               
        ddr = (*tty_m->ddrstart)(NULL);
        if(ddr){
          (*tty_m->ddacquire)(ddr, &usb_serial_dd);
          status = (*tty_m->ttyopen)(&usdi->ttyfile, ddr, usb_serial_service);
          (*tty_m->ddrdone)(ddr);
          
          if(status == B_OK){
            sem_id sem = spawn_kernel_thread(usb_serial_device_thread,
                                          "usb_serial_device_thread", 0xa, usdi);
            if(sem != -1)
              resume_thread(sem);
            else
              TRACE_ALWAYS("usb_serial_open: cannot spawn kernel thread!!!\n");
          
            usdi->device->ctrlout = CLS_LINE_DTR | CLS_LINE_RTS;
            (*usdi->device->hw->set_control_line_state)(usdi->device,
                                                        usdi->device->ctrlout);
          
            status = (*usb_m->queue_interrupt)(usdi->device->pipe_control,
                                               usdi->device->interrupt_buffer,
                                               usdi->device->interrupt_buffer_size, 
                                               usb_serial_notify_interrupt, usdi);
          }
        }else
          status = B_NO_MEMORY;
          
        if(status != B_OK)
          free(usdi);
      }else
        status = B_NO_MEMORY;
    }
    release_sem(usb_serial_lock);
  }

  TRACE_FUNCRET("usb_serial_open returns:%08x\n", status);
  return status;
}

/* usb_serial_read - handle read() calls */
static status_t usb_serial_read (void* cookie, off_t position,
                                 void *buf, size_t* num_bytes){
  status_t status = B_OK;
  struct ddrover *ddr;
  TRACE_FUNCALLS("usb_serial_read:%08x position:%Ld buf:%08x len:%d\n",
                                               cookie, position, buf, *num_bytes);
  
  ddr = (*tty_m->ddrstart)(NULL);
  if(ddr){
    if(cookie)
      status = (*tty_m->ttyread)(cookie, ddr, buf, num_bytes);
    else
      (*tty_m->ddacquire)(ddr, &usb_serial_dd);
    (*tty_m->ddrdone)(ddr);
  }else{
    TRACE_ALWAYS("usb_serial_read : ddrstart problem !!!\n");
    status = B_NO_MEMORY; 
  }
  
  TRACE_FUNCRET("usb_serial_read returns:%08x readed:%d\n", status, *num_bytes);
  return status;
}

/* usb_serial_write - handle write() calls */

static status_t usb_serial_write (void* cookie, off_t position,
                                  const void* buffer, size_t* num_bytes){
  status_t status = B_OK;
  usb_serial_device_info *usdi = (usb_serial_device_info *)cookie;
  usb_serial_device *usd = usdi->device;
  size_t to_write = *num_bytes;
  TRACE_FUNCALLS("usb_serial_write:%08x position:%Ld buf:%08x len:%d\n",
                                         cookie, position, buffer, *num_bytes);
  *num_bytes = 0;                /* tell caller nothing was written */

  while(to_write > 0){
    size_t len = to_write;

    (*usd->hw->on_write)(usd, buffer, &len);

    usd->actual_len_write = 0;
    usd->dev_status_write = 0;

    status = (*usb_m->queue_bulk)(usd->pipe_write, usd->write_buffer, 
                                   len, usb_serial_device_notify_out, cookie);
    if(status != B_OK){
      TRACE_ALWAYS("usb_serial_write : dev %p status %lx", cookie, status);
      break;
    }

    status = acquire_sem_etc(usd->done_write, 1, B_CAN_INTERRUPT, 0);
    if(status != B_OK){
      TRACE_ALWAYS("usb_serial_write : acquire_sem_etc() %lx (%s)",
                                                    status, strerror(status));
      break;
    }

    ((const char*)buffer) += usd->actual_len_write;
    *num_bytes += usd->actual_len_write;
    to_write   -= usd->actual_len_write;
  }

  TRACE_FUNCRET("usb_serial_write returns:%08x written:%d\n", status, *num_bytes);
  return status;
}

/* usb_serial_control - handle ioctl calls */
static status_t usb_serial_control (void* cookie, uint32 op, void* arg, size_t len){
  status_t status = B_BAD_VALUE;
  TRACE_FUNCALLS("usb_serial_control cookie:%08x op:%08x arg:%08x len:%d\n",
                                                           cookie, op, arg, len);

  if(cookie){
    struct ddrover *ddr = (*tty_m->ddrstart)(NULL);
    if(ddr){
      status = (*tty_m->ttycontrol)(cookie, ddr, op, arg, len);
      (*tty_m->ddrdone)(ddr);
    }else
      status = B_NO_MEMORY;
  }else
    status = ENODEV;

  TRACE_FUNCRET("usb_serial_control returns:%08x\n", status);
  return status;
}

#if defined(B_BEOS_VERSION_DANO) || defined(__HAIKU__)

/* usb_serial_select - handle select start */
static status_t usb_serial_select (void* cookie, uint8 event, uint32 ref, selectsync *_sync){
  status_t status = B_BAD_VALUE;
  TRACE_FUNCALLS("usb_serial_select cookie:%08x event:%08x ref:%08x sync:%p\n",
                                                           cookie, event, ref, _sync);

  if(cookie){
    struct ddrover *ddr = (*tty_m->ddrstart)(NULL);
    if(ddr){
      status = (*tty_m->ttyselect)(cookie, ddr, event, ref, _sync);
      (*tty_m->ddrdone)(ddr);
    }else
      status = B_NO_MEMORY;
  }else
    status = ENODEV;

  TRACE_FUNCRET("usb_serial_select returns:%08x\n", status);
  return status;
}

/* usb_serial_deselect - handle select exit */
static status_t usb_serial_deselect (void* cookie, uint8 event, selectsync *_sync){
  status_t status = B_BAD_VALUE;
  TRACE_FUNCALLS("usb_serial_deselect cookie:%08x event:%08x sync:%p\n",
                                                           cookie, event, _sync);

  if(cookie){
    struct ddrover *ddr = (*tty_m->ddrstart)(NULL);
    if(ddr){
      status = (*tty_m->ttydeselect)(cookie, ddr, event, _sync);
      (*tty_m->ddrdone)(ddr);
    }else
      status = B_NO_MEMORY;
  }else
    status = ENODEV;

  TRACE_FUNCRET("usb_serial_deselect returns:%08x\n", status);
  return status;
}

#endif

/* usb_serial_close - handle close() calls */
static status_t usb_serial_close (void* cookie){
  status_t status = ENODEV;
  TRACE_FUNCALLS("usb_serial_close:%08x\n", cookie);

  if(cookie){
    usb_serial_device_info *usdi = (usb_serial_device_info *)cookie;
    usb_serial_device *usd = usdi->device;
    struct ddrover *ddr;
    
    (*usd->hw->on_close)(usd);

    ddr = (*tty_m->ddrstart)(NULL);
    if(ddr){
      status = (*tty_m->ttyclose)(cookie, ddr);
      (*tty_m->ddrdone)(ddr);
    }else
      status = B_NO_MEMORY;
  }

  TRACE_FUNCRET("usb_serial_close returns:%08x\n", status);
  return status;
}

/* usb_serial_free - called after the last device is closed, and after
   all i/o is complete.*/
static status_t usb_serial_free (void* cookie){
  status_t status = B_OK;
  TRACE_FUNCALLS("usb_serial_free:%08x\n", cookie);

  if(cookie){
    struct ddrover *ddr = (*tty_m->ddrstart)(NULL);
    if(ddr){
      status = (*tty_m->ttyfree)(cookie, ddr);
      (*tty_m->ddrdone)(ddr);
    }else
      status = B_NO_MEMORY;
    free(cookie);
  }

  TRACE_FUNCRET("usb_serial_free returns:%08x\n", status);
  return status;
}

/* publish_devices - return a null-terminated array of devices
   supported by this driver. */
const char** publish_devices(){
  int i, j;
  TRACE_FUNCALLS("publish_devices\n");

  for(i=0; usb_serial_names[i]; i++)
    free(usb_serial_names[i]);

  acquire_sem(usb_serial_lock);
  for(i=0, j=0; i < DEVICES_COUNT; i++){
    if(usb_serial_devices[i]){
      usb_serial_names[j] = malloc(strlen(basename + 2));
      if(usb_serial_names[j]){
        sprintf(usb_serial_names[j], basename, i);
        j++;
      }
      else
        TRACE_ALWAYS("publish_devices - NO MEMORY\n");
    }
  }
  usb_serial_names[j] = NULL;
  release_sem(usb_serial_lock);

  return (const char **)&usb_serial_names[0];
}

/* find_device - return ptr to device hooks structure for a
   given device name */
device_hooks* find_device(const char* name){
  TRACE_FUNCALLS("find_device(%s)\n", name);
  return &usb_serial_hooks;
}

status_t add_device(usb_serial_device *usd, const usb_configuration_info *uci,
                                            usb_endpoint_info *comm_epi, 
                                            usb_endpoint_info *data_out_epi, 
                                            usb_endpoint_info *data_in_epi){
  char name[32]; 
  status_t status = ENODEV;
  size_t buf_len;
  int i = 0;
 
  TRACE_FUNCALLS("add_device(%08x, %08x, %08x, %08x, %08x)\n",
                             usd, uci, comm_epi, data_in_epi, data_out_epi);
  
  acquire_sem(usb_serial_lock);

  for(i = 0; i < DEVICES_COUNT; i++){
    if(usb_serial_devices[i] != NULL)
      continue;

    usb_serial_devices[i] = usd;
      
    usd->active = 1;
    usd->open = 0;
    
    sprintf(name, "usb_serial:%d:done_read", i );
    usd->done_read = create_sem(0, name);
    
    sprintf(name, "usb_serial:%d:done_write", i);
    usd->done_write = create_sem(0, name);

    usd->tty = NULL;
    
    buf_len = usd->read_buffer_size + usd->write_buffer_size +
                                                     usd->interrupt_buffer_size;

    usd->buffers_area = create_area("usb_serial:buffers_area",
                                    (void *)&usd->read_buffer,
                                    B_ANY_KERNEL_ADDRESS,
                                    ROUNDUP(buf_len, B_PAGE_SIZE),
                                    B_CONTIGUOUS, B_READ_AREA|B_WRITE_AREA);
      
    usd->write_buffer     = usd->read_buffer + usd->read_buffer_size;
    usd->interrupt_buffer = usd->write_buffer + usd->write_buffer_size;           

    (*usb_m->set_configuration)(usd->dev, uci);

    usd->pipe_control = (comm_epi) ? comm_epi->handle : 0;
    usd->pipe_write = data_out_epi->handle;
    usd->pipe_read = data_in_epi->handle;

    status = B_OK; 
    break;
  } 
  
  release_sem(usb_serial_lock);
  
  TRACE_FUNCRET("add_device returns:%08x\n", status);
  return status;
}


status_t usb_serial_device_added(const usb_device *dev, void **cookie){
  int i;
  status_t status = B_OK;
  const usb_device_descriptor *udd;
  usb_serial_device *usd = 0;
  TRACE_FUNCALLS("usb_serial_device_added:%08x cookie:%08x\n", dev, cookie);
  
  udd = (*usb_m->get_device_descriptor)(dev);

  TRACE_ALWAYS("Probing device: %08x/%08x\n", udd->vendor_id, udd->product_id);

  *cookie = 0;
  for(i=1; i < sizeof(usb_serial_hw_devices)/sizeof(usb_serial_hw_devices[0]); i++)
    if(usb_serial_hw_devices[i].vendor_id == udd->vendor_id
      && usb_serial_hw_devices[i].product_id == udd->product_id){
      const usb_configuration_info *uci;
      uci = (*usb_m->get_nth_configuration)(dev, 0);
      if(uci){
        usd = malloc(sizeof(usb_serial_device));
        if(usd){
          usd->dev = dev;
          usd->hw = &usb_serial_hw_devices[i];
          usd->read_buffer_size = 
          usd->write_buffer_size = 
          usd->interrupt_buffer_size = ROUNDUP(DEF_BUFFER_SIZE, 16);
          if((status = (*usb_serial_hw_devices[i].add_device)(usd, uci)) == B_OK){
            *cookie = dev;
            TRACE_ALWAYS("%s (%04x/%04x) added \n",
                                        usb_serial_hw_devices[i].descr,
                                        usb_serial_hw_devices[i].vendor_id,
                                        usb_serial_hw_devices[i].product_id);
          }else
            free(usd);
        }else
          status = B_NO_MEMORY;
      }
      break;
    }

  if(!*cookie){
    if(b_acm_support &&
         udd->device_class == USB_DEV_CLASS_COMM &&
           udd->device_subclass == 0 &&
             udd->device_protocol == 0 ){
      const usb_configuration_info *uci;
      uint16 idx = 0;
      do{
        uci = (*usb_m->get_nth_configuration)(dev, idx++);
        if(uci){
          if(uci->interface_count != 2)
            continue;
          usd = malloc(sizeof(usb_serial_device));
          if(usd){
            usd->dev = dev;
            usd->hw = &usb_serial_hw_devices[0];
            usd->read_buffer_size = 
            usd->write_buffer_size = 
            usd->interrupt_buffer_size = ROUNDUP(DEF_BUFFER_SIZE, 16);
            if((status = (*usb_serial_hw_devices[0].add_device)(usd, uci)) == B_OK){
              *cookie = dev;
              TRACE_ALWAYS("%s (%04x/%04x) added \n",
                                         usb_serial_hw_devices[0].descr,
                                         udd->vendor_id, udd->product_id);
            }else
              free(usd);
          }else
            status = B_NO_MEMORY;
        }
      }while(uci);
    }else{
      TRACE_ALWAYS("device has wrong class, subclass and protocol\n");
      status = B_ERROR;
    }
  }
  TRACE_FUNCRET("usb_serial_device_added returns:%08x\n", status);
  return status;
}

status_t usb_serial_device_removed(void *cookie){
  int i;
  status_t status = B_OK;
  usb_device *ud = (usb_device *) cookie;
  TRACE_FUNCALLS("usb_serial_device_removed:%08x\n", cookie);

  acquire_sem(usb_serial_lock);
  
  for(i = 0; i < DEVICES_COUNT; i++ ){
    usb_serial_device *usd = usb_serial_devices[i];
    if(usd){
      if(ud == usd->dev){
        usd->active = 0;
        if(!usd->open)
          break;
            
        delete_area(usd->buffers_area);
        delete_sem(usd->done_read);
        delete_sem(usd->done_write);
        free(usd);
        usb_serial_devices[i] = 0;
      }
    }
  }
  
  release_sem(usb_serial_lock);

  TRACE_FUNCRET("usb_serial_device_removed returns:%08x\n", status);
  return status;
}

