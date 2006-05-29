/*
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 * 
 */

#ifndef _USB_ACM_H_ 
  #define _USB_ACM_H_

status_t add_acm_device(usb_serial_device *usd,
                        const usb_configuration_info *uci);
status_t reset_acm_device(usb_serial_device *usd);
status_t set_line_coding_acm(usb_serial_device *usd,
                             usb_serial_line_coding *line_coding);
status_t set_control_line_state_acm(usb_serial_device *usd, uint16 state);
void on_read_acm(usb_serial_device *usd, char **buf, size_t *num_bytes);
void on_write_acm(usb_serial_device *usd, const char *buf, size_t *num_bytes);
void on_close_acm(usb_serial_device *usd);

#endif //_USB_ACM_H_ 
