/*
	USB Joystick driver for BeOS R5
	Copyright 2000 (C) ITO, Takayuki
	All rights reserved
*/

#include <Debug.h>
#include <driver_settings.h>
#include <limits.h>
#include <stdlib.h>
#include "driver.h"

#define	AXIS_VALUE_MIN	(-32768)
#define	AXIS_VALUE_MAX	32767

#define	USB_VID_MICROSOFT	0x045e

#define	DPRINTF_INFO(x)	dprintf x
#define	DPRINTF_ERR(x)	dprintf x

/* gameport driver cookie (per open) */

typedef struct driver_cookie
{
	struct driver_cookie	*next;

	my_device_info			*my_dev;
	bool					enhanced;

	/* below are valid under enhanced mode */
	joystick_module_info	joy_mod_info;
} driver_cookie;
	
/* NB global variables are valid only while driver is loaded */

_EXPORT int32	api_version = B_CUR_DRIVER_API_VERSION;

const char *my_driver_name = "usb_joy";
const char *my_base_name = "joystick/usb/";

usb_module_info *usb;

/*
	driver settings
*/

static product_info *prod_info = NULL;
static int num_prod_info = 0;

#define	atoi2(s)	(int)strtol((s),NULL,0)

static void load_settings_file (void)
{
	void *handle;
	int entry_idx, param_idx, i;
	static product_info null_prod_info;
	const driver_settings *settings;

	/* make default mapping */
	null_prod_info.vendor_id = null_prod_info.product_id = 0;
	for (i = 0; i < MAX_AXES; i++)
		null_prod_info.axis_tbl [i] = i;
	for (i = 0; i < MAX_HATS; i++)
		null_prod_info.hat_tbl [i] = i;
	for (i = 0; i < MAX_BUTTONS; i++)
		null_prod_info.button_tbl [i] = i;

	handle = load_driver_settings (my_driver_name);
	if (handle == NULL)
	{
		DPRINTF_ERR ((MY_ID "settings file open error\n"));
		prod_info = &null_prod_info;
		num_prod_info = 0;
		return;
	}

	settings = get_driver_settings (handle);
	/* FIXME: num_prod_info may not be equal to parameter_count */
	num_prod_info = settings->parameter_count;
	if (num_prod_info == 0)
	{
		prod_info = &null_prod_info;
		return;
	}
	prod_info = malloc (sizeof (product_info) * (num_prod_info + 1));
	if (prod_info == NULL)
	{
		DPRINTF_ERR ((MY_ID "no memory\n"));
		prod_info = &null_prod_info;
		num_prod_info = 0;
		return;
	}

	prod_info [0] = null_prod_info;
	for (entry_idx = 0; entry_idx < num_prod_info; entry_idx++)
	{
		product_info *prod;
		const driver_parameter *entry;

		entry = &settings->parameters [entry_idx];
		if (strcmp (entry->name, "version") == 0)
		{
			/* XXX */
		}
		else if (strcmp (entry->name, "product") == 0)
		{
			prod = &prod_info [entry_idx + 1];
			*prod = null_prod_info;
			assert (entry->value_count == 2);
			prod->vendor_id  = atoi2 (entry->values [0]);
			prod->product_id = atoi2 (entry->values [1]);

			for (param_idx = 0; param_idx < entry->parameter_count; param_idx++)
			{
				const driver_parameter *param;

				param = &entry->parameters [param_idx];
				if (strcmp (param->name, "button") == 0)
				{
					int src, dst;
					assert (param->value_count >= 2);
					src = atoi2 (param->values [0]) - 1;
					dst = atoi2 (param->values [1]) - 1;
					if (src < MAX_BUTTONS && dst < MAX_BUTTONS)
						prod->button_tbl [src] = dst;
				}
				else if (strcmp (param->name, "axis") == 0)
				{
					int src, dst;
					assert (param->value_count >= 2);
					src = atoi2 (param->values [0]) - 1;
					dst = atoi2 (param->values [1]) - 1;
					if (src < MAX_AXES && dst < MAX_AXES)
						prod->axis_tbl [src] = dst;
				}
				else if (strcmp (param->name, "hat") == 0)
				{
					int src, dst;
					assert (param->value_count >= 2);
					src = atoi2 (param->values [0]) - 1;
					dst = atoi2 (param->values [1]) - 1;
					if (src < MAX_HATS && dst < MAX_HATS)
						prod->hat_tbl [src] = dst;
				}
			}
		}
	}
	unload_driver_settings (handle);
}

