/*
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 * 
 */

#include <KernelExport.h>
#include <Drivers.h>
#include <USB.h>
#include <string.h>
#include <ttylayer.h>

#include "driver.h"
#include "ftdi.h"
#include "uftdireg.h"


status_t add_ftdi_device(usb_serial_device *usd,
                         const usb_configuration_info *uci)
{
  struct usb_endpoint_info *comm_epi = NULL;
  struct usb_endpoint_info *data_out_epi = NULL;
  struct usb_endpoint_info *data_in_epi  = NULL;
  status_t status = ENODEV;
  int i = 0;

  TRACE_FUNCALLS("> add_ftdi_device(%08x, %08x)\n", usd, uci);

  if(uci->interface_count){
    struct usb_interface_info *uii = uci->interface[0].active;
    for(i = 0; i < uii->endpoint_count; i++){
      if(uii->endpoint[i].descr->attributes == USB_EP_ATTR_BULK){
        if((uii->endpoint[i].descr->endpoint_address & USB_EP_ADDR_DIR_IN)
                                                       == USB_EP_ADDR_DIR_IN){
          data_in_epi = &uii->endpoint[i];
        }else{
          if(uii->endpoint[i].descr->endpoint_address
                          /*USB_EP_ADDR_DIR_OUT = 0x0*/){
            comm_epi = data_out_epi = &uii->endpoint[i];
          }
        }
      }
    }
  
    if(comm_epi && data_in_epi && data_out_epi){
      if(usd->hw->product_id == PROD_8U100AX)
        usd->spec.ftdi.hdrlen = 1;
      else
        usd->spec.ftdi.hdrlen = 0;
    
      usd->read_buffer_size = 
      usd->write_buffer_size =
      usd->interrupt_buffer_size = ROUNDUP(FTDI_BUF_SIZE, 16);
      status = add_device(usd, uci, comm_epi, data_out_epi, data_in_epi);
    }
  }
  
  TRACE_FUNCRET("< add_ftdi_device returns:%08x\n", status);
  return status;
}

status_t reset_ftdi_device(usb_serial_device *usd){
  status_t status = B_OK;
  size_t len = 0;
  TRACE_FUNCALLS("> reset_ftdi_device(%08x)\n", usd);
  status = (*usb_m->send_request)(usd->dev,
                                  USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
                                  FTDI_SIO_RESET, FTDI_SIO_RESET_SIO,
                                  FTDI_PIT_DEFAULT, 0, 0, 0, &len);
  TRACE_FUNCRET("< reset_ftdi_device returns:%08x\n", status);
  return status;
}

