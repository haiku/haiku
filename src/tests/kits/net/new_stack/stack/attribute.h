/* attribute.h
 * private definitions for network attributes
 */

#ifndef OBOS_NET_STACK_ATTRIBUTE_H
#define OBOS_NET_STACK_ATTRIBUTE_H

#include <iovec.h>
#include <stdarg.h>

#include <SupportDefs.h>

#include "net_stack.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ATTRIBUTE_IOVECS	(16)
#define MAX_ATTRIBUTE_DATA_LEN	(256)

struct net_attribute {
	struct net_attribute *next;
	string_token id;
	uint32 type;
	size_t size;
	union {
		bool	boolean;
		uint8	byte;
		uint16	word;
		uint32	dword;
		uint64	ddword;
		char 	data[MAX_ATTRIBUTE_DATA_LEN];	// Beware: data max size is limited :-(
		void 	*ptr;
		iovec 	vec[MAX_ATTRIBUTE_IOVECS];		// Beware: these are limited too :-(
	} u;
};

extern status_t 		add_attribute(net_attribute **in_list, string_token id, int type, va_list args);
extern net_attribute *	new_attribute(net_attribute **in_list, string_token id);
extern status_t		  	delete_attribute(net_attribute *attribut, net_attribute **from_list);
extern net_attribute * 	find_attribute(net_attribute *list, string_token id, int index, int *type, void **value, size_t *size);

#ifdef __cplusplus
}
#endif

#endif	// OBOS_NET_STACK_ATTRIBUTE_H
