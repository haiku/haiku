/* some misc functions... */

#include <stdio.h>
#include <strings.h>

#ifdef _KERNEL_MODE
#include <KernelExport.h>
#endif

#include "net_misc.h"
#include "sys/socket.h"

/* Basically use dump_ to see the address plus message on a line,
 * print_ to simply have the address printed with nothing else...
 */

void dump_ipv4_addr(char *msg, void *ad)
{
        uint8 *b = (uint8*)ad;
        printf("%s %d.%d.%d.%d\n", msg, b[0], b[1], b[2], b[3]);
}

void print_ipv4_addr(void *ad)
{
        uint8 *b = (uint8*)ad;
        printf("%d.%d.%d.%d", b[0], b[1], b[2], b[3]);
}

void dump_ether_addr(char *msg, void *ea)
{
	uint8 *b = (uint8*)ea;
        printf("%s %02x:%02x:%02x:%02x:%02x:%02x\n", msg,
		b[0], b[1], b[2],
		b[3], b[4], b[5]);
}

void print_ether_addr(void *ea)
{
	uint8 *b = (uint8*)ea;
	printf("%02x:%02x:%02x:%02x:%02x:%02x",
		b[0], b[1], b[2],
                b[3], b[4], b[5]);
}

void dump_buffer(char *buffer, int len) 
{
	uint8 *b = (uint8 *)buffer;
	int i;

	printf ("  ");
	for (i=0;i<len;i++) {
		if (i%16 == 0)
			printf("\n  ");
		if (i%2 == 0)
			printf(" %02x", b[i]);
		else
			printf("%02x ", b[i]);
	}
	printf("\n\n");
}

int compare_sockaddr(struct sockaddr *a, struct sockaddr *b)
{
	if (a->sa_len == 4) /* IPv4 address, basically a uint32 */
		return (*(a->sa_data) = *(b->sa_data));
	return memcmp((void*)a->sa_data, (void*)b->sa_data, a->sa_len);
}

