/* attribute.h
 * private definitions for network attributes
 */

#ifndef OBOS_NET_STACK_ATTRIBUTE_H
#define OBOS_NET_STACK_ATTRIBUTE_H

#include <iovec.h>

#include <SupportDefs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct net_attribute
{
	struct net_attribute *next;
	const void *id;
	uint32 type;
	uint32 size;
	union {
		bool	boolean;
		uint8	byte;
		uint16	word;
		uint32	dword;
		uint64	ddword;
		char 	data[256];
		void 	*ptr;
		iovec 	vec[16];	// Beware: 16 max :-(
	} u;
} net_attribute;

extern const void *	 	register_attribute_id(const char *name);	
extern net_attribute *	new_attribute(net_attribute **in_list, const void *id);
extern status_t		  	delete_attribute(net_attribute *attribut, net_attribute **from_list);
extern net_attribute * 	find_attribute(net_attribute *list, const void *id, int *type, void **value, size_t *size);

#ifdef __cplusplus
}
#endif

#endif	// OBOS_NET_STACK_ATTRIBUTE_H
