#ifndef _PMAP_H

#define _PMAP_H

const int32 PMAP_PORT = 111;      /* portmapper port number */ 
const int32 PMAP_VERSION = 2;
const int32 PMAP_PROGRAM = 100000;

const int32 PMAP_IPPROTO_TCP = 6;      /* protocol number for TCP/IP */ 
const int32 PMAP_IPPROTO_UDP = 17;     /* protocol number for UDP/IP */ 
      
enum
{
	PMAPPROC_NULL		= 0, 
	PMAPPROC_SET		= 1, 	
	PMAPPROC_UNSET		= 2, 
	PMAPPROC_GETPORT	= 3, 
	PMAPPROC_DUMP		= 4, 
	PMAPPROC_CALLIT		= 5   
};

#endif
