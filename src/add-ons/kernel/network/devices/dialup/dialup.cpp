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
#include <sys/uio.h>


#define HDLC_FLAG_SEQUENCE	0x7e
#define HDLC_CONTROL_ESCAPE	0x7d

#define HDLC_ALL_STATIONS	0xff
#define HDLC_UI				0x03

#define HDLC_HEADER_LENGTH	4


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
	bigtime_t		last_closing_flag_sequence_time;
	bool			data_mode;
	char			init_string[64];
	char			dial_string[64];
	char			escape_string[8];
	bigtime_t		escape_silence;
	char			hangup_string[16];
	bigtime_t		tx_flag_timeout;
	uint32			rx_accm;
	uint32			tx_accm[8];
};

net_buffer_module_info* gBufferModule;
static net_stack_module_info* sStackModule;


//	#pragma mark -


static status_t
switch_to_command_mode(dialup_device* device)
{
	if (device->state != UP)
		return B_ERROR;

	if (!device->data_mode)
		return B_OK;

	snooze(device->escape_silence);

	ssize_t size = write(device->fd, device->escape_string,
			strlen(device->escape_string));
	if (size != (ssize_t)strlen(device->escape_string))
		return B_IO_ERROR;

	snooze(device->escape_silence);
	device->data_mode = false;
	return B_OK;
}


#if 0
static status_t
switch_to_data_mode(dialup_device* device)
{
	if (device->state != UP)
		return B_ERROR;

	if (device->data_mode)
		return B_OK;

	// TODO: check if it's needed, as these days any
	// escaped AT commands switch back to data mode automatically
	// after their completion...
	ssize_t size = write(device->fd, "ATO", 3);
	if (size != 3)
		return B_IO_ERROR;

	device->data_mode = true;
	return B_OK;
}
#endif


static status_t
send_command(dialup_device* device, const char* command)
{
	status_t status;
	if (device->data_mode) {
		status = switch_to_command_mode(device);
		if (status != B_OK)
			return status;
	}

	ssize_t bytesWritten = write(device->fd, command, strlen(command));
	if (bytesWritten != (ssize_t)strlen(command))
		return B_IO_ERROR;

	if (write(device->fd, "\r", 1) != 1)
		return B_IO_ERROR;

	return B_OK;
}


static status_t
read_command_reply(dialup_device* device, const char* command,
	char* reply, int replyMaxSize)
{
	if (device->data_mode)
		return B_ERROR;

	int i = 0;
	while (i < replyMaxSize) {

		ssize_t bytesRead = read(device->fd, &reply[i], 1);
		if (bytesRead != 1)
			return B_IO_ERROR;

		if (reply[i] == '\n') {
			// filter linefeed char
			continue;
		}

		if (reply[i] == '\r') {
			reply[i] = '\0';

			// is command reply or command echo (if any) ?
			if (!strcasecmp(reply, command))
				return B_OK;

			// It's command echo line. Just ignore it.
			i = 0;
			continue;
		}
		i++;
	}

	// replyMaxSize not large enough to store the full reply line.
	return B_NO_MEMORY;
}


