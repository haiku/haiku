// LSHOSTS.C

// BeOS-specific includes
#include "TypeConstants.h"
#include "Errors.h"
#include "OS.h"

// POSIX includes
#include "stdio.h"
#include "malloc.h"
#include "string.h"
#include "errno.h"
#include "socket.h"
#include "netdb.h"
#include "signal.h"

#include "betalk.h"
#include "sysdepdefs.h"

#ifndef NULL
#define NULL			0L
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET		(int)(~0)
#endif

int main(int argc, char *argv[]);
static void recvAlarm(int signal);
static void usage();


int main(int argc, char *argv[])
{
	bt_request request;
	struct sockaddr_in ourAddr, toAddr, fromAddr;
	struct hostent *ent;
	char buffer[8192];
	int sock, addrLen, bytes;
	unsigned int serverIP;
#ifdef SO_BROADCAST
	int on = 1;
#endif

	if (argc > 2)
		usage();

	memset(&toAddr, 0, sizeof(toAddr));
	toAddr.sin_family = AF_INET;
	toAddr.sin_port = htons(BT_QUERYHOST_PORT);
	if (argc == 1)
		toAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	else
	{
		ent = gethostbyname(argv[1]);
		if (ent == NULL)
			return 0;

		serverIP = ntohl(*((unsigned int *) ent->h_addr));
		toAddr.sin_addr.s_addr = htonl(serverIP);
	}

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
		return 0;

	memset(&ourAddr, 0, sizeof(ourAddr));
	ourAddr.sin_family = AF_INET;
	ourAddr.sin_port = htons(BT_QUERYHOST_PORT);
	ourAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sock, (struct sockaddr *) &ourAddr, sizeof(ourAddr)))
		if (errno != EADDRINUSE)
			return 0;

	// Normally, a setsockopt() call is necessary to turn broadcast mode on
	// explicitly, although some versions of Unix don't care.  BeOS doesn't
	// currently even define SO_BROADCAST, unless you have BONE installed.
#ifdef SO_BROADCAST
	if (argc == 1)
		if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0)
		{
			printf("Unable to obtain permission for a network broadcast.\n");
			closesocket(sock);
			return 0;
		}
#endif
	signal(SIGALRM, recvAlarm);

	strcpy(request.signature, BT_RPC_SIGNATURE);
	request.command = (argc == 1) ? BT_REQ_HOST_PROBE : BT_REQ_SHARE_PROBE;
	if (sendto(sock, (char *) &request, sizeof(request), 0, (struct sockaddr *) &toAddr, sizeof(toAddr)) == -1)
	{
		printf("%s\n", strerror(errno));
		closesocket(sock);
		return 0;
	}

	memset(buffer, 0, sizeof(buffer));
	alarm(3);

	while (1)
	{
		addrLen = sizeof(fromAddr);
		bytes = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *) &fromAddr, &addrLen);
		if (bytes < 0)
		{
			if (errno == EINTR)
				break;
		}

		if (strncmp(buffer, BT_RPC_SIGNATURE, strlen(BT_RPC_SIGNATURE)) != 0)
			if (argc == 1)
			{
				struct sockaddr_in *sin = (struct sockaddr_in *) &fromAddr;
				ent = gethostbyaddr((char *) &sin->sin_addr, sizeof(sin->sin_addr), AF_INET);
				if (ent)
					printf("%s\n", ent->h_name);
				else
				{
					uint8 *p = (uint8 *) &sin->sin_addr;
					printf("%d.%d.%d.%d\n", p[0], p[1], p[2], p[3]);
				}
			}
			else
			{
				bt_resource resource;
				int bufPos = 0;
				while (bufPos < bytes)
				{
					memcpy(&resource, buffer + bufPos, sizeof(bt_resource));
					resource.type = B_LENDIAN_TO_HOST_INT32(resource.type);
					resource.subType = B_LENDIAN_TO_HOST_INT32(resource.subType);
					if (resource.type == BT_SHARED_NULL)
						break;

					bufPos += sizeof(bt_resource);
					printf("%s (%s)\n", resource.name,
						resource.type == BT_SHARED_FOLDER ? "Shared Files" : "Shared Printer");
				}
			}
	}

	alarm(0);
	signal(SIGALRM, SIG_DFL);
	closesocket(sock);
}

static void recvAlarm(int signal)
{
	return;
}

static void usage()
{
	printf("Usage: lshosts [host]\n");
	exit(0);
}
