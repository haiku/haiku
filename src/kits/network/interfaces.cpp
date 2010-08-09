/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <net/if.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <AutoDeleter.h>


namespace BPrivate {
	class Socket {
		public:
			Socket()
			{
				fFD = socket(AF_INET, SOCK_DGRAM, 0);
			}

			~Socket()
			{
				if (fFD >= 0)
					close(fFD);
			}

			int FD() const { return fFD; }

		private:
			int	fFD;
	};
}	// namespace BPrivate


//	#pragma mark -


unsigned
if_nametoindex(const char* name)
{
	BPrivate::Socket socket;
	if (socket.FD() < 0)
		return 0;

	ifreq request;
	strlcpy(request.ifr_name, name, IF_NAMESIZE);
	if (ioctl(socket.FD(), SIOCGIFINDEX, &request, sizeof(struct ifreq)) < 0)
		return 0;

	return request.ifr_index;
}


char*
if_indextoname(unsigned index, char* nameBuffer)
{
	BPrivate::Socket socket;
	if (socket.FD() < 0)
		return NULL;

	ifreq request;
	request.ifr_index = index;
	if (ioctl(socket.FD(), SIOCGIFNAME, &request, sizeof(struct ifreq)) < 0)
		return NULL;

	strlcpy(nameBuffer, request.ifr_name, IF_NAMESIZE);
	return nameBuffer;
}


struct if_nameindex*
if_nameindex(void)
{
	BPrivate::Socket socket;
	if (socket.FD() < 0)
		return NULL;

	// get a list of all interfaces

	ifconf config;
	config.ifc_len = sizeof(config.ifc_value);
	if (ioctl(socket.FD(), SIOCGIFCOUNT, &config, sizeof(struct ifconf)) < 0)
		return NULL;

	int count = (int)config.ifc_value;
	ifreq* interfaces = (ifreq*)malloc(count * sizeof(struct ifreq));
	if (interfaces == NULL)
		return NULL;

	MemoryDeleter deleter(interfaces);

	config.ifc_len = count * sizeof(struct ifreq);
	config.ifc_req = interfaces;
	if (ioctl(socket.FD(), SIOCGIFCONF, &config, sizeof(struct ifconf)) < 0)
		return NULL;

	struct if_nameindex* interfaceArray = (struct if_nameindex*)malloc(
		sizeof(struct if_nameindex) * (count + 1));
	if (interfaceArray == NULL)
		return NULL;

	for (int i = 0; i < count; i++) {
		interfaceArray[i].if_index = interfaces->ifr_index;
		interfaceArray[i].if_name = strdup(interfaces->ifr_name);

		interfaces
			= (ifreq*)((char*)interfaces + _SIZEOF_ADDR_IFREQ(interfaces[0]));
	}

	interfaceArray[count].if_index = 0;
	interfaceArray[count].if_name = NULL;

	return interfaceArray;
}


void
if_freenameindex(struct if_nameindex *interfaceArray)
{
	for (int i = 0; interfaceArray[i].if_name; i++) {
		free(interfaceArray[i].if_name);
	}
	free(interfaceArray);
}

