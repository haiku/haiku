/*
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 * 
 */

#include <ByteOrder.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <USB.h>
#include <ttylayer.h>

#include "driver.h"
#include "klsi.h"

status_t add_klsi_device(usb_serial_device *usd,
                             const usb_configuration_info *uci)
{
  struct usb_endpoint_info *comm_epi = NULL;
  struct usb_endpoint_info *data_out_epi = NULL;
  struct usb_endpoint_info *data_in_epi  = NULL;
  const usb_device_descriptor *ddesc;
  status_t status = ENODEV;
  uint32 i = 0;

  TRACE_FUNCALLS("> add_klsi_device(%08x, %08x)\n", usd, uci);
  
  if(uci->interface_count){
    struct usb_interface_info *uii = uci->interface[0].active;
    for(i = 0; i < uii->endpoint_count; i++){
      if(uii->endpoint[i].descr->attributes == USB_EP_ATTR_INTERRUPT){
        if((uii->endpoint[i].descr->endpoint_address & USB_EP_ADDR_DIR_IN)
                          == USB_EP_ADDR_DIR_IN /*USB_EP_ADDR_DIR_OUT = 0x0*/){
          TRACE("add_klsi_device:comm endpoint at %d\n", i);
          comm_epi = &uii->endpoint[i];
        }
      }
    }

    for(i = 0; i < uii->endpoint_count; i++){
      if(uii->endpoint[i].descr->attributes == USB_EP_ATTR_BULK){
        if((uii->endpoint[i].descr->endpoint_address & USB_EP_ADDR_DIR_IN)
                                                      == USB_EP_ADDR_DIR_IN){
          TRACE("add_klsi_device:in endpoint at %d\n", i);
          data_in_epi = &uii->endpoint[i];
//          usd->read_buffer_size = uii->endpoint[i].descr->max_packet_size;
        }else{
          if((uii->endpoint[i].descr->endpoint_address & USB_EP_ADDR_DIR_IN)
                                                      == USB_EP_ADDR_DIR_OUT){
            TRACE("add_klsi_device:out endpoint at %d\n", i);
            data_out_epi = &uii->endpoint[i];
//            usd->write_buffer_size = uii->endpoint[i].descr->max_packet_size;
          }
        }
      }
    }
  
    if(comm_epi && data_in_epi && data_out_epi){
      usd->read_buffer_size = 
      usd->write_buffer_size =
      usd->interrupt_buffer_size = ROUNDUP(KLSI_BUF_SIZE, 16);
      status = add_device(usd, uci, comm_epi, data_out_epi, data_in_epi);
    }
  }
  
  TRACE_FUNCRET("< add_klsi_device returns:%08x\n", status);
  return status;
}

status_t reset_klsi_device(usb_serial_device *usd){
  status_t status;
  size_t len = 0;
  TRACE_FUNCALLS("> reset_klsi_device(%08x)\n", usd);
  
  status = (*usb_m->send_request)(usd->dev,
                                  USB_REQTYPE_VENDOR | USB_REQTYPE_INTERFACE_OUT,
                                  KLSI_CONF_REQUEST, 
                                  KLSI_CONF_REQUEST_READ_ON, 0, 0, 
                                  0, 0, &len);
  TRACE_FUNCRET("< reset_klsi_device returns:%08x\n", status);
  return status;
}

status_t set_line_coding_klsi(usb_serial_device *usd,
                              usb_serial_line_coding *line_coding){
  status_t status = B_OK;
  uint8 rate;
  size_t len1 = 0, len2 = 0;
  uint8 coding_packet[5];
  TRACE_FUNCALLS("> set_line_coding_klsi(%08x, {%d, %02x, %02x, %02x})\n",
                                        usd, line_coding->speed,
                                        line_coding->stopbits,
                                        line_coding->parity,
                                        line_coding->databits);
  
  coding_packet[0] = 5; /* size */
  switch (line_coding->speed){
    case 300: rate = klsi_sio_b300; break;
    case 600: rate = klsi_sio_b600; break;
    case 1200: rate = klsi_sio_b1200; break;
    case 2400: rate = klsi_sio_b2400; break;
    case 4800: rate = klsi_sio_b4800; break;
    case 9600: rate = klsi_sio_b9600; break;
    case 19200: rate = klsi_sio_b19200; break;
    case 38400: rate = klsi_sio_b38400; break;
    case 57600: rate = klsi_sio_b57600; break;
    case 115200: rate = klsi_sio_b115200; break;
    default:
      rate = klsi_sio_b19200;
      TRACE_ALWAYS("= set_line_coding_klsi: Datarate:%d is not supported "
                   "by this hardware. Defaulted to %d\n",
                                      line_coding->speed, rate);
    break; 
  }
  coding_packet[1] = rate;
  /* only 7,8 */
  coding_packet[2] = line_coding->databits;
  /* unknown */
  coding_packet[3] = 0;
  coding_packet[4] = 1;
  
  status = (*usb_m->send_request)(usd->dev,
                                  USB_REQTYPE_VENDOR | USB_REQTYPE_INTERFACE_OUT,
                                  KLSI_SET_REQUEST, 0, 0, 0, 
                                  coding_packet, sizeof(coding_packet), &len1);
  if(status != B_OK)
    TRACE_ALWAYS("= set_line_coding_ftdi: datarate set request failed:%08x",
                                                                     status);  

  return status;
}

status_t set_control_line_state_klsi(usb_serial_device *usd, uint16 state){
  status_t status = B_OK;
  return status;
}


void on_read_klsi(usb_serial_device *usd, char **buf, size_t *num_bytes){
  size_t num;
  if (*num_bytes <= 2) {
    *num_bytes = 0;
    return;
  }
  num = B_LENDIAN_TO_HOST_INT16(*(uint16*)(*buf));
  *num_bytes = MIN(num, (*num_bytes - 2));
  *buf += 2;
}

void on_write_klsi(usb_serial_device *usd, const char *buf, size_t *num_bytes){
  if(*num_bytes > usd->write_buffer_size)
    *num_bytes = usd->write_buffer_size;
  if(*num_bytes > 62)
    *num_bytes = 62;
*num_bytes = 6;
  *((uint16*)usd->write_buffer) = B_HOST_TO_LENDIAN_INT16(*num_bytes);
  memcpy(usd->write_buffer+2, buf, *num_bytes);
memcpy(usd->write_buffer+2, "ATDT0\n", 6);
  *num_bytes = 64;
}

void on_close_klsi(usb_serial_device *usd){
  status_t status;
  size_t len = 0;
  TRACE_FUNCALLS("> on_close_klsi(%08x)\n", usd);
  status = (*usb_m->send_request)(usd->dev,
                                  USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
                                  KLSI_CONF_REQUEST, 
                                  KLSI_CONF_REQUEST_READ_OFF, 0, 0, 
                                  0, 0, &len);
  (*usb_m->cancel_queued_transfers)(usd->pipe_read);
  (*usb_m->cancel_queued_transfers)(usd->pipe_write);
}
