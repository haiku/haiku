/*
 * Copyright 2004-2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Andre Alves Garzia, andre@andregarzia.com
 */
 
#include "settings.h"

#include <String.h>
#include <File.h>
#include <Path.h>
 
#include <SupportDefs.h>
#include <AutoDeleter.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

Settings::Settings(const char *name)
{
	fSocket = socket(AF_INET, SOCK_DGRAM, 0);
	fName = name;
	ReadConfiguration();	
}

Settings::~Settings()
{
	close(fSocket);
	
}

bool
Settings::_PrepareRequest(struct ifreq& request)
{
	//This function is used for talking direct to the stack. 
	//It´s used by _ShowConfiguration.
	
	const char* name = fName.String();
	
	if (strlen(name) > IF_NAMESIZE)
		return false;

	strcpy(request.ifr_name, name);
	return true;
}


void
Settings::ReadConfiguration() 
{
	
	ifreq request;
	if (!_PrepareRequest(request))
		return;
		
	BString text = "dummy";
	char address[32];
	sockaddr_in* inetAddress = NULL;
	
	// Obtain IP.	
	if (ioctl(fSocket, SIOCGIFADDR, &request,
			sizeof(request)) < 0) {
		return;
	}

	inetAddress = (sockaddr_in*)&request.ifr_addr;
	if (inet_ntop(AF_INET, &inetAddress->sin_addr, address,
				sizeof(address)) == NULL) {
			return;
		}
		
	fIP = address;
	
	// Obtain netmask.
	if (ioctl(fSocket, SIOCGIFNETMASK, &request,
			sizeof(request)) < 0) {
		return;
	}
	
	inetAddress = (sockaddr_in*)&request.ifr_mask;
	if (inet_ntop(AF_INET, &inetAddress->sin_addr, address,
				sizeof(address)) == NULL) {
			return;
		}
		
	fNetmask = address;
	
	// Obtain gateway
	
	char* gwAddress;
	ifconf config;
	config.ifc_len = sizeof(config.ifc_value);
	if (ioctl(fSocket, SIOCGRTSIZE, &config, sizeof(struct ifconf)) < 0)
		return;
		
	uint32 size = (uint32)config.ifc_value;
	if (size == 0)
		return;
	
	void *buffer = malloc(size);
	if (buffer == NULL)
		return;
	
	MemoryDeleter bufferDeleter(buffer);
	config.ifc_len = size;
	config.ifc_buf = buffer;
	
	if (ioctl(fSocket, SIOCGRTTABLE, &config, sizeof(struct ifconf)) < 0)
		return;
		
	ifreq *interface = (ifreq *)buffer;
	ifreq *end = (ifreq *)((uint8 *)buffer + size);
	
	
	while (interface < end) {
		route_entry& route = interface->ifr_route;
	
		
		if (route.flags & RTF_GATEWAY) {
			inetAddress = (sockaddr_in *)route.gateway;
					
			
			gwAddress = inet_ntoa(inetAddress->sin_addr);
			fGateway = gwAddress;
		}
		
		int32 addressSize = 0;
		if (route.destination != NULL)
			addressSize += route.destination->sa_len;
		if (route.mask != NULL)
			addressSize += route.mask->sa_len;
		if (route.gateway != NULL)
			addressSize += route.gateway->sa_len;
			
		interface = (ifreq *)((addr_t)interface + 
			IF_NAMESIZE + sizeof(route_entry) + addressSize);
	
	}
	
	bufferDeleter.Detach();
	free(buffer);	
	
	// Now that silly DNS problem...
	fNameservers.MakeEmpty();

#define	MATCH(line, name) \
	(!strncmp(line, name, sizeof(name) - 1) && \
	(line[sizeof(name) - 1] == ' ' || \
	 line[sizeof(name) - 1] == '\t'))

		
	int nserv = 0;
	char buf[1024];
	register FILE *fp;
	register char *cp, **pp;
	register int n;
	int MAXNS = 2;
	
	if ((fp = fopen("/etc/resolv.conf", "r")) != NULL) {
	    /* read the config file */
	    while (fgets(buf, sizeof(buf), fp) != NULL) {
			/* skip comments */
			if (*buf == ';' || *buf == '#')
				continue;
			
		    /* read nameservers to query */
			if (MATCH(buf, "nameserver") && nserv < MAXNS) {
				char sbuf[2];

				cp = buf + sizeof("nameserver") - 1;
				while (*cp == ' ' || *cp == '\t')
					cp++;
				cp[strcspn(cp, ";# \t\n")] = '\0';
				if ((*cp != '\0') && (*cp != '\n')) {
					fNameservers.AddItem(new BString(cp));
					nserv++;
				}
			}
			continue;
		}
		fclose(fp);
	}
}
