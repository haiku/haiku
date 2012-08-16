/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef DNS_RESOLVER_H
#define DNS_RESOLVER_H


#include <netdb.h>
#include <stdlib.h>

#include <module.h>


#define DNS_RESOLVER_MODULE_NAME	"network/dns_resolver/v1"


struct dns_resolver_module {
	module_info module;
	status_t (*getaddrinfo)(const char* node, const char* service,
		const struct addrinfo* hints, struct addrinfo** res);
};


static inline int
kgetaddrinfo(const char* node, const char* service,
	const struct addrinfo* hints, struct addrinfo** res)
{
	dns_resolver_module* dns;
	status_t result = get_module(DNS_RESOLVER_MODULE_NAME,
		reinterpret_cast<module_info**>(&dns));
	if (result != B_OK)
		return result;

	result = dns->getaddrinfo(node, service, hints, res);

	put_module(DNS_RESOLVER_MODULE_NAME);

	return result;
}


static inline void
kfreeaddrinfo(struct addrinfo* res)
{
	free(res);
}


#define getaddrinfo		kgetaddrinfo
#define freeaddrinfo	kfreeaddrinfo


#endif	//	DNS_RESOLVER_H

