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
  status_t status = ENODEV;
  int i = 0;

  TRACE_FUNCALLS("> add_prolific_device(%08x, %08x)\n", usd, uci);

  if(uci->interface_count){
    struct usb_interface_info *uii = uci->interface[0].active;
    for(i = 0; i < uii->endpoint_count; i++){
      if(uii->endpoint[i].descr->attributes == USB_EP_ATTR_INTERRUPT){
        if((uii->endpoint[i].descr->endpoint_address & USB_EP_ADDR_DIR_IN)
                          == USB_EP_ADDR_DIR_IN) /*USB_EP_ADDR_DIR_OUT = 0x0*/
          comm_epi = &uii->endpoint[i];
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
          data_in_epi = &uii->endpoint[i];
        }else{
          if(uii->endpoint[i].descr->endpoint_address
                                          /*USB_EP_ADDR_DIR_OUT = 0x0*/){
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

status_t reset_prolific_device(usb_serial_device *usd){
  status_t status;
  size_t len = 0;
  TRACE_FUNCALLS("> reset_prolific_device(%08x)\n", usd);
  status = (*usb_m->send_request)(usd->dev,
                                  USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
                                  PROLIFIC_SET_REQUEST, 0, 0, 0, 0, 0, &len);
  TRACE_FUNCRET("< reset_prolific_device returns:%08x\n", status);
  return status;
}
