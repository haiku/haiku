/*
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 * 
 */

#ifndef _USB_FTDI_H_ 
  #define _USB_FTDI_H_

#define VND_FTDI     0x0403

#define PROD_8U100AX 0x8372
#define PROD_8U232AM 0x6001

#define FTDI_BUF_SIZE 0x40

status_t add_ftdi_device(usb_serial_device *usd,
                         const usb_configuration_info *uci);
status_t reset_ftdi_device(usb_serial_device *usd);
status_t set_line_coding_ftdi(usb_serial_device *usd,
                              usb_serial_line_coding *line_coding);
status_t set_control_line_state_ftdi(usb_serial_device *usd, uint16 state);
void on_read_ftdi(usb_serial_device *usd, char **buf, size_t *num_bytes);
void on_write_ftdi(usb_serial_device *usd, const char *buf, size_t *num_bytes);
void on_close_ftdi(usb_serial_device *usd);

#endif //_USB_FTDI_H_ 
