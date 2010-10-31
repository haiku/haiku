/*
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 * 
 */

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>

#include <USB.h>
#include <malloc.h>
#include <string.h> /* strerror */
#include <stdlib.h> /* strtol */
#include <stdio.h>  /* sprintf */


#include "usb_vision.h"
#include "tracing.h"

#define BASENAME_LEN 0x10 /*must be synchronized with below !!!*/
static const char *basename = "video/usb_vision/%u";

status_t usb_vision_device_added(const usb_device *dev, void **cookie);
status_t usb_vision_device_removed(void *cookie);

static usb_notify_hooks notify_hooks = {
    &usb_vision_device_added,
    &usb_vision_device_removed
}; 

struct usb_module_info *usb;
usb_vision_device *usb_vision_devices[DEVICES_COUNT];
char * usb_vision_names[DEVICES_COUNT + 1];
sem_id usb_vision_lock = -1;

struct usb_support_descriptor supported_devices[] = {
  {0, 0, 0, 0x0573, 0x4d31},
};

/* init_hardware - called once the first time the driver is loaded */
status_t init_hardware (void){
  TRACE("init_hardware\n"); /*special case - no file-logging activated now*/
  return B_OK;
}

/* init_driver - optional function - called every time the driver is loaded. */
status_t init_driver (void){
  int i;
  status_t status = B_OK;
  load_setting();
  create_log();

  TRACE_FUNCALLS("init_driver\n");
  
  if((status = get_module(B_USB_MODULE_NAME, (module_info **)&usb)) == B_OK){
    if(usb){
      for(i = 0; i < DEVICES_COUNT; i++)
        usb_vision_devices[i] = 0;
        
      usb_vision_names[0] = NULL;  

      (*usb->register_driver)(DRIVER_NAME, supported_devices, SIZEOF(supported_devices), DRIVER_NAME);
      (*usb->install_notify)(DRIVER_NAME, &notify_hooks);

      usb_vision_lock = create_sem(1, DRIVER_NAME"_devices_table_lock");
    }else{
      status = B_ERROR;
      TRACE_ALWAYS("init_driver failed: usb:%08x", usb);
    } 
  }else
    TRACE_ALWAYS("init_driver failed:%lx cannot get a module %s", status, B_USB_MODULE_NAME);
  
  TRACE_FUNCRET("init_driver returns:%08x\n", status);
  return status;
}


/* uninit_driver - optional function - called every time the driver is unloaded */
void uninit_driver (void){
  int i;
  TRACE_FUNCALLS("uninit_driver\n");

  (*usb->uninstall_notify)(DRIVER_NAME);
  acquire_sem(usb_vision_lock);
  
  for(i = 0; i < DEVICES_COUNT; i++)
    if(usb_vision_devices[i]){
      free(usb_vision_devices[i]);
      usb_vision_devices[i] = 0;
    }
  
  release_sem_etc(usb_vision_lock, 1, B_DO_NOT_RESCHEDULE);
  delete_sem(usb_vision_lock);
  
  for(i = 0; usb_vision_names[i]; i++)
    free(usb_vision_names[i]);

  put_module(B_USB_MODULE_NAME);
}

	
/* usb_vision_open - handle open() calls */

static status_t usb_vision_open (const char *name, uint32 flags, void** cookie)
{
  int i;
  status_t status = ENODEV;
  TRACE_FUNCALLS("usb_vision_open:%s flags:%d cookie:%08x\n", name, flags, cookie);
  
  for(i = 0; i < DEVICES_COUNT; i++)
    TRACE("%08x\n", usb_vision_devices[i]);
  
  *cookie = NULL;
  i = strtol(name + BASENAME_LEN, NULL, 10);
  if(i >= 0 && i < DEVICES_COUNT){
    acquire_sem(usb_vision_lock);
    if(usb_vision_devices[i]){
      if(atomic_add(&usb_vision_devices[i]->open_count, 1) == 0){
        *cookie = usb_vision_devices[i];
        TRACE("cookie in open:%08x\n", *cookie);
        status = B_OK;
      }else{
        atomic_add(&usb_vision_devices[i]->open_count, -1);
        status = B_BUSY;
      }
    }
    release_sem(usb_vision_lock);
  }

  TRACE_FUNCRET("usb_vision_open returns:%08x\n", status);
  return status;
}

/* usb_vision_read - handle read() calls */

static status_t usb_vision_read (void* cookie, off_t position, void *buf, size_t* num_bytes)
{
	*num_bytes = 0;				/* tell caller nothing was read */
	return B_IO_ERROR;
}


/* usb_vision_write - handle write() calls */

static status_t usb_vision_write (void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	*num_bytes = 0;				/* tell caller nothing was written */
	return B_IO_ERROR;
}

