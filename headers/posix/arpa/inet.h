/* arpa/inet.h */

/* public definitions of inet functions... */

#ifndef _INET_H_
#define _INET_H_

#include <sys/param.h>
#include <sys/types.h>

in_addr_t        inet_addr (const char *);
int              inet_aton (const char *, struct in_addr *);
in_addr_t        inet_lnaof (struct in_addr);
struct in_addr   inet_makeaddr (in_addr_t , in_addr_t);
char *           inet_neta (in_addr_t, char *, size_t);
in_addr_t        inet_netof (struct in_addr);
in_addr_t        inet_network (const char *);
char            *inet_net_ntop (int, const void *, int, char *, size_t);
int              inet_net_pton (int, const char *, void *, size_t);
char            *inet_ntoa (struct in_addr);
int              inet_pton (int, const char *, void *);
const char      *inet_ntop (int, const void *, char *, size_t);
u_int            inet_nsap_addr (const char *, u_char *, int);
char            *inet_nsap_ntoa (int, const u_char *, char *);

#endif /* _INET_H */
