/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef DNS_RESOLVER_H
#define DNS_RESOLVER_H


#include <module.h>


#define DNS_RESOLVER_MODULE_NAME	"network/dns_resolver/v1"


struct dns_resolver_module {
   module_info module;
   status_t (*dns_resolve)(const char* host, uint32* addr);
};


#endif	//	DNS_RESOLVER_H

