/* This file describes a netpacket kernel buffer */

#include <KernelExport.h>

typedef struct net_packet 
{
	void *buffer;	/* Contains the packet without CRC */
	uint16 len;		/* Contains the length of the packet */
} net_packet_t;

struct net_packet_list;

typedef struct net_packet_list {
	net_packet_t *packet; /* Contains a pointer to the current packet */
	struct net_packet_list *next; /* Contains a pointer to the next packet -- if 0, then ther is none */
} net_packet_list_t;


net_packet_t *get_packet();
void append_packet( void *pointer , int16 size );
void set_sem( sem_id sem );
