/* if.h
 * Interface definitions for beos
 */

#ifndef OBOS_IF_DL_H
#define OBOS_IF_DL_H

#include "net/if.h"

/* link level sockaddr structure */
struct sockaddr_dl {
	uint8	sdl_len;      /* Total length of sockaddr */
	uint8	sdl_family;   /* AF_LINK */
	uint16	sdl_index;    /* if != 0, system given index for interface */
	uint8	sdl_type;     /* interface type */
	uint8	sdl_nlen;     /* interface name length, no trailing 0 reqd. */
	uint8	sdl_alen;     /* link level address length */
	uint8	sdl_slen;     /* link layer selector length */
	char	sdl_data[12]; /* minimum work area, can be larger;
                                   contains both if name and ll address */
};

/* Macro to get a pointer to the link level address */
#define LLADDR(s)	((caddr_t)((s)->sdl_data + (s)->sdl_nlen))

void    link_addr (const char *, struct sockaddr_dl *);
char    *link_ntoa (const struct sockaddr_dl *);

#endif /* OBOS_IF_DL_H */

