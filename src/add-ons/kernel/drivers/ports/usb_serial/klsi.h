/*
 * Copyright (c) 2003-2004 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 * 
 */

#ifndef _USB_KLSI_H_ 
  #define _USB_KLSI_H_

#define VND_PALM       0x0830
#define VND_KLSI       0x05e9

#define PROD_PALM_CONNECT     0x0080
#define PROD_KLSI_KL5KUSB105D 0x00c0

#define KLSI_SET_REQUEST  0x01
#define KLSI_POLL_REQUEST 0x02
#define KLSI_CONF_REQUEST 0x03
#define KLSI_CONF_REQUEST_READ_ON  0x03
#define KLSI_CONF_REQUEST_READ_OFF 0x02

// not sure
#define KLSI_BUF_SIZE 64

enum {
	klsi_sio_b115200 = 0,
	klsi_sio_b57600 = 1,
	klsi_sio_b38400 = 2,
	klsi_sio_b19200 = 4,
	klsi_sio_b9600 = 6,
	/* unchecked */
	klsi_sio_b4800 = 8,
	klsi_sio_b2400 = 9,
	klsi_sio_b1200 = 10,
	klsi_sio_b600 = 11,
	klsi_sio_b300 = 12,
};


status_t add_klsi_device(usb_serial_device *usd,
                             const usb_configuration_info *uci);
status_t reset_klsi_device(usb_serial_device *usd);
status_t set_line_coding_klsi(usb_serial_device *usd,
                              usb_serial_line_coding *line_coding);

status_t set_control_line_state_klsi(usb_serial_device *usd, uint16 state);
void on_read_klsi(usb_serial_device *usd, char **buf, size_t *num_bytes);
void on_write_klsi(usb_serial_device *usd, const char *buf, size_t *num_bytes);
void on_close_klsi(usb_serial_device *usd);

#endif //_USB_PROLIFIC_H_ 
