/*
 * Copyright 2008-2016, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include <debug.h>
#include <driver_settings.h>
#include <netinet/in.h>
#include <sys/socket.h>


#define NETCONSOLE_PORT 6666


static int gSocket;


static int
createSocket()
{
	gSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (gSocket < 0)
		return B_ERROR;

	// bind socket
	sockaddr_in fSocketAddress;
	fSocketAddress.sin_family = AF_INET;
	fSocketAddress.sin_port = 0;
	fSocketAddress.sin_addr.s_addr = INADDR_ANY;
	fSocketAddress.sin_len = sizeof(sockaddr_in);
	if (bind(gSocket, (sockaddr*)&fSocketAddress, sizeof(fSocketAddress)) < 0) {
		return B_ERROR;
	}

	// set SO_BROADCAST on socket
	int soBroadcastValue = 1;
	if (setsockopt(gSocket, SOL_SOCKET, SO_BROADCAST, &soBroadcastValue,
			sizeof(soBroadcastValue)) < 0) {
		return B_ERROR;
	}

	return B_OK;
}


// FIXME this can't work this way, because debugger_puts is called with
// interrupts disabled and can't send to the network directly. Must be reworked
// to use a buffer, and do the network access from another thread. A similar
// solution is implemented to get syslog data to the syslog_daemon.
static int
debugger_puts(const char* message, int32 length)
{
	if (gSocket < 0)
		createSocket();

	if (gSocket >= 0) {
		// init server address to broadcast
		sockaddr_in fServerAddress;
		fServerAddress.sin_family = AF_INET;
		fServerAddress.sin_port = htons(NETCONSOLE_PORT);
		fServerAddress.sin_addr.s_addr = htonl(INADDR_BROADCAST);
		fServerAddress.sin_len = sizeof(sockaddr_in);

		return sendto(gSocket, message, length, 0,
			(sockaddr*)&fServerAddress, sizeof(fServerAddress));
	}

	return 0;
}


static status_t
std_ops(int32 op, ...)
{
	void* handle;
	bool load = false;

	switch (op) {
		case B_MODULE_INIT:
			gSocket = -1;
			return B_OK;
#if 0
			handle = load_driver_settings("kernel");
			if (handle) {
				load = get_driver_boolean_parameter(handle,
					"netconsole_debug_output", load, true);
				unload_driver_settings(handle);
			}
			if (load)
				gSocket = -1;
			return load ? B_OK : B_ERROR;
#endif
		case B_MODULE_UNINIT:
			if (gSocket >= 0)
				close(gSocket);
			return B_OK;
	}
	return B_BAD_VALUE;
}


static struct debugger_module_info sModuleInfo = {
	{
		"debugger/netconsole/v1",
		0,
		&std_ops
	},
	NULL,
	NULL,
	debugger_puts,
	NULL
};

module_info* modules[] = {
	(module_info*)&sModuleInfo,
	NULL
};