static status_t
hangup(dialup_device* device)
{
	if (device->state != UP)
		return B_ERROR;

	// TODO: turn device's DTR down instead. Or do that too after sending command
	char reply[8];

	if (send_command(device, device->hangup_string) != B_OK
		|| read_command_reply(device, device->hangup_string,
			reply, sizeof(reply)) != B_OK
		|| strcmp(reply, "OK"))
		return B_ERROR;

	device->state = DOWN;
	return B_OK;
}


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

	dialup_device* device = new (std::nothrow)dialup_device;
	if (device == NULL) {
		put_module(NET_BUFFER_MODULE_NAME);
		return B_NO_MEMORY;
	}

	memset(device, 0, sizeof(dialup_device));

	strcpy(device->name, name);
	device->flags = IFF_POINTOPOINT;
	device->type = IFT_PPP; // this device handle RFC 1331 frame format only
	device->mtu = 1500;
	device->media = 0;
	device->header_length = HDLC_HEADER_LENGTH;

	device->fd = -1;
	device->state = DOWN;
	device->data_mode = false;
	device->last_closing_flag_sequence_time = 0;

	// default AT strings
	strncpy(device->init_string, "ATZ", sizeof(device->init_string));
	strncpy(device->dial_string, "ATDT", sizeof(device->dial_string));
	strncpy(device->hangup_string, "ATH0", sizeof(device->hangup_string));

	strncpy(device->escape_string, "+++", sizeof(device->escape_string));
	device->escape_silence = 1000000;

	device->tx_flag_timeout = 1000000;

	// default rx & tx Async-Control-Character-Map
	memset(&device->rx_accm, 0xFF, sizeof(device->rx_accm));
	memset(&device->tx_accm, 0xFF, sizeof(device->tx_accm));

	*_device = device;
	return B_OK;
}


status_t
dialup_uninit(net_device* _device)
{
	dialup_device* device = (dialup_device*)_device;
	delete device;

	put_module(NET_BUFFER_MODULE_NAME);
	gBufferModule = NULL;
	return B_OK;
}


status_t
dialup_up(net_device* _device)
{
	dialup_device* device = (dialup_device*)_device;

	device->fd = open(device->name, O_RDWR);
	if (device->fd < 0)
		return errno;

	device->media = IFM_ACTIVE;

	// init port
	if (ioctl(device->fd, TCGETA, &device->line_config,
		sizeof(device->line_config)) < 0)
		goto err;

	// adjust options
	device->line_config.c_cflag &= ~CBAUD;
	device->line_config.c_cflag &= CSIZE;
	device->line_config.c_cflag &= CS8;
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

	// init modem & start dialing phase

	char reply[32];

	if (strlen(device->init_string) > 0) {
		// Send modem init string
		if (send_command(device, device->init_string) != B_OK
			|| read_command_reply(device, device->init_string,
				reply, sizeof(reply)) != B_OK
			|| strcmp(reply, "OK")) {
			errno = B_IO_ERROR;
			goto err;
		}
	}

	reply[0] = '\0';

	if (strlen(device->dial_string) > 0) {
		// Send dialing string
		device->state = DIALING;
		if (send_command(device, device->dial_string) != B_OK
			|| read_command_reply(device, device->dial_string,
				reply, sizeof(reply)) != B_OK
			|| strncmp(reply, "CONNECT", 7)) {
			errno = B_IO_ERROR;
			goto err;
		}
	}

	device->state = UP;
	device->data_mode = true;

	device->media |= IFM_FULL_DUPLEX;
	device->flags |= IFF_LINK;

	device->link_quality = 1000;
	if (strlen(reply) > 7) {
		// get speed from "CONNECTxxxx" reply
		device->link_speed = atoi(&reply[8]);
	} else {
		// Set default speed (theorically, it could be 300 bits/s even)
		device->link_speed = 19200;
	}

	return B_OK;

err:
	close(device->fd);
	device->fd = -1;
	device->media = 0;

	return errno;
}


