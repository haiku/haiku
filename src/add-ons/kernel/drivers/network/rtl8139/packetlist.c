#include "packetlist.h"
#include <malloc.h>

static net_packet_list_t *firstpacketlist = 0;
static net_packet_list_t *lastpacketlist = 0;
static sem_id mysem;

void append_packet( void *pointer , int16 size )
{
	net_packet_list_t *temp;
	
	temp = (net_packet_list_t *) malloc( sizeof( net_packet_list_t ) );
	temp->packet = (net_packet_t *) malloc( sizeof( net_packet_t ) );
	temp->next = 0;
	temp->packet->len = size;
	temp->packet->buffer = (void *) malloc( size );
	memcpy( temp->packet->buffer , pointer , size );
	
	//This is the first packet
	if (firstpacketlist == 0 )
		firstpacketlist = temp;
	
	if ( lastpacketlist == 0 )
		lastpacketlist = temp;
	else
	{
		lastpacketlist->next = temp;
		lastpacketlist = temp;
	}
	//Release the semaphore with 1, so with each packet read can do a run
	release_sem_etc( mysem , 1 , B_DO_NOT_RESCHEDULE );
}

net_packet_t *get_packet()
{
	net_packet_list_t *temp;
	net_packet_t *retval;
	
	if ( firstpacketlist == 0 )
		return 0;
		
	temp = firstpacketlist->next;
	retval = firstpacketlist->packet;
	free( firstpacketlist );
	
	if ( temp == 0 ) //There are no more packets in teh queue
	{
		firstpacketlist = 0;
		lastpacketlist = 0;
	}
	return retval;
}
	
void set_sem( sem_id sem )
{
	mysem = sem;
}