/*
	callback: got a report, issue next request
*/

static void usb_callback (
	void *cookie, 
	uint32 status, 
	void *data, 
	uint32 actual_len)
{
	status_t st;
	my_device_info *my_dev = cookie;

	assert (cookie != NULL);

	acquire_sem (my_dev->sem_lock);
	my_dev->actual_length = actual_len;
	my_dev->bus_status = status;	/* B_USB_STATUS_* */
	if (status != B_OK)
	{
		/* request failed */
		release_sem (my_dev->sem_lock);
		DPRINTF_ERR ((MY_ID "bus status %d\n", (int)status));
		//if (status == B_USB_STATUS_IRP_CANCELLED_BY_REQUEST)
		//{
			/* cancelled: device is unplugged */
			return;
		//}
#if 1
		//st = usb->clear_feature (my_dev->ept->handle, USB_FEATURE_ENDPOINT_HALT);
		//if (st != B_OK)
		//	DPRINTF_ERR ((MY_ID "clear_feature() error %d\n", (int)st));
#endif
	}
	else
	{
		/* got a report */
#if 0
		int i;
		char linbuf [256];
		uint8 *buffer = my_dev->buffer;

		for (i = 0; i < my_dev->total_report_size; i++)
			sprintf (&linbuf[i*3], "%02X ", buffer [i]);
		DPRINTF_INFO ((MY_ID "input report: %s\n", linbuf));
#endif
		my_dev->buffer_valid = true;
		my_dev->timestamp = system_time ();
		release_sem (my_dev->sem_lock);
	}

	/* issue next request */

	st = usb->queue_interrupt (my_dev->ept->handle, my_dev->buffer,
		my_dev->total_report_size, usb_callback, my_dev);
	if (st != B_OK)
	{
		/* XXX probably endpoint stall */
		DPRINTF_ERR ((MY_ID "queue_interrupt() error %d\n", (int)st));
	}
}

/*
	USB specific device hooks
*/