static status_t xet_nt_register(bool is_read, usb_vision_device *uvd, xet_nt100x_reg *ri)
{
  status_t status = B_ERROR;
  //uint8 req_type = USB_REQTYPE_VENDOR | (is_read ? USB_REQTYPE_DEVICE_IN : USB_REQTYPE_DEVICE_OUT);
  
  TRACE_FUNCALLS("set_nt_register:%08x, %08x\n", uvd, ri);
  TRACE_FUNCRES(trace_reginfo, ri);
  
  //(*usb->send_request)(uvd->dev, req_type,
  //                     )

  TRACE_FUNCRET("set_nt_register returns:%08x\n", status);
  return status;
}

/* usb_vision_control - handle ioctl calls */
static status_t usb_vision_control (void* cookie, uint32 op, void* arg, size_t len)
{
  status_t status = B_BAD_VALUE;
  TRACE_FUNCALLS("usb_vision_control:%08x, %d, %08x, %d\n", cookie, op, arg, len);
  switch(op){
  case NT_IOCTL_READ_REGISTER:
    status = xet_nt_register(true, (usb_vision_device *)cookie, (xet_nt100x_reg *) arg);
    break;
  case NT_IOCTL_WRITE_REGISTER:
    status = xet_nt_register(false, (usb_vision_device *)cookie, (xet_nt100x_reg *) arg);
    break;
  default:
    break;
  }
  TRACE_FUNCRET("usb_vision_control returns:%08x\n", status);
  return status;
}

/* usb_vision_close - handle close() calls */
static status_t usb_vision_close (void* cookie)
{
  status_t status = B_OK;//ENODEV;
  TRACE_FUNCALLS("usb_vision_close:%08x\n", cookie);

  TRACE_FUNCRET("usb_vision_close returns:%08x\n", status);
  return status;
}


/* usb_vision_free - called after the last device is closed, and after all i/o is complete. */
static status_t usb_vision_free (void* cookie)
{
  status_t status = B_OK;
  TRACE_FUNCALLS("usb_vision_free:%08x\n", cookie);

  if(cookie){
    usb_vision_device *uvd = (usb_vision_device *)cookie;
    atomic_add(&uvd->open_count, -1);
  }

  TRACE_FUNCRET("usb_vision_free returns:%08x\n", status);
  return status;
}

/* function pointers for the device hooks entry points */
device_hooks usb_vision_hooks = {
	usb_vision_open, 			/* -> open entry point */
	usb_vision_close, 			/* -> close entry point */
	usb_vision_free,			/* -> free cookie */
	usb_vision_control, 		/* -> control entry point */
	usb_vision_read,			/* -> read entry point */
	usb_vision_write			/* -> write entry point */
};

/* publish_devices - return a null-terminated array of devices
   supported by this driver. */

const char** publish_devices(){
  int i, j;
  TRACE_FUNCALLS("publish_devices\n");

  for(i=0; usb_vision_names[i]; i++)
    free(usb_vision_names[i]);

  acquire_sem(usb_vision_lock);
  for(i=0, j=0; i < DEVICES_COUNT; i++){
    if(usb_vision_devices[i]){
      usb_vision_names[j] = malloc(strlen(basename + 2));
      if(usb_vision_names[j]){
        sprintf(usb_vision_names[j], basename, i);
        j++;
      }
      else
        TRACE_ALWAYS("publish_devices - NO MEMORY\n");
    }
  }
  usb_vision_names[j] = NULL;
  release_sem(usb_vision_lock);

  return (const char **)&usb_vision_names[0];
}

/* find_device - return ptr to device hooks structure for a
   given device name */
device_hooks* find_device(const char* name){
  TRACE_FUNCALLS("find_device(%s)\n", name);
  return &usb_vision_hooks;
}

