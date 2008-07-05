/*
 * Copyright 2008, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT license.
 */


#include <NetAddress.h>

#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void
checkNetAddr(const BNetAddress& netAddr, uint32 nwAddr, uint16 nwPort)
{
	int32 status;
	in_addr addr;
	unsigned short port;
	if ((status = netAddr.GetAddr(addr, &port)) != B_OK) {
		fprintf(stderr, 
			"failed to get in_addr/port from localhost address: %s\n",
			strerror(status));
		exit(1);
	}
	if (addr.s_addr != nwAddr) {
		fprintf(stderr, "expected in_addr %lx - got %x\n", nwAddr, addr.s_addr);
		exit(1);
	}
	uint16 hostPort = ntohs(nwPort);
	if (port != hostPort) {
		fprintf(stderr, "expected port %u - got %u\n", hostPort, port);
		exit(1);
	}

	sockaddr_in sockaddr;
	if ((status = netAddr.GetAddr(sockaddr)) != B_OK) {
		fprintf(stderr, 
			"failed to get sockaddr_in from netAddr: %s\n",
			strerror(status));
		exit(1);
	}
	if (sockaddr.sin_family != AF_INET) {
		fprintf(stderr, "expected sockaddr-family %u - got %u\n", AF_INET,
			sockaddr.sin_family);
		exit(1);
	}
	if (sockaddr.sin_port != nwPort) {
		fprintf(stderr, "expected sockaddr-port %x - got %x\n", nwPort,
			sockaddr.sin_port);
		exit(1);
	}
	if (sockaddr.sin_addr.s_addr != nwAddr) {
		fprintf(stderr, "expected sockaddr-address %lx - got %x\n", nwAddr,
			sockaddr.sin_addr.s_addr);
		exit(1);
	}
}


int
main(int argc, const char* const* argv)
{
	BNetAddress netAddr;
	if (sizeof(netAddr) != 52) {
		fprintf(stderr, "expected sizeof(netAddr) to be 52 - is %ld\n",
			sizeof(netAddr));
		exit(1);
	}

	netAddr.SetTo("localhost", 123);
	checkNetAddr(netAddr, htonl(0x7F000001), htons(123));

	netAddr.SetTo(htonl(0x7F000001), 123);
	checkNetAddr(netAddr, htonl(0x7F000001), htons(123));

	in_addr addr;
	addr.s_addr = htonl(0x7F000001);
	netAddr.SetTo(addr, 54321);
	checkNetAddr(netAddr, htonl(0x7F000001), htons(54321));

	sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = htonl(0x7F000001);
	sockaddr.sin_port = htons(54321);
	netAddr.SetTo(sockaddr);
	checkNetAddr(netAddr, htonl(0x7F000001), htons(54321));

	printf("Everything went fine.\n");
	
	return 0;
}