static status_t my_device_added (
	const usb_device *dev, 
	void **cookie)
{
	my_device_info *my_dev;
	const usb_device_descriptor *dev_desc;
	const usb_configuration_info *conf;
	const usb_interface_info *intf;
	status_t st;
	usb_hid_descriptor *hid_desc;
	uint8 *rep_desc = NULL;
	size_t desc_len, actual;
	decomp_item *items;
	size_t num_items;
	int i, ifno, fd, report_id;

	assert (dev != NULL && cookie != NULL);
	DPRINTF_INFO ((MY_ID "device_added()\n"));

	dev_desc = usb->get_device_descriptor (dev);
	DPRINTF_INFO ((MY_ID "vendor ID 0x%04X, product ID 0x%04X\n",
		dev_desc->vendor_id, dev_desc->product_id));

	/* check interface class */

	if ((conf = usb->get_nth_configuration 
		(dev, DEFAULT_CONFIGURATION)) == NULL)
	{
		DPRINTF_ERR ((MY_ID "cannot get default configuration\n"));
		return B_ERROR;
	}

	for (ifno = 0; ifno < conf->interface_count; ifno++)
	{
		/* This is C; I can use "class" :-> */
		int class, subclass, protocol;

		intf = conf->interface [ifno].active;
		class    = intf->descr->interface_class;
		subclass = intf->descr->interface_subclass;
		protocol = intf->descr->interface_protocol;
		DPRINTF_INFO ((MY_ID "interface %d: class %d, subclass %d, protocol %d\n",
			ifno, class, subclass, protocol));
		if (class == USB_CLASS_HID && subclass == 0)
			break;
	}

	if (ifno >= conf->interface_count)
	{
		DPRINTF_INFO ((MY_ID "Non-boot HID interface not found\n"));
		return B_ERROR;
	}

	/* read HID descriptor */

	desc_len = sizeof (usb_hid_descriptor);
	hid_desc = malloc (desc_len);
	st = usb->send_request (dev, 
		USB_REQTYPE_INTERFACE_IN | USB_REQTYPE_STANDARD,
		USB_REQUEST_GET_DESCRIPTOR,
		USB_DESCRIPTOR_HID << 8, ifno, desc_len, 
		hid_desc, &desc_len);
	DPRINTF_INFO ((MY_ID "get_hid_desc: st=%d, len=%d\n", 
		(int) st, (int)desc_len));
	if (st != B_OK)
		desc_len = 256;		/* XXX */

	/* read report descriptor */

	desc_len = hid_desc->descriptor_info [0].descriptor_length;
	free (hid_desc);
	rep_desc = malloc (desc_len);
	assert (rep_desc != NULL);
	st = usb->send_request (dev, 
		USB_REQTYPE_INTERFACE_IN | USB_REQTYPE_STANDARD,
		USB_REQUEST_GET_DESCRIPTOR,
		USB_DESCRIPTOR_HID_REPORT << 8, ifno, desc_len, 
		rep_desc, &desc_len);
	DPRINTF_INFO ((MY_ID "get_hid_rep_desc: st=%d, len=%d\n", 
		(int) st, (int)desc_len));
	if (st != B_OK)
	{
		free (rep_desc);
		return B_ERROR;
	}

	/* save report descriptor for troubleshooting */

	fd = open ("/tmp/rep_desc.bin", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd >= 0)
	{
		write (fd, rep_desc, desc_len);
		close (fd);
	}

	/* XXX check application type */
	/* Generic Desktop : Joystick or Game Pad */
	/* or SideWinder Strategic Commander */

	if (memcmp (rep_desc, "\x05\x01\x09\x04", 4) != 0 &&
	    memcmp (rep_desc, "\x05\x01\x09\x05", 4) != 0 &&
	    memcmp (rep_desc, "\x05\x01\x09\x00", 4) != 0)
	{
		DPRINTF_INFO ((MY_ID "not a game controller\n"));
		free (rep_desc);
		return B_ERROR;
	}

	/* configuration */

	if ((st = usb->set_configuration (dev, conf)) != B_OK)
	{
		DPRINTF_ERR ((MY_ID "set_configuration() failed %d\n", (int)st));
		free (rep_desc);
		return B_ERROR;
	}

	if ((my_dev = create_device (dev, intf)) == NULL)
	{
		free (rep_desc);
		return B_ERROR;
	}

	/* decompose report descriptor */

	num_items = desc_len;	/* XXX */
	items = malloc (sizeof (decomp_item) * num_items);
	assert (items != NULL);
	decompose_report_descriptor (rep_desc, desc_len, items, &num_items);
	free (rep_desc);

	/* parse report descriptor */

	my_dev->num_insns = num_items;	/* XXX */
	my_dev->insns = malloc (sizeof (report_insn) * my_dev->num_insns);
	assert (my_dev->insns != NULL);
	parse_report_descriptor (items, num_items, my_dev->insns, 
		&my_dev->num_insns, &my_dev->total_report_size, &report_id);
	free (items);
	realloc (my_dev->insns, sizeof (report_insn) * my_dev->num_insns);
	DPRINTF_INFO ((MY_ID "%d items, %d insns, %d bytes\n", 
		(int)num_items, (int)my_dev->num_insns, (int)my_dev->total_report_size));

	/* count axes, hats and buttons */

	count_controls (my_dev->insns, my_dev->num_insns,
		&my_dev->num_axes, &my_dev->num_hats, &my_dev->num_buttons);
	DPRINTF_INFO ((MY_ID "%d axes, %d hats, %d buttons\n",
		my_dev->num_axes, my_dev->num_hats, my_dev->num_buttons));
	if (my_dev->num_axes > MAX_AXES)
		my_dev->num_axes = MAX_AXES;
	if (my_dev->num_buttons > MAX_BUTTONS)
		my_dev->num_buttons = MAX_BUTTONS;
	if (my_dev->num_hats > MAX_HATS)
		my_dev->num_hats = MAX_HATS;

	/* lookup driver settings */

	my_dev->prod_info = &prod_info [0];		/* default mapping */
	for (i = 0; i < num_prod_info; i++)
	{
		const product_info *p = &prod_info [i + 1];
		if (p->vendor_id  == dev_desc->vendor_id &&
		    p->product_id == dev_desc->product_id)
		{
			my_dev->prod_info = p;
			DPRINTF_INFO ((MY_ID "settings found for 0x%04x 0x%04x\n",
				p->vendor_id, p->product_id));
			break;
		}
	}

	/* XXX */
	if (dev_desc->vendor_id == USB_VID_MICROSOFT)
	{
		DPRINTF_INFO ((MY_ID "Microsoft product\n"));
		my_dev->flags = FLAG_FORCE_SIGNED;
	}

	/* get initial state */

	st = usb->send_request (dev,
		USB_REQTYPE_INTERFACE_IN | USB_REQTYPE_CLASS,
		USB_REQUEST_HID_GET_REPORT,
		0x0100 | report_id, ifno, my_dev->total_report_size,
		my_dev->buffer, &actual);
	if (st == B_OK)
		my_dev->buffer_valid = true;
	else
		DPRINTF_ERR ((MY_ID "Get_Report failed %d\n", (int)st));
	my_dev->timestamp = system_time ();

	/* issue interrupt transfer */

	my_dev->ept = &intf->endpoint [0];		/* interrupt IN */
	st = usb->queue_interrupt (my_dev->ept->handle, my_dev->buffer,
		my_dev->total_report_size, usb_callback, my_dev);
	if (st != B_OK)
	{
		DPRINTF_ERR ((MY_ID "queue_interrupt() error %d\n", (int)st));
		return B_ERROR;
	}

	/* create a port */

	add_device_info (my_dev);

	*cookie = my_dev;
	DPRINTF_INFO ((MY_ID "added %d\n", my_dev->number));

	return B_OK;
}

