/* arpa/inet.h */

/* public definitions of inet functions... */

#ifndef _INET_H_
#define _INET_H_

#include <sys/param.h>
#include <sys/types.h>
#include <netinet/in.h>


/* R5 libnet function names conflict with libbind. BONE uses a prefix to solve this. */
#ifndef BUILDING_R5_LIBNET

#define	inet_addr		__inet_addr
#define	inet_aton		__inet_aton
#define	inet_lnaof		__inet_lnaof
#define	inet_makeaddr	__inet_makeaddr
#define	inet_neta		__inet_neta
#define	inet_netof		__inet_netof
#define	inet_network	__inet_network
#define	inet_net_ntop	__inet_net_ntop
#define	inet_net_pton	__inet_net_pton
#define	inet_ntoa		__inet_ntoa
#define	inet_pton		__inet_pton
#define	inet_ntop		__inet_ntop
#define	inet_nsap_addr	__inet_nsap_addr
#define	inet_nsap_ntoa	__inet_nsap_ntoa

#endif /* BUILDING_R5_LIBNET */


#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* _INET_H */
