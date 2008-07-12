/*
	USB joystick driver for BeOS R5
	Copyright 2000 (C) ITO, Takayuki
	All rights reserved
*/

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <USB3.h>
#include <bus_manager.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "joystick_driver.h"
#include "hidparse.h"

/* HID class-specific definitions */

#define	USB_CLASS_HID	3

#define	USB_DESCRIPTOR_HID			0x21
#define	USB_DESCRIPTOR_HID_REPORT	0x22

#define	USB_REQUEST_HID_GET_REPORT	0x01

typedef struct
{
	uint8	length;
	uint8	descriptor_type;
	uint16	hid_version;
	uint8	country_code;
	uint8	num_descriptors;
	struct
	{
		uint8	descriptor_type;
		uint16	descriptor_length;
	} _PACKED descriptor_info [1];
} _PACKED usb_hid_descriptor;

/* driver specific definitions */

#define	DRIVER_NAME	"usb_joy"

#define	MY_ID	"\033[34m" DRIVER_NAME ":\033[m "
#define	MY_ERR	"\033[31merror:\033[m "
#define	MY_WARN	"\033[31mwarning:\033[m "
#define	assert(x) \
	((x) ? 0 : dprintf (MY_ID "assertion failed at " __FILE__ ", line %d\n", __LINE__))

/* 0-origin */
#define	DEFAULT_CONFIGURATION	0

#define	BUF_SIZ	B_PAGE_SIZE

struct driver_cookie;

/* read from driver settings file */
typedef struct product_info {
	uint16 vendor_id, product_id;
	int8 axis_tbl [MAX_AXES];
	int8 hat_tbl [MAX_HATS];
	int8 button_tbl [MAX_BUTTONS];
} product_info;

#define	FLAG_FORCE_SIGNED	1

typedef struct my_device_info
{
	/* list structure */
	struct my_device_info *next;

	/* maintain device */
	sem_id sem_cb;
	sem_id sem_lock;
	area_id buffer_area;
	void *buffer;
	bool buffer_valid;

	const usb_device *dev;
	int number;

	bool active;
	int open;
	struct driver_cookie *open_fds;

	/* workarea for transfer */
	int usbd_status, bus_status, cmd_status;
	int actual_length;
	const usb_endpoint_info *ept;

	report_insn	*insns;
	size_t num_insns;
	size_t total_report_size;
	int num_buttons, num_axes, num_hats;
	bigtime_t timestamp;
	const product_info *prod_info;
	uint flags;
} my_device_info;

/* driver.c */

extern usb_module_info *usb;
extern const char *my_driver_name;
extern const char *my_base_name;

/* devmgmt.c */

my_device_info *
create_device (const usb_device *dev, const usb_interface_info *ii);

void 
remove_device (my_device_info *my_dev);

/* devlist.c */

extern sem_id my_device_list_lock;
extern bool my_device_list_changed;

void add_device_info (my_device_info *my_dev);
void remove_device_info (my_device_info *my_dev);
my_device_info *search_device_info (int number);

extern char **my_device_names;

void alloc_device_names (void);
void free_device_names (void);
void rebuild_device_names (void);

/* usb_raw.c */

status_t
do_control (my_device_info *my_dev, int request_type, int request,
	int value, int index, size_t length, void *buf);

status_t
do_bulk_sg (my_device_info *my_dev, const usb_endpoint_info *ept, 
	const iovec *vec, size_t vecsiz, size_t length);

status_t
do_bulk (my_device_info *my_dev, const usb_endpoint_info *ept, 
	void *buf, size_t length);

status_t
do_interrupt (my_device_info *my_dev, const usb_endpoint_info *ept, 
	void *buf, size_t length);