void
dialup_down(net_device* _device)
{
	dialup_device* device = (dialup_device*)_device;

	if (device->flags & IFF_LINK
		&& hangup(device) == B_OK)
		device->flags &= ~IFF_LINK;

	close(device->fd);
	device->fd = -1;
	device->media = 0;
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
	dialup_device* device = (dialup_device*)_device;

	if (device->fd == -1)
		return B_FILE_ERROR;

	dprintf("try to send HDLC packet of %lu bytes (flags %ld):\n", buffer->size, buffer->flags);

	if (buffer->size < HDLC_HEADER_LENGTH)
		return B_BAD_VALUE;

	iovec* ioVectors = NULL;
	iovec* ioVector;
	uint8* packet = NULL;
	int packetSize = 0;
	status_t status;
	ssize_t bytesWritten;

	uint32 vectorCount = gBufferModule->count_iovecs(buffer);
	if (vectorCount < 1) {
		status = B_BAD_VALUE;
		goto err;
	}

	ioVectors = (iovec*)malloc(sizeof(iovec)*vectorCount);
	if (ioVectors == NULL) {
		status = B_NO_MEMORY;
		goto err;
	}
	gBufferModule->get_iovecs(buffer, ioVectors, vectorCount);

	// encode HDLC packet

	// worst case: begin and end sequence flags plus each payload byte escaped
	packet = (uint8*)malloc(2 + 2 * buffer->size);
	if (packet == NULL) {
		status = B_NO_MEMORY;
		goto err;
	}

	// Mark frame start if the prior frame closing flag was sent
	// more than a second ago.
	// Otherwise, the prior closing flag sequence is the open flag of this
	// frame
	if (device->tx_flag_timeout
		&& system_time() - device->last_closing_flag_sequence_time
			> device->tx_flag_timeout) {
		packet[packetSize++] = HDLC_FLAG_SEQUENCE;
	}

	// encode frame data
	ioVector = ioVectors;
	while (vectorCount--) {
		uint8* data = (uint8*) ioVector->iov_base;
		for (unsigned int i = 0; i < ioVector->iov_len; i++) {
			if (data[i] < 0x20
				|| data[i] == HDLC_FLAG_SEQUENCE
				|| data[i] == HDLC_CONTROL_ESCAPE) {
				// needs escape
				packet[packetSize++] = HDLC_CONTROL_ESCAPE;
				packet[packetSize++] = data[i] ^ 0x20;
			} else
				packet[packetSize++] = data[i];
		}
		// next io vector
		ioVector++;
	}

	// mark frame end
	packet[packetSize++] = HDLC_FLAG_SEQUENCE;

	// send HDLC packet

	bytesWritten = write(device->fd, packet, packetSize);
	if (bytesWritten < 0) {
		status = errno;
		goto err;
	}
	device->last_closing_flag_sequence_time = system_time();

	device->stats.send.packets++;
	device->stats.send.bytes += bytesWritten;
	status = B_OK;
	goto done;

err:
	device->stats.send.errors++;

done:
	free(ioVectors);
	free(packet);
	return status;
}


status_t
dialup_receive_data(net_device* _device, net_buffer** _buffer)
{
	dialup_device* device = (dialup_device*)_device;

	if (device->fd == -1)
		return B_FILE_ERROR;

	net_buffer* buffer = gBufferModule->create(256);
	if (buffer == NULL)
		return ENOBUFS;

	status_t status;
	ssize_t bytesRead;
	uint8* data = NULL;
	uint8* packet = (uint8*)malloc(2 + 2 * buffer->size);
	if (packet == NULL) {
		status = B_NO_MEMORY;
		goto err;
	}

	status = gBufferModule->append_size(buffer,
		device->mtu + HDLC_HEADER_LENGTH, (void**)&data);
	if (status == B_OK && data == NULL) {
		dprintf("scattered I/O is not yet supported by dialup device.\n");
		status = B_NOT_SUPPORTED;
	}
	if (status < B_OK)
		goto err;

	while (true) {
		bytesRead = read(device->fd, data, device->mtu + HDLC_HEADER_LENGTH);
		if (bytesRead < 0) {
			// TODO
		}
	}

	status = gBufferModule->trim(buffer, bytesRead);
	if (status < B_OK) {
		device->stats.receive.dropped++;
		goto err;
	}

	device->stats.receive.bytes += bytesRead;
	device->stats.receive.packets++;

	*_buffer = buffer;
	status = B_OK;
	goto done;

err:
	gBufferModule->free(buffer);
	device->stats.receive.errors++;

done:
	free(packet);
	return status;
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
