/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin, <phoudoin %at% haiku-os %dot% org>
 */


#include <net_buffer.h>
#include <net_device.h>
#include <net_stack.h>

#include <KernelExport.h>

#include <errno.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <new>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#define HDLC_CONTROL_ESCAPE	0x7d
#define HDLC_FLAG_SEQUENCE	0x7e
#define HDLC_ALL_STATIONS	0xff
#define HDLC_UI				0x03


enum dialup_state {
	DOWN,
	DIALING,
	UP,
	HANGINGUP
};

struct dialup_device : net_device {
	int				fd;
	struct termios	line_config;
	dialup_state 	state;
	char			init_string[64];
	char			dial_string[64];
	char			escape_string[8];
	bigtime_t		escape_silence;
	char			hangup_string[16];
};

net_buffer_module_info* gBufferModule;
static net_stack_module_info* sStackModule;

//	#pragma mark -


status_t
dialup_init(const char* name, net_device** _device)
{
	// make sure this is a device in /dev/ports
	if (strncmp(name, "/dev/ports/", 11))
		return B_BAD_VALUE;

	status_t status = get_module(NET_BUFFER_MODULE_NAME, (module_info**)&gBufferModule);
	if (status < B_OK)
		return status;

	dialup_device* device = new (std::nothrow) dialup_device;
	if (device == NULL) {
		put_module(NET_BUFFER_MODULE_NAME);
		return B_NO_MEMORY;
	}

	memset(device, 0, sizeof(dialup_device));

	strcpy(device->name, name);
	device->flags = IFF_POINTOPOINT;
	device->type = IFT_PPP; // this device handle RFC 1331 frame format only
	device->mtu = 1502;
	device->media = 0;
	device->header_length = 8; // HDLC_HEADER_LENGTH;
	device->fd = -1;
	device->state = DOWN;

	// default AT strings
	strncpy(device->init_string, "ATZ", sizeof(device->init_string));
	strncpy(device->dial_string, "ATDT", sizeof(device->dial_string));
	strncpy(device->hangup_string, "ATH0", sizeof(device->hangup_string));

	strncpy(device->escape_string, "+++", sizeof(device->escape_string));
	device->escape_silence = 1000000;

	*_device = device;
	return B_OK;
}


status_t
dialup_uninit(net_device* _device)
{
	dialup_device* device = (dialup_device*)_device;
	delete device;

	put_module(NET_BUFFER_MODULE_NAME);
	return B_OK;
}


status_t
dialup_up(net_device* _device)
{
	dialup_device* device = (dialup_device*)_device;

	device->fd = open(device->name, O_RDWR);
	if (device->fd < 0)
		return errno;

	// init port
	if (ioctl(device->fd, TCGETA, &device->line_config,
		sizeof(device->line_config)) < 0)
		goto err;

	// adjust options
	device->line_config.c_cflag &= ~CBAUD;
	device->line_config.c_cflag |= B115200;	// TODO: make this configurable too...
	device->line_config.c_cflag |= (CLOCAL | CREAD);
	device->line_config.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	device->line_config.c_oflag &= ~OPOST;
	device->line_config.c_cc[VMIN] = 0;
	device->line_config.c_cc[VTIME] = 10;

	// set new options
	if(ioctl(device->fd, TCSETA, &device->line_config,
		sizeof(device->line_config)) < 0)
		goto err;

	// TODO: init modem & start dialing phase
	device->state = DIALING;
	return B_OK;

err:
	close(device->fd);
	device->fd = -1;
	return errno;
}


void
dialup_down(net_device* _device)
{
	dialup_device* device = (dialup_device*)_device;

	// TODO: hangup if needed
	if (device->flags & IFF_LINK) {
		snooze(device->escape_silence);
		if (write(device->fd, device->escape_string,
			strlen(device->escape_string)) > 0) {
			snooze(device->escape_silence);
			// TODO: send hangup string and check for OK ack
			write(device->fd, device->hangup_string,
				strlen(device->hangup_string));
		}
		device->flags &= ~IFF_LINK;
	}

	close(device->fd);
	device->fd = -1;
}


status_t
dialup_control(net_device* _device, int32 op, void* argument,
	size_t length)
{
	dialup_device* device = (dialup_device*)_device;
	return ioctl(device->fd, op, argument, length);
}


status_t
dialup_send_data(net_device* _device, net_buffer* buffer)
{
	return B_NOT_SUPPORTED;
}


status_t
dialup_receive_data(net_device* _device, net_buffer** _buffer)
{
	dialup_device* device = (dialup_device*)_device;

	if (device->fd == -1)
		return B_FILE_ERROR;

	return ENOBUFS;
}


status_t
dialup_set_mtu(net_device* _device, size_t mtu)
{
	dialup_device* device = (dialup_device*)_device;

	device->mtu = mtu;
	return B_OK;
}


status_t
dialup_set_promiscuous(net_device* _device, bool promiscuous)
{
	return B_NOT_SUPPORTED;
}


status_t
dialup_set_media(net_device* device, uint32 media)
{
	return B_NOT_SUPPORTED;
}


status_t
dialup_add_multicast(struct net_device* _device, const sockaddr* _address)
{
	return B_NOT_SUPPORTED;
}


status_t
dialup_remove_multicast(struct net_device* _device, const sockaddr* _address)
{
	return B_NOT_SUPPORTED;
}


static status_t
dialup_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			status_t status = get_module(NET_STACK_MODULE_NAME,
				(module_info**)&sStackModule);
			if (status < B_OK)
				return status;

			return B_OK;
		}

		case B_MODULE_UNINIT:
		{
			put_module(NET_STACK_MODULE_NAME);
			return B_OK;
		}

		default:
			return B_ERROR;
	}
}


net_device_module_info sDialUpModule = {
	{
		"network/devices/dialup/v1",
		0,
		dialup_std_ops
	},
	dialup_init,
	dialup_uninit,
	dialup_up,
	dialup_down,
	dialup_control,
	dialup_send_data,
	dialup_receive_data,
	dialup_set_mtu,
	dialup_set_promiscuous,
	dialup_set_media,
	dialup_add_multicast,
	dialup_remove_multicast,
};

module_info* modules[] = {
	(module_info*)&sDialUpModule,
	NULL
};