static status_t my_device_removed (void *cookie)
{
	my_device_info *my_dev = cookie;

	assert (cookie != NULL);

	DPRINTF_INFO ((MY_ID "device_removed(%d)\n", my_dev->number));
	usb->cancel_queued_transfers (my_dev->ept->handle);
	remove_device_info (my_dev);
	if (my_dev->open == 0)
	{
		if (my_dev->insns != NULL)
			free (my_dev->insns);
		remove_device (my_dev);
	}
	else
	{
		DPRINTF_INFO ((MY_ID "%s%d still open\n", my_base_name, my_dev->number));
		my_dev->active = false;
	}
	return B_OK;
}

static usb_notify_hooks my_notify_hooks =
{
	my_device_added, my_device_removed
};

#define	SUPPORTED_DEVICES	1
usb_support_descriptor my_supported_devices [SUPPORTED_DEVICES] =
{
	{ USB_CLASS_HID, 0, 0, 0, 0 },
};


/* ----------
	my_device_open - handle open() calls
----- */

static status_t my_device_open
	(const char *name,
	uint32 flags,
	driver_cookie **out_cookie)
{
	driver_cookie *cookie;
	my_device_info *my_dev;
	int number;
	const char *p;

	assert (name != NULL);
	assert (out_cookie != NULL);
	DPRINTF_INFO ((MY_ID "open(%s)\n", name));

	if (strlen (name) < strlen (my_base_name) + 1)	/* "0" */
		return B_ENTRY_NOT_FOUND;
	p = &name [strlen (my_base_name)];
	number = atoi (p);

	if ((my_dev = search_device_info (number)) == NULL)
		return B_ENTRY_NOT_FOUND;
	if ((cookie = malloc (sizeof (driver_cookie))) == NULL)
		return B_NO_MEMORY;

	acquire_sem (my_dev->sem_lock);
	cookie->enhanced = false;
	cookie->my_dev = my_dev;
	cookie->next = my_dev->open_fds;
	my_dev->open_fds = cookie;
	my_dev->open++;
	release_sem (my_dev->sem_lock);

	*out_cookie = cookie;
	DPRINTF_INFO ((MY_ID "device %d open (%d)\n", number, my_dev->open));
	return B_OK;
}

/*
	interpret input report
*/

