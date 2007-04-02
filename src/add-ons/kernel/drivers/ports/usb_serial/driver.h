/*
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 * 
 */

#ifndef _USB_SERIAL_DRIVER_H_ 
  #define _USB_SERIAL_DRIVER_H_

/* The name of this driver ... */  
#define DRIVER_NAME "usb_serial"  
/* Maximal count of devices supported by this driver at the same time. */
#define DEVICES_COUNT 20
 
#include "tracing.h"

/* Some usefull helper defines ... */
#define SIZEOF(array) (sizeof(array)/sizeof(array[0])) /* size of array */
/* This one rounds the size to integral count of segs (segments) */
#define ROUNDUP(size, seg) (((size) + (seg) - 1) & ~((seg) - 1))
/* buffer size */
#define DEF_BUFFER_SIZE 0x100

/* line coding defines ... Come from CDC USB specs? */
#define LC_STOP_BIT_1  0
#define LC_STOP_BIT_2  2

#define LC_PARITY_NONE  0
#define LC_PARITY_ODD   1
#define LC_PARITY_EVEN  2
/* struct that represents line coding */
typedef struct _s_usb_serial_line_coding{
  uint32 speed;
  uint8 stopbits;
  uint8 parity;
  uint8 databits;
} usb_serial_line_coding;

/* "forgotten" requests for CDC ACM devices */
#define SET_LINE_CODING        0x20
#define SET_CONTROL_LINE_STATE 0x22

/* control line states */
#define  CLS_LINE_DTR  0x0001
#define  CLS_LINE_RTS  0x0002

/* "forgotten" attributes etc ...*/
#ifndef USB_EP_ADDR_DIR_IN
#define USB_EP_ADDR_DIR_IN   0x80
#define USB_EP_ADDR_DIR_OUT  0x00
#endif

#ifndef USB_EP_ATTR_CONTROL
#define USB_EP_ATTR_CONTROL      0x00
#define USB_EP_ATTR_ISOCHRONOUS  0x01
#define USB_EP_ATTR_BULK         0x02 
#define USB_EP_ATTR_INTERRUPT    0x03 
#endif
/* USB class - communication devices */
#define USB_DEV_CLASS_COMM     0x02

#define USB_INT_CLASS_CDC      0x02
#define USB_INT_SUBCLASS_ABSTRACT_CONTROL_MODEL 0x02

#define USB_INT_CLASS_CDC_DATA 0x0a
#define USB_INT_SUBCLASS_DATA  0x00

/* a kind of forward declaration */
typedef struct _s_usb_serial_device usb_serial_device;

/* usb_serial_hw - represents the different hardware devices */
typedef struct _s_usb_serial_hw{
  uint16 vendor_id; /* vendor id and card id of device */
  uint16 product_id;
   /* callbacks used in "customization" of the driver behaviour */
  status_t (*add_device)(usb_serial_device *usd,
                         const usb_configuration_info *uci);
  status_t (*reset_device)(usb_serial_device *usd);
  status_t (*set_line_coding)(usb_serial_device *usd,
                              usb_serial_line_coding *line_coding);
  status_t (*set_control_line_state)(usb_serial_device *usd, uint16 state);
  void     (*on_read)(usb_serial_device *usd, char **buf, size_t *num_bytes);
  void     (*on_write)(usb_serial_device *usd, const char *buf,
                       size_t *num_bytes);
  void     (*on_close)(usb_serial_device *usd);
  const char *descr; /* informational description */
}usb_serial_hw;

status_t add_device(usb_serial_device *usd, const usb_configuration_info *uci,
                                            usb_endpoint_info *comm_epi, 
                                            usb_endpoint_info *data_out_epi, 
                                            usb_endpoint_info *data_in_epi);

/* this one was typedefed into usb_serial_device - see above for details
   this struct represents the main device object */
struct _s_usb_serial_device{
  int active; /* is device active - performing some tasks */
  int32 open; /* is device opened */
  const usb_device *dev; /* points to usb device handle */
  /* communication pipes */
  usb_pipe *pipe_control;
  usb_pipe *pipe_read;
  usb_pipe *pipe_write;
  /* line coding */
  usb_serial_line_coding line_coding;
  
  /* data buffers */  
  area_id buffers_area;
  /* read buffer */
  char *read_buffer;
  size_t read_buffer_size;
  /* write buffer */
  char *write_buffer; 
  size_t write_buffer_size;
  /* interrupt buffer */
  char *interrupt_buffer;
  size_t interrupt_buffer_size;
  
  /* variables used in callback functionality */
  size_t actual_len_read;
  uint32 dev_status_read;
  size_t actual_len_write;
  uint32 dev_status_write;
  size_t actual_len_interrupt;
  uint32 dev_status_interrupt;
  /* semaphores used in callbacks */
  sem_id done_read;
  sem_id done_write;
  
  uint16 ctrlout;
  
  struct tty *tty;

  usb_serial_hw *hw; /* points to corresponding hardware description */
  /* place for device specific data */
  union{
    struct _spec_acm{ /* ACM-compatible devices */
    }acm;
    struct _spec_prolific{ /* Prolific devices*/
      bool is_HX; /* Linux handles HX type differently */
    }prolific;
    struct _spec_ftdi{ /* FTDI devices*/
      uint8 hdrlen;
      uint8 status_msr;
      uint8 status_lsr;
    }ftdi;
  }spec;
}; /*already typedef-ed - see comment above! */
/* high-level of abstraction of the device*/
typedef struct _s_usb_serial_device_info{
  struct ttyfile ttyfile; /*  */
  usb_serial_device *device; /* */
}usb_serial_device_info;

extern usb_serial_device *usb_serial_devices[];
extern char * usb_serial_names[];
extern usb_module_info *usb_m;

#endif//_USB_SERIAL_DRIVER_H_ 
