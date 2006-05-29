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
#include "acm.h"

status_t add_acm_device(usb_serial_device *usd,
                        const usb_configuration_info *uci)
{
  struct usb_endpoint_info *comm_epi = NULL;
  struct usb_endpoint_info *data_out_epi = NULL;
  struct usb_endpoint_info *data_in_epi  = NULL;
  status_t status = ENODEV;
  int i = 0;
 
  TRACE_FUNCALLS("> add_acm_device(%08x, %08x)\n", usd, uci);

  for(i=0; i < uci->interface_count; i++){
    usb_interface_descriptor *uid =
                         uci->interface[i].active->descr;
    if(uid->interface_class == USB_INT_CLASS_CDC &&
         uid->interface_subclass == USB_INT_SUBCLASS_ABSTRACT_CONTROL_MODEL &&
            uid->num_endpoints == 1){
      comm_epi = uci->interface[i].active->endpoint;
    }
  
    if(uid->interface_class == USB_INT_CLASS_CDC_DATA &&
        (uid->interface_subclass == 2 || // ?????????????
            uid->interface_subclass == USB_INT_SUBCLASS_DATA)){
      if(uid->num_endpoints == 2){
        data_out_epi = &uci->interface[i].active->endpoint[0];
        data_in_epi = &uci->interface[i].active->endpoint[1];
      }
   }
   if(comm_epi && data_out_epi && data_in_epi)
     break;
  }
  
  if(comm_epi && data_out_epi && data_in_epi){
    status = add_device(usd, uci, comm_epi, data_out_epi, data_in_epi);
  }
  
  TRACE_FUNCRET("< add_acm_device returns:%08x\n", status);
  return status;
}

status_t reset_acm_device(usb_serial_device *usd){
  status_t status = B_OK;
  TRACE_FUNCALLS("> reset_acm_device(%08x)\n", usd);
  TRACE_FUNCRET("< reset_acm_device returns:%08x\n", status);
  return status;
}

status_t set_line_coding_acm(usb_serial_device *usd,
                             usb_serial_line_coding *line_coding){
  size_t len = 0;
  status_t status = B_OK;
  TRACE_FUNCALLS("> set_line_coding_acm(%08x, {%d, %02x, %02x, %02x})\n",
                                       usd, line_coding->speed,
                                       line_coding->stopbits,
                                       line_coding->parity,
                                       line_coding->databits);
  status = (*usb_m->send_request)(usd->dev,
                                  USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_OUT,
                                  SET_LINE_CODING, 0x0, 0x0,
                                  sizeof(usb_serial_line_coding),
                                  line_coding,
                                  sizeof(usb_serial_line_coding), &len);
  TRACE_FUNCRET("< set_line_coding_acm returns:%08x\n", status);
  return status;
}

status_t set_control_line_state_acm(usb_serial_device *usd, uint16 state){
  size_t len = 0;
  status_t status = B_OK;
  TRACE_FUNCALLS("> set_control_line_state_acm(%08x, %04x)\n", usd, state);
  status = (*usb_m->send_request)(usd->dev,
                                  USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_OUT,
                                  SET_CONTROL_LINE_STATE, state, 0, 0, 0, 0,
                                  &len);
  TRACE_FUNCRET("< set_control_line_state_acm returns:%08x\n", status);
  return status;
}

void on_read_acm(usb_serial_device *usd, char **buf, size_t *num_bytes){
}

void on_write_acm(usb_serial_device *usd, const char *buf, size_t *num_bytes){
  if(*num_bytes > usd->write_buffer_size)
    *num_bytes = usd->write_buffer_size;
  memcpy(usd->write_buffer, buf, *num_bytes);
}

void on_close_acm(usb_serial_device *usd){
  TRACE_FUNCALLS("> add_acm_device(%08x)\n", usd);
  (*usb_m->cancel_queued_transfers)(usd->pipe_read);
  (*usb_m->cancel_queued_transfers)(usd->pipe_write);
  (*usb_m->cancel_queued_transfers)(usd->pipe_control);
}

