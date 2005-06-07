#ifndef _LINK_MESSAGE_H_
#define _LINK_MESSAGE_H_


#include <SupportDefs.h>


struct message_header {
	int32	size;
	uint32	code;
	uint32	flags;
};

#endif	/* _LINK_MESSAGE_H_ */
