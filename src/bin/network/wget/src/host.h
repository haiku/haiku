/* Declarations for host.c
   Copyright (C) 1995, 1996, 1997, 2001 Free Software Foundation, Inc.

This file is part of GNU Wget.

GNU Wget is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

GNU Wget is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wget; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

In addition, as a special exception, the Free Software Foundation
gives permission to link the code of its release of Wget with the
OpenSSL project's "OpenSSL" library (or with modified versions of it
that use the same license as the "OpenSSL" library), and distribute
the linked executables.  You must obey the GNU General Public License
in all respects for all of the code used other than "OpenSSL".  If you
modify this file, you may extend this exception to your version of the
file, but you are not obligated to do so.  If you do not wish to do
so, delete this exception statement from your version.  */

#ifndef HOST_H
#define HOST_H

#ifdef WINDOWS
# include <winsock.h>
#else
# include <netdb.h>
# include <sys/socket.h>
# include <netinet/in.h>
#ifndef __BEOS__
# include <arpa/inet.h>
#endif
#endif

struct url;
struct address_list;

/* This struct defines an IP address, tagged with family type.  */

typedef struct {
  /* Address type. */
  enum { 
    IPV4_ADDRESS
#ifdef ENABLE_IPV6
    , IPV6_ADDRESS 
#endif /* ENABLE_IPV6 */
  } type;

  /* Address data union: ipv6 contains IPv6-related data (address and
     scope), and ipv4 contains the IPv4 address.  */
  union {
#ifdef ENABLE_IPV6
    struct {
      struct in6_addr addr;
# ifdef HAVE_SOCKADDR_IN6_SCOPE_ID
      unsigned int scope_id;
# endif
    } ipv6;
#endif /* ENABLE_IPV6 */
    struct {
      struct in_addr addr;
    } ipv4;
  } u;
} ip_address;

/* Because C doesn't support anonymous unions, access to ip_address
   elements is unwieldy.  Hence the accessors.

   The _ADDR accessors return the address as the struct in_addr or
   in6_addr.  The _DATA accessor returns a pointer to the address data
   -- pretty much the same as the above, but cast to void*.  The
   _SCOPE accessor returns the address's scope_id, and makes sense
   only when IPv6 and HAVE_SOCKADDR_IN6_SCOPE_ID are both defined.  */

#define ADDRESS_IPV4_IN_ADDR(x) ((x)->u.ipv4.addr)
/* Don't use &x->u.ipv4.addr.s_addr because it can be #defined to a
   bitfield, which you can't take an address of.  */
#define ADDRESS_IPV4_DATA(x) ((void *)&(x)->u.ipv4.addr)

#define ADDRESS_IPV6_IN6_ADDR(x) ((x)->u.ipv6.addr)
#define ADDRESS_IPV6_DATA(x) ((void *)&(x)->u.ipv6.addr)
#define ADDRESS_IPV6_SCOPE(x) ((x)->u.ipv6.scope_id)

enum {
  LH_SILENT  = 1,
  LH_BIND    = 2,
  LH_REFRESH = 4
};
struct address_list *lookup_host PARAMS ((const char *, int));

void address_list_get_bounds PARAMS ((const struct address_list *,
				      int *, int *));
const ip_address *address_list_address_at PARAMS ((const struct address_list *,
						   int));
int address_list_contains PARAMS ((const struct address_list *, const ip_address *));
void address_list_set_faulty PARAMS ((struct address_list *, int));
void address_list_set_connected PARAMS ((struct address_list *));
int address_list_connected_p PARAMS ((const struct address_list *));
void address_list_release PARAMS ((struct address_list *));

const char *pretty_print_address PARAMS ((const ip_address *));
#ifdef ENABLE_IPV6
int is_valid_ipv6_address PARAMS ((const char *, const char *));
#endif

int accept_domain PARAMS ((struct url *));
int sufmatch PARAMS ((const char **, const char *));

void host_cleanup PARAMS ((void));

#endif /* HOST_H */