status_t set_line_coding_ftdi(usb_serial_device *usd,
                              usb_serial_line_coding *line_coding){
  status_t status = B_OK;
  int32 rate, data = 0;
  size_t len1 = 0, len2 = 0;
  TRACE_FUNCALLS("> set_line_coding_ftdi(%08x, {%d, %02x, %02x, %02x})\n",
                                        usd, line_coding->speed,
                                        line_coding->stopbits,
                                        line_coding->parity,
                                        line_coding->databits);
  
  if(usd->hw->product_id == PROD_8U100AX){
    switch (line_coding->speed){
      case 300: rate = ftdi_sio_b300; break;
      case 600: rate = ftdi_sio_b600; break;
      case 1200: rate = ftdi_sio_b1200; break;
      case 2400: rate = ftdi_sio_b2400; break;
      case 4800: rate = ftdi_sio_b4800; break;
      case 9600: rate = ftdi_sio_b9600; break;
      case 19200: rate = ftdi_sio_b19200; break;
      case 38400: rate = ftdi_sio_b38400; break;
      case 57600: rate = ftdi_sio_b57600; break;
      case 115200: rate = ftdi_sio_b115200; break;
      default:
        rate = ftdi_sio_b19200;
        TRACE_ALWAYS("= set_line_coding_ftdi: Datarate:%d is not supported "
                     "by this hardware. Defaulted to %d\n",
                                        line_coding->speed, rate);
      break; 
    }
  }else{
    switch(line_coding->speed){
      case 300: rate = ftdi_8u232am_b300; break;
      case 600: rate = ftdi_8u232am_b600; break;
      case 1200: rate = ftdi_8u232am_b1200; break;
      case 2400: rate = ftdi_8u232am_b2400; break;
      case 4800: rate = ftdi_8u232am_b4800; break;
      case 9600: rate = ftdi_8u232am_b9600; break;
      case 19200: rate = ftdi_8u232am_b19200; break;
      case 38400: rate = ftdi_8u232am_b38400; break;
      case 57600: rate = ftdi_8u232am_b57600; break;
      case 115200: rate = ftdi_8u232am_b115200; break;
      case 230400: rate = ftdi_8u232am_b230400; break;
      case 460800: rate = ftdi_8u232am_b460800; break;
      case 921600: rate = ftdi_8u232am_b921600; break;
      default:
        rate = ftdi_sio_b19200;
        TRACE_ALWAYS("= set_line_coding_ftdi: Datarate:%d is not supported "
                     "by this hardware. Defaulted to %d\n",
                                        line_coding->speed, rate);
        break;
    }
  }
  
  status = (*usb_m->send_request)(usd->dev,
                                  USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
                                  FTDI_SIO_SET_BAUD_RATE, rate,
                                  FTDI_PIT_DEFAULT, 0, 0, 0, &len1);
  if(status != B_OK)
    TRACE_ALWAYS("= set_line_coding_ftdi: datarate set request failed:%08x",
                                                                     status);  

  switch(line_coding->stopbits){
    case LC_STOP_BIT_2: data = FTDI_SIO_SET_DATA_STOP_BITS_2; break;
    case LC_STOP_BIT_1: data = FTDI_SIO_SET_DATA_STOP_BITS_1; break;
    default:
      TRACE_ALWAYS("= set_line_coding_ftdi: Wrong stopbits param:%d",
                                                      line_coding->stopbits);
      break;
  }
  
  switch(line_coding->parity){
    case LC_PARITY_NONE: data |= FTDI_SIO_SET_DATA_PARITY_NONE; break;
    case LC_PARITY_EVEN: data |= FTDI_SIO_SET_DATA_PARITY_EVEN; break;
    case LC_PARITY_ODD:  data |= FTDI_SIO_SET_DATA_PARITY_ODD; break;
    default:
      TRACE_ALWAYS("= set_line_coding_ftdi: Wrong parity param:%d",
                                                        line_coding->parity);
      break;
  }
  
  switch(line_coding->databits){
    case 8: data |= FTDI_SIO_SET_DATA_BITS(8); break;
    case 7: data |= FTDI_SIO_SET_DATA_BITS(7); break;
    default:
      TRACE_ALWAYS("= set_line_coding_ftdi: Wrong databits param:%d",
                                                        line_coding->databits);
      break;
  }

  status = (*usb_m->send_request)(usd->dev,
                                  USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
                                  FTDI_SIO_SET_DATA, data,
                                  FTDI_PIT_DEFAULT, 0, 0, 0, &len2);
  if(status != B_OK)
    TRACE_ALWAYS("= set_line_coding_ftdi: data set request failed:%08x",
                                                                   status);  
  
  TRACE_FUNCRET("< set_line_coding_ftdi returns:%08x\n", status);
  return status;
}

status_t set_control_line_state_ftdi(usb_serial_device *usd, uint16 state){
  status_t status = B_OK;
  int32 ctrl;
  size_t len = 0;
  TRACE_FUNCALLS("> set_control_line_state_ftdi(%08x, %04x)\n", usd, state);
  
  ctrl = (state & CLS_LINE_RTS) ?
                         FTDI_SIO_SET_RTS_HIGH : FTDI_SIO_SET_RTS_LOW;
  ctrl |= (state & CLS_LINE_DTR) ?
                         FTDI_SIO_SET_DTR_HIGH : FTDI_SIO_SET_DTR_LOW;
  status = (*usb_m->send_request)(usd->dev,
                                  USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
                                  FTDI_SIO_MODEM_CTRL, ctrl,
                                  FTDI_PIT_DEFAULT, 0, 0, 0, &len);
  if(status != B_OK)
    TRACE_ALWAYS("= set_control_line_state_ftdi: control set request "
                 "failed:%08x", status);  
  
  TRACE_FUNCRET("< set_control_line_state_ftdi returns:%08x\n", status);
  return status;
}

void on_read_ftdi(usb_serial_device *usd, char **buf, size_t *num_bytes){
  usd->spec.ftdi.status_msr = FTDI_GET_MSR(*buf);
  usd->spec.ftdi.status_lsr = FTDI_GET_LSR(*buf);
  TRACE("MSR:%02x LSR:%02x\n", usd->spec.ftdi.status_msr,
                               usd->spec.ftdi.status_lsr);
  *buf += 2;
  *num_bytes -= 2;
}

void on_write_ftdi(usb_serial_device *usd, const char *buf,
                   size_t *num_bytes){
  char *buf_to = usd->write_buffer;
  int hdrlen = usd->spec.ftdi.hdrlen;
  TRACE("hdrlen:%d\n", hdrlen);
  if(*num_bytes > usd->write_buffer_size - hdrlen)
    *num_bytes = usd->write_buffer_size - hdrlen;
  if(hdrlen > 0)
    *buf_to = FTDI_OUT_TAG(*num_bytes, FTDI_PIT_DEFAULT);
  memcpy(buf_to + hdrlen, buf, *num_bytes);
  *num_bytes += hdrlen;
}

void on_close_ftdi(usb_serial_device *usd){
  TRACE_FUNCALLS("> on_close_ftdi(%08x)\n", usd);
  (*usb_m->cancel_queued_transfers)(usd->pipe_read);
  (*usb_m->cancel_queued_transfers)(usd->pipe_write);
}

