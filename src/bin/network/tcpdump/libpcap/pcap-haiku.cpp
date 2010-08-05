/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		James Woodcock
 */


#include "config.h"
#include "pcap-int.h"

#include <OS.h>

#include <sys/socket.h>
#include <sys/sockio.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


bool
prepare_request(struct ifreq& request, const char* name)
{
	if (strlen(name) >= IF_NAMESIZE)
		return false;

	strcpy(request.ifr_name, name);
	return true;
}


static int
pcap_read_haiku(pcap_t* handle, int maxPackets, pcap_handler callback,
	u_char* userdata)
{
	// Receive a single packet

	u_char* buffer = handle->buffer + handle->offset;
	struct sockaddr_dl from;
	ssize_t bytesReceived;
	do {
		if (handle->break_loop) {
			// Clear the break loop flag, and return -2 to indicate our
			// reasoning
			handle->break_loop = 0;
			return -2;
		}

		socklen_t fromLength = sizeof(from);
		bytesReceived = recvfrom(handle->fd, buffer, handle->bufsize, MSG_TRUNC,
			(struct sockaddr*)&from, &fromLength);
	} while (bytesReceived < 0 && errno == B_INTERRUPTED);

	if (bytesReceived < 0) {
		if (errno == B_WOULD_BLOCK) {
			// there is no packet for us
			return 0;
		}

		snprintf(handle->errbuf, sizeof(handle->errbuf),
			"recvfrom: %s", strerror(errno));
		return -1;
	}

	int32 captureLength = bytesReceived;
	if (captureLength > handle->snapshot)
		captureLength = handle->snapshot;

	// run the packet filter
	if (!handle->md.use_bpf && handle->fcode.bf_insns) {
		if (bpf_filter(handle->fcode.bf_insns, buffer, bytesReceived,
				captureLength) == 0) {
			// packet got rejected
			return 0;
		}
	}

	// fill in pcap_header

	pcap_pkthdr header;
	header.caplen = captureLength;
	header.len = bytesReceived;
	header.ts.tv_usec = system_time() % 1000000;
	header.ts.tv_sec = system_time() / 1000000;
		// TODO: get timing from packet!!!

	/* Call the user supplied callback function */
	callback(userdata, &header, buffer);
	return 1;
}


static int
pcap_inject_haiku(pcap_t *handle, const void *buffer, size_t size)
{
	// we don't support injecting packets yet
	// TODO: use the AF_LINK protocol (we need another socket for this) to
	// inject the packets
	strlcpy(handle->errbuf, "Sending packets isn't supported yet",
		PCAP_ERRBUF_SIZE);
	return -1;
}


static int
pcap_setfilter_haiku(pcap_t *handle, struct bpf_program *filter)
{
	// Make our private copy of the filter
	if (install_bpf_program(handle, filter) < 0) {
		// install_bpf_program() filled in errbuf
		return -1;
	}

	// we don't support kernel filters at all
	handle->md.use_bpf = 0;
	return 0;
}


static int
pcap_stats_haiku(pcap_t *handle, struct pcap_stat *stats)
{
	ifreq request;
	prepare_request(request, handle->md.device);

	if (ioctl(handle->fd, SIOCGIFSTATS, &request, sizeof(struct ifreq)) < 0) {
		snprintf(handle->errbuf, PCAP_ERRBUF_SIZE, "pcap_stats: %s",
			strerror(errno));
		return -1;
	}

	handle->md.stat.ps_recv += request.ifr_stats.receive.packets;
	handle->md.stat.ps_drop += request.ifr_stats.receive.dropped;
	*stats = handle->md.stat;
	return 0;
}


//	#pragma mark - pcap API