static int sign_extend
	(int value, 
	int size)
{
	if (value & (1 << (size - 1)))
		return value | (UINT_MAX << size);
	else
		return value;
}

static void interpret_report
	(uint8 *report, 
	size_t total_report_size,
	const report_insn *insns, 
	size_t num_insns,
	const product_info *prod_info, 
	uint flags,
	extended_joystick *joy_data)
{
	int i, j, axis_idx, hat_idx;

	assert (report != NULL);
	assert (insns != NULL);
	assert (prod_info != NULL);
	assert (joy_data != NULL);

	report [total_report_size] = 0;		/* guard */
	joy_data->axes [0] = 0; 		/* X and Y axes are handled specially */
	joy_data->axes [1] = 0;
	axis_idx = 2;
	hat_idx = 0;
	joy_data->buttons = 0;
	for (i = 0; i < num_insns; i++)
	{
		const report_insn *insn = &insns [i];
		int32 value = 
			(((report [insn->byte_idx + 1] << 8) | 
			   report [insn->byte_idx]) >> insn->bit_pos) 
			& ((1 << insn->num_bits) - 1);
		int32 range;

		switch (insn->ctrl_type)
		{
		case TYPE_BUTTON:
			if (insn->attributes & 2)	/* variable */
			{
				if (insn->usage.id_min - 1 < MAX_BUTTONS)
					joy_data->buttons |= 
						(value & 1) << prod_info->button_tbl [insn->usage.id_min - 1];
			}
			else
			{
				value = value - insn->log_min + insn->usage.id_min;
				if (value - 1 < MAX_BUTTONS)
					joy_data->buttons |= 1 << prod_info->button_tbl [value - 1];
			}
			break;

		case TYPE_AXIS_X:
		case TYPE_AXIS_Y:
		case TYPE_AXIS:
			/*
				literal values returned by device should be bound by
				logical min/max, but actually physical min/max 
				in case of game controllers
			*/
			if (insn->is_phy_signed)
			{
				/* rarely */
				range = insn->signed_phy_max - insn->signed_phy_min + 1;
				value = sign_extend (value, insn->num_bits);
			}
			else
			{
				range = insn->phy_max - insn->phy_min + 1;
				if ((flags & FLAG_FORCE_SIGNED) && insn->is_log_signed)
					/* MS SideWinder GamePad Pro, FreeStyle Pro, 
					Strategic Commander */
					value = sign_extend (value, insn->num_bits);
				else
					/* most vendors, MS SideWinder USB GamePad, 
					SideWinder Joystick */
					value -= range >> 1;
			}
			value = value * (AXIS_VALUE_MAX - AXIS_VALUE_MIN + 1) / range;
			switch (insn->ctrl_type)
			{
			case TYPE_AXIS_X:	j = 0;			break;
			case TYPE_AXIS_Y:	j = 1;			break;
			case TYPE_AXIS:		j = axis_idx++;	break;
			}
			joy_data->axes [prod_info->axis_tbl [j]] = value;
			break;

		case TYPE_HAT:
			if (value < insn->log_min || insn->log_max < value)
				value = 0;		/* null state */
			else
			{
				int num_pos = insn->log_max - insn->log_min + 1;
				value -= insn->log_min;
				switch (num_pos)
				{
				case 4:
					/* 4-pos (0,1,2,3) to 8-pos (0,2,4,6) */
					value <<= 1;
					break;
				case 8:
					/* do nothing */
					break;
				default:
					/* XXX */
					DPRINTF_ERR ((MY_ID "%d-position hat unsupported\n", num_pos));
					break;
				}
				value++;		/* 1 origin */
			}
			joy_data->hats [prod_info->hat_tbl [hat_idx++]] = value;
			break;
		}
	}
}

/* ----------
	my_device_read - handle read() calls
----- */