static status_t create_add_device(usb_vision_device *uvd, const struct usb_configuration_info *uci,
                                              struct usb_endpoint_info *control_epi, 
                                              struct usb_endpoint_info *data_epi){
//  char name[32]; 
  status_t status = ENODEV;
//  size_t buf_len;
  int i = 0;
 
  TRACE_FUNCALLS("create_add_device(%08x, %08x, %08x, %08x)\n", uvd, uci, control_epi, data_epi);
  
  acquire_sem(usb_vision_lock);

  for(i = 0; i < DEVICES_COUNT; i++){
    if(usb_vision_devices[i] != NULL)
      continue;

    usb_vision_devices[i] = uvd;
   /*   
    usd->active = 1;
    usd->open = 0;
   
    sprintf(name, "usb_vision:%d:done_read", i );
    usd->done_read = create_sem(0, name);
    
    sprintf(name, "usb_vision:%d:done_write", i);
    usd->done_write = create_sem(0, name);

    usd->tty = NULL;
    
    buf_len = usd->read_buffer_size + usd->write_buffer_size + usd->interrupt_buffer_size;

    usd->buffers_area = create_area("usb_serial:buffers_area", (void *)&usd->read_buffer, B_ANY_KERNEL_ADDRESS,
                                        ROUNDUP(buf_len, B_PAGE_SIZE),
                                           B_CONTIGUOUS, B_READ_AREA|B_WRITE_AREA);
      
    usd->write_buffer     = usd->read_buffer + usd->read_buffer_size;
    usd->interrupt_buffer = usd->write_buffer + usd->write_buffer_size;           
*/
    (*usb->set_configuration)(uvd->dev, uci);

    uvd->control_pipe = control_epi->handle;
    uvd->data_pipe = data_epi->handle;
   
    status = B_OK; 
    break;
  } 
  release_sem(usb_vision_lock);
  
  TRACE_FUNCRET("add_device returns:%08x\n", status);
  return status;
}

static status_t add_device(usb_vision_device *uvd, const usb_configuration_info *uci){
  usb_endpoint_info *control_epi = NULL;
  usb_endpoint_info *data_epi = NULL;
  status_t status = ENODEV;
  int i = 0;
  usb_interface_info *uii = uci->interface[0].active;
  TRACE_FUNCALLS("> add_device(%08x, %08x)\n", uvd, uci);

  for(i=0; i < uii->endpoint_count; i++){
    if(uii->endpoint[i].descr->attributes == USB_EP_ATTR_ISOCHRONOUS){
      if((uii->endpoint[i].descr->endpoint_address & USB_EP_ADDR_DIR_IN) == USB_EP_ADDR_DIR_IN){
        data_epi = &uii->endpoint[i];
        TRACE("iso_ep:%d\n", i);
      }
    }
    if(uii->endpoint[i].descr->attributes == USB_EP_ATTR_CONTROL){
      control_epi = &uii->endpoint[i];
        TRACE("cont_ep:%d\n", i);
    }
    if(control_epi && data_epi)
      break;
  }
  
  if(control_epi && data_epi){
    status = create_add_device(uvd, uci, control_epi, data_epi);
  }
  
  TRACE_FUNCRET("< create_add_device returns:%08x\n", status);
  return status;
}

status_t usb_vision_device_added(const usb_device *dev, void **cookie){
  int dev_idx;
  status_t status = B_OK;
  const usb_device_descriptor *udd;
  usb_vision_device *uvd = 0;
  TRACE_FUNCALLS("usb_vision_device_added:%08x cookie:%08x\n", dev, cookie);
  
  udd = (*usb->get_device_descriptor)(dev);
  TRACE_ALWAYS("Probing device: %08x/%08x\n", udd->vendor_id, udd->product_id);

  *cookie = 0;
  for(dev_idx = 0; dev_idx < SIZEOF(supported_devices); dev_idx++)
    if(supported_devices[dev_idx].vendor == udd->vendor_id
      && supported_devices[dev_idx].product == udd->product_id){
      const usb_configuration_info *uci;
      int cfg_idx;
      for(cfg_idx = 0; (uci = (*usb->get_nth_configuration)(dev, cfg_idx)) != NULL; cfg_idx++){
        uvd = malloc(sizeof(usb_vision_device));
        if(uvd){
          memset(uvd, 0, sizeof(usb_vision_device));
          uvd->dev = dev;
          if((status = add_device(uvd, uci)) == B_OK){
            *cookie = (void *)dev;
            TRACE_ALWAYS("(%04x/%04x) added \n", supported_devices[dev_idx].vendor,
                                               supported_devices[dev_idx].product);
            break;
          }else
            free(uvd);
        }else
          status = B_NO_MEMORY;
      }
      break;
    }

  TRACE_FUNCRET("usb_vision_device_added returns:%08x\n", status);
  return status;
}

status_t usb_vision_device_removed(void *cookie){
  int i;
  status_t status = B_OK;
  struct usb_device *ud = (struct usb_device *) cookie;
  TRACE_FUNCALLS("usb_vision_device_removed:%08x\n", cookie);

  acquire_sem(usb_vision_lock);
  
  for(i = 0; i < DEVICES_COUNT; i++ ){
    usb_vision_device *uvd = usb_vision_devices[i];
    if(uvd){
      if(ud == uvd->dev){
        free(uvd);
        usb_vision_devices[i] = 0;
      }
    }
  }
  
  release_sem(usb_vision_lock);

  TRACE_FUNCRET("usb_vision_device_removed returns:%08x\n", status);
  return status;
}