extern "C" pcap_t *
pcap_open_live(const char *device, int snapLength, int /*promisc*/,
	int /*timeout*/, char *errorBuffer)
{
	// TODO: handle promiscous mode!

	// we need a socket to talk to the networking stack
	int socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (socket < 0) {
		snprintf(errorBuffer, PCAP_ERRBUF_SIZE,
			"The networking stack doesn't seem to be available.\n");
		return NULL;
	}

	struct ifreq request;
	if (!prepare_request(request, device)) {
		snprintf(errorBuffer, PCAP_ERRBUF_SIZE,
			"Interface name \"%s\" is too long.", device);
		close(socket);
		return NULL;
	}

	// check if the interface exist
	if (ioctl(socket, SIOCGIFINDEX, &request, sizeof(request)) < 0) {
		snprintf(errorBuffer, PCAP_ERRBUF_SIZE,
			"Interface \"%s\" does not exist.\n", device);
		close(socket);
		return NULL;
	}

	close(socket);
		// no longer needed after this point

	// get link level interface for this interface

	socket = ::socket(AF_LINK, SOCK_DGRAM, 0);
	if (socket < 0) {
		snprintf(errorBuffer, PCAP_ERRBUF_SIZE, "No link level: %s\n",
			strerror(errno));
		return NULL;
	}

	// start monitoring
	if (ioctl(socket, SIOCSPACKETCAP, &request, sizeof(struct ifreq)) < 0) {
		snprintf(errorBuffer, PCAP_ERRBUF_SIZE, "Cannot start monitoring: %s\n",
			strerror(errno));
		close(socket);
		return NULL;
	}

	pcap_t* handle = (pcap_t*)malloc(sizeof(pcap_t));
	if (handle == NULL) {
		snprintf(errorBuffer, PCAP_ERRBUF_SIZE, "malloc: %s", strerror(errno));
		close(socket);
		return NULL;
	}

	memset(handle, 0, sizeof(pcap_t));

	handle->bufsize = 65536;
		// TODO: should be determined by interface MTU

	// allocate buffer for monitoring the device
	handle->buffer = (u_char*)malloc(handle->bufsize);
	if (handle->buffer == NULL) {
		snprintf(errorBuffer, PCAP_ERRBUF_SIZE, "buffer malloc: %s",
			strerror(errno));
		free(handle);
		close(socket);
		return NULL;
	}

	handle->md.device = strdup(device);
	if (handle->md.device == NULL) {
		snprintf(errorBuffer, PCAP_ERRBUF_SIZE, "strdup device: %s",
			strerror(errno));
		free(handle);
		close(socket);
		return NULL;
	}

	handle->snapshot = snapLength;
	handle->offset = 0;
	handle->selectable_fd = socket;
	handle->fd = socket;
	handle->linktype = DLT_EN10MB;
		// TODO: check interface type!

	handle->read_op = pcap_read_haiku;
	handle->setfilter_op = pcap_setfilter_haiku;
	handle->inject_op = pcap_inject_haiku;
	handle->stats_op = pcap_stats_haiku;

	// use default hooks where possible
	handle->getnonblock_op = pcap_getnonblock_fd;
	handle->setnonblock_op = pcap_setnonblock_fd;
	handle->close_op = pcap_close_common;

	return handle;
}


extern "C" int
pcap_platform_finddevs(pcap_if_t** _allDevices, char* errorBuffer)
{
	// we need a socket to talk to the networking stack
	int socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (socket < 0) {
		snprintf(errorBuffer, PCAP_ERRBUF_SIZE,
			"The networking stack doesn't seem to be available.\n");
		return -1;
	}

	// get a list of all interfaces

	ifconf config;
	config.ifc_len = sizeof(config.ifc_value);
	if (ioctl(socket, SIOCGIFCOUNT, &config, sizeof(struct ifconf)) < 0) {
		close(socket);
		return -1;
	}

	uint32 count = (uint32)config.ifc_value;
	if (count == 0) {
		snprintf(errorBuffer, PCAP_ERRBUF_SIZE,
			"There are no interfaces defined!\n");
		close(socket);
		return -1;
	}

	void* buffer = malloc(count * sizeof(struct ifreq));
	if (buffer == NULL) {
		snprintf(errorBuffer, PCAP_ERRBUF_SIZE, "Out of memory.\n");
		close(socket);
		return -1;
	}

	config.ifc_len = count * sizeof(struct ifreq);
	config.ifc_buf = buffer;
	if (ioctl(socket, SIOCGIFCONF, &config, sizeof(struct ifconf)) < 0) {
		close(socket);
		return -1;
	}

	ifreq* interface = (ifreq*)buffer;

	for (uint32 i = 0; i < count; i++) {
		int flags = 0;

		// get interface type

		int linkSocket = ::socket(AF_LINK, SOCK_DGRAM, 0);
		if (linkSocket < 0) {
			fprintf(stderr, "No link level: %s\n", strerror(errno));
		} else {
			struct ifreq request;
			if (!prepare_request(request, interface->ifr_name)) {
				snprintf(errorBuffer, PCAP_ERRBUF_SIZE,
					"Interface name \"%s\" is too long.", interface->ifr_name);
				close(linkSocket);
				close(socket);
				return -1;
			}

			if (ioctl(linkSocket, SIOCGIFADDR, &request, sizeof(struct ifreq))
					== 0) {
				sockaddr_dl &link = *(sockaddr_dl*)&request.ifr_addr;

				if (link.sdl_type == IFT_LOOP)
					flags = IFF_LOOPBACK;
			}
			close(linkSocket);
		}

		pcap_add_if(_allDevices, interface->ifr_name, flags, NULL, errorBuffer);

		interface = (ifreq*)((uint8*)interface
			+ _SIZEOF_ADDR_IFREQ(interface[0]));
	}

	free(buffer);
	close(socket);
	return 0;
}