static status_t my_device_read
	(driver_cookie *cookie,
	off_t position,
	void *buf,
	size_t *num_bytes)
{
	status_t err = B_ERROR;
	my_device_info *my_dev;
	extended_joystick joy_data;

	assert (cookie != NULL);
	assert (buf != NULL);
	assert (num_bytes != NULL);
	my_dev = cookie->my_dev;
	assert (my_dev != NULL);
//	DPRINTF_INFO ((MY_ID "read(%d)\n", my_dev->number));

	if (!my_dev->active)
		return B_ERROR;		/* already unplugged */

	memset (&joy_data, 0, sizeof (extended_joystick));
	if (my_dev->buffer_valid)
	{
		interpret_report (my_dev->buffer, my_dev->total_report_size,
			my_dev->insns, my_dev->num_insns, 
			my_dev->prod_info, my_dev->flags, &joy_data);
	}

	if (cookie->enhanced && *num_bytes >= sizeof (extended_joystick))
	{
		/* enhanced mode */
		joy_data.timestamp = my_dev->timestamp;
		memcpy (buf, &joy_data, sizeof (extended_joystick));
		*num_bytes = sizeof (extended_joystick);
		err = B_OK;
	}

	if (!cookie->enhanced && *num_bytes >= sizeof (joystick))
	{
		/* standard mode */
		joystick std_joy_data;

		std_joy_data.timestamp = my_dev->timestamp;
		std_joy_data.horizontal = joy_data.axes [0];
		std_joy_data.vertical   = joy_data.axes [1];
		/* false if pressed */
		std_joy_data.button1 = (joy_data.buttons & 1) ? false : true;
		std_joy_data.button2 = (joy_data.buttons & 2) ? false : true;
		memcpy (buf, &std_joy_data, sizeof (joystick));
		*num_bytes = sizeof (joystick);
		err = B_OK;
	}

	return err;
}


/* ----------
	my_device_write - handle write() calls
----- */

static status_t my_device_write
	(driver_cookie *cookie,
	off_t position,
	const void *buf,
	size_t *num_bytes)
{
	return B_ERROR;
}

/* ----------
	my_device_control - handle ioctl calls
----- */

static status_t my_device_control
	(driver_cookie *cookie,
	uint32 op,
	void *arg,
	size_t len)
{
	status_t err = B_ERROR;
	my_device_info *my_dev;

	assert (cookie != NULL);
	my_dev = cookie->my_dev;
	assert (my_dev != NULL);
	DPRINTF_INFO ((MY_ID "ioctl(%u)\n", (int)op));

	if (!my_dev->active)
		return B_ERROR;		/* already unplugged */

	switch (op)
	{
	case B_JOYSTICK_SET_DEVICE_MODULE:
		/* called by BJoystick, Joysticks pref */
		/*
			if this ioctl returns successfully, 
			Joysticks preference assumes that 
			the joystick product may be connected
		*/
		assert (arg != NULL);
		if (!cookie->enhanced &&
			strstr (((joystick_module_info*)arg)->device_name, "USB") != NULL)	/* XXX */
		{
			cookie->joy_mod_info = *(joystick_module_info *)arg;

			/* currently I don't use a joystick module */

			/* set actual # of controls detected */
			/* this will be returned in B_JOYSTICK_GET_DEVICE_MODULE */
			cookie->joy_mod_info.num_axes = my_dev->num_axes;
			cookie->joy_mod_info.num_hats = my_dev->num_hats;
			cookie->joy_mod_info.num_buttons = my_dev->num_buttons;

			/* enter enhanced mode */
			cookie->enhanced = true;
			err = B_OK;
		}
		break;

	case B_JOYSTICK_GET_DEVICE_MODULE:
		/* return modified info */
		assert (arg != NULL);
		if (cookie->enhanced)
		{
			*(joystick_module_info *)arg = cookie->joy_mod_info;
			err = B_OK;
		}
		break;

	case B_JOYSTICK_GET_SPEED_COMPENSATION:
	case B_JOYSTICK_SET_SPEED_COMPENSATION:
	case B_JOYSTICK_GET_MAX_LATENCY:
	case B_JOYSTICK_SET_MAX_LATENCY:
	case B_JOYSTICK_SET_RAW_MODE:
	default:
		/* not implemented */
		break;
	}

	return err;
}


/* ----------
	my_device_close - handle close() calls
----- */

