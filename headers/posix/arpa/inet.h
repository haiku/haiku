/*
 * Copyright 2009-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _INET_H_
#define	_INET_H_


#include <netinet/in.h>
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/types.h>


#ifdef __cplusplus
extern "C" {
#endif

in_addr_t		inet_addr(const char* addressString);
int				inet_aton(const char* addressString, struct in_addr* address);
char*			inet_cidr_ntop(int family, const void* source, int bits,
					char* dest, size_t destSize);
int				inet_cidr_pton(int family, const char* sourceString, void* dest,
					int* _bits);
u_long			inet_lnaof(struct in_addr address);
struct in_addr	inet_makeaddr(u_long net, u_long host);
char*			inet_net_ntop(int family, const void* source, int bits,
					char* dest, size_t destSize);
int				inet_net_pton(int family, const char* sourceString, void* dest,
					size_t destSize);
char*			inet_neta(u_long source, char* dest, size_t destSize);
u_long			inet_netof(struct in_addr address);
u_long			inet_network(const char* addressString);
char*			inet_ntoa(struct in_addr address);
const char*		inet_ntop(int family, const void* source, char* dest,
					socklen_t destSize);
int				inet_pton(int family, const char* sourceString, void* dest);
u_int			inet_nsap_addr(const char* sourceString, u_char* dest,
					int destSize);
char*			inet_nsap_ntoa(int sourceLength, const u_char* source,
					char* dest);

#ifdef __cplusplus
}
#endif

#endif	/* _INET_H_ */
