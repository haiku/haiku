/*
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 * 
 */

#include <KernelExport.h>
#include <Drivers.h>
#include <USB.h>
#include <ttylayer.h>

#include "driver.h"
#include "prolific.h"

status_t add_prolific_device(usb_serial_device *usd,
                             const usb_configuration_info *uci)
{
  struct usb_endpoint_info *comm_epi = NULL;
  struct usb_endpoint_info *data_out_epi = NULL;
  struct usb_endpoint_info *data_in_epi  = NULL;
  const usb_device_descriptor *ddesc;
  status_t status = ENODEV;
  uint32 i = 0;

  TRACE_FUNCALLS("> add_prolific_device(%08x, %08x)\n", usd, uci);
  
  /* check for device type.
   * Linux checks for type 0, 1 and HX, but handles 0 and 1 the same.
   * We'll just check for HX then.
   */
  if ((ddesc = (*usb_m->get_device_descriptor)(usd->dev))){
    usd->spec.prolific.is_HX = (ddesc->device_class != 0x02 && 
                                ddesc->max_packet_size_0 == 0x40);
  }

  if(uci->interface_count){
    struct usb_interface_info *uii = uci->interface[0].active;
    for(i = 0; i < uii->endpoint_count; i++){
      if(uii->endpoint[i].descr->attributes == USB_EP_ATTR_INTERRUPT){
        if((uii->endpoint[i].descr->endpoint_address & USB_EP_ADDR_DIR_IN)
                          == USB_EP_ADDR_DIR_IN /*USB_EP_ADDR_DIR_OUT = 0x0*/){
          TRACE("add_prolific_device:comm endpoint at %d\n", i);
          comm_epi = &uii->endpoint[i];
        }
      }
    }
     /*They say that USB-RSAQ1 has 2 interfaces */
    if(uci->interface_count == 2){
      uii = uci->interface[1].active;
    }

    for(i = 0; i < uii->endpoint_count; i++){
      if(uii->endpoint[i].descr->attributes == USB_EP_ATTR_BULK){
        if((uii->endpoint[i].descr->endpoint_address & USB_EP_ADDR_DIR_IN)
                                                      == USB_EP_ADDR_DIR_IN){
          TRACE("add_prolific_device:in endpoint at %d\n", i);
          data_in_epi = &uii->endpoint[i];
        }else{
          if((uii->endpoint[i].descr->endpoint_address & USB_EP_ADDR_DIR_IN)
                                                      == USB_EP_ADDR_DIR_OUT){
            TRACE("add_prolific_device:out endpoint at %d\n", i);
            data_out_epi = &uii->endpoint[i];
          }
        }
      }
    }
  
    if(comm_epi && data_in_epi && data_out_epi){
      usd->read_buffer_size = 
      usd->write_buffer_size =
      usd->interrupt_buffer_size = ROUNDUP(PROLIFIC_BUF_SIZE, 16);
      status = add_device(usd, uci, comm_epi, data_out_epi, data_in_epi);
    }
  }
  
  TRACE_FUNCRET("< add_prolific_device returns:%08x\n", status);
  return status;
}

struct req_item {
	uint8 requestType;
	uint8 request;
	uint16 value;
	uint16 index;
};

#define PLRT_O (USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT)
#define PLRT_I (USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_IN)

/* Linux sends all those, and it seems to work for me */
/* see drivers/usb/serial/pl2303.c */
static struct req_item pl_reset_common_reqs[] = {
  {PLRT_I, PROLIFIC_SET_REQUEST, 0x8484, 0},
  {PLRT_O, PROLIFIC_SET_REQUEST, 0x0404, 0},
  {PLRT_I, PROLIFIC_SET_REQUEST, 0x8484, 0},
  {PLRT_I, PROLIFIC_SET_REQUEST, 0x8383, 0},
  {PLRT_I, PROLIFIC_SET_REQUEST, 0x8484, 0},
  {PLRT_O, PROLIFIC_SET_REQUEST, 0x0404, 1},
  {PLRT_I, PROLIFIC_SET_REQUEST, 0x8484, 0},
  {PLRT_I, PROLIFIC_SET_REQUEST, 0x8383, 0},
  {PLRT_O, PROLIFIC_SET_REQUEST, 0x0000, 1},
  {PLRT_O, PROLIFIC_SET_REQUEST, 0x0001, 0}
};
static struct req_item pl_reset_common_hx[] = {
  {PLRT_O, PROLIFIC_SET_REQUEST, 2, 0x44},
  {PLRT_O, PROLIFIC_SET_REQUEST, 8, 0},
  {PLRT_O, PROLIFIC_SET_REQUEST, 0, 0}
};
static struct req_item pl_reset_common_nhx[] = {
  {PLRT_O, PROLIFIC_SET_REQUEST, 2, 0x24}
};

static status_t usb_send_requ_list(const usb_device *dev, struct req_item *list, size_t len){
  uint32 i;
  status_t status;
  for (i = 0; i < len; i++){
    char buf[10];
    bool o = (list[i].requestType == PLRT_O);
    size_t buflen = 1;
    status = (*usb_m->send_request)(dev, 
                                    list[i].requestType,
                                    list[i].request,
                                    list[i].value,
                                    list[i].index,
                                    0, o?0:buf, o?0:buflen, &buflen);
    TRACE("usb_send_requ_list: reqs[%d]: %08lx\n", i, status);
  }
  return B_OK;
}

status_t reset_prolific_device(usb_serial_device *usd){
  status_t status;
  TRACE_FUNCALLS("> reset_prolific_device(%08x)\n", usd);
  
  status = usb_send_requ_list(usd->dev, pl_reset_common_reqs, SIZEOF(pl_reset_common_reqs));
  if (usd->spec.prolific.is_HX)
    status = usb_send_requ_list(usd->dev, pl_reset_common_hx, SIZEOF(pl_reset_common_hx));
  else
    status = usb_send_requ_list(usd->dev, pl_reset_common_nhx, SIZEOF(pl_reset_common_nhx));

  status = B_OK; /* discard */
  TRACE_FUNCRET("< reset_prolific_device returns:%08x\n", status);
  return status;
}