static status_t my_device_close
	(driver_cookie *cookie)
{
	my_device_info *my_dev;

	assert (cookie != NULL && cookie->my_dev != NULL);
	my_dev = cookie->my_dev;
	DPRINTF_INFO ((MY_ID "close(%d)\n", my_dev->number));

	/* detach the cookie from list */

	acquire_sem (my_dev->sem_lock);
	if (my_dev->open_fds == cookie)
		my_dev->open_fds = cookie->next;
	else
	{
		driver_cookie *p;
		for (p = my_dev->open_fds; p != NULL; p = p->next)
		{
			if (p->next == cookie)
			{
				p->next = cookie->next;
				break;
			}
		}
	}
	--my_dev->open;
	release_sem (my_dev->sem_lock);

	return B_OK;
}


/* -----
	my_device_free - called after the last device is closed, and after
	all i/o is complete.
----- */
static status_t my_device_free
	(driver_cookie *cookie)
{
	my_device_info *my_dev;

	assert (cookie != NULL && cookie->my_dev != NULL);
	my_dev = cookie->my_dev;
	DPRINTF_INFO ((MY_ID "free(%d)\n", my_dev->number));

	free (cookie);
	if (my_dev->open > 0)
		DPRINTF_INFO ((MY_ID "%d opens left\n", my_dev->open));
	else if (!my_dev->active)
	{
		DPRINTF_INFO ((MY_ID "removed %d\n", my_dev->number));
		if (my_dev->insns != NULL)
			free (my_dev->insns);
		remove_device (my_dev);
	}

	return B_OK;
}


/* -----
	function pointers for the device hooks entry points
----- */

static device_hooks my_device_hooks = {
	(device_open_hook)    my_device_open,
	(device_close_hook)   my_device_close,
	(device_free_hook)    my_device_free,
	(device_control_hook) my_device_control,
	(device_read_hook)    my_device_read,
	(device_write_hook)   my_device_write,
	NULL, NULL, NULL, NULL
};


/* ----------
	init_hardware - called once the first time the driver is loaded
----- */
_EXPORT status_t init_hardware (void)
{
	DPRINTF_INFO ((MY_ID "init_hardware() " __DATE__ " " __TIME__ "\n"));
	return B_OK;
}

/* ----------
	init_driver - optional function - called every time the driver
	is loaded.
----- */
_EXPORT status_t init_driver (void)
{

	DPRINTF_INFO ((MY_ID "init_driver() " __DATE__ " " __TIME__ "\n"));

	if (get_module (B_USB_MODULE_NAME, (module_info **) &usb) != B_OK)
		return B_ERROR;

	if ((my_device_list_lock = create_sem (1, "dev_list_lock")) < 0)
	{
		put_module (B_USB_MODULE_NAME);
		return my_device_list_lock;		/* error code */
	}

	load_settings_file ();

	usb->register_driver (my_driver_name, my_supported_devices, 
		SUPPORTED_DEVICES, NULL);
	usb->install_notify (my_driver_name, &my_notify_hooks);
	DPRINTF_INFO ((MY_ID "init_driver() OK\n"));

	return B_OK;
}


/* ----------
	uninit_driver - optional function - called every time the driver
	is unloaded
----- */
_EXPORT void uninit_driver (void)
{
	DPRINTF_INFO ((MY_ID "uninit_driver()\n"));
	usb->uninstall_notify (my_driver_name);
	if (prod_info != NULL && num_prod_info > 0)
		free (prod_info);
	delete_sem (my_device_list_lock);
	put_module (B_USB_MODULE_NAME);
	free_device_names ();
}

/*
	publish_devices
	device names are generated dynamically
*/

_EXPORT const char **publish_devices (void)
{
	DPRINTF_INFO ((MY_ID "publish_devices()\n"));

	if (my_device_list_changed)
	{
		free_device_names ();
		alloc_device_names ();
		if (my_device_names != NULL)
			rebuild_device_names ();
		my_device_list_changed = false;
	}
	assert (my_device_names != NULL);
	return (const char **) my_device_names;
}

/* ----------
	find_device - return ptr to device hooks structure for a
	given device name
----- */

_EXPORT device_hooks *find_device
	(const char *name)
{
	assert (name != NULL);
	DPRINTF_INFO ((MY_ID "find_device(%s)\n", name));
	if (strncmp (name, my_base_name, strlen (my_base_name)) != 0)
		return NULL;
	return &my_device_hooks;
}

