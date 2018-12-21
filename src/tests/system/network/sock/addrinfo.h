/* -*- c-basic-offset: 8; -*- */
#ifndef	ADDRINFO_H
#define	ADDRINFO_H

/*
 * Everything here really belongs in <netdb.h>.
 * These defines are separate for now, to avoid having to modify the
 * system's header.
 */

struct addrinfo {
  int		ai_flags;		/* AI_PASSIVE, AI_CANONNAME */
  int		ai_family;		/* PF_xxx */
  int		ai_socktype;		/* SOCK_xxx */
  int		ai_protocol;		/* IPPROTO_xxx for IPv4 and IPv6 */
  size_t	ai_addrlen;		/* length of ai_addr */
  char		*ai_canonname;		/* canonical name for host */
  struct sockaddr	*ai_addr;	/* binary address */
  struct addrinfo	*ai_next;	/* next structure in linked list */
};

/* following for getaddrinfo() */
#define	AI_PASSIVE	 1	/* socket is intended for bind() + listen() */
#define	AI_CANONNAME	 2	/* return canonical name */

/* following for getnameinfo() */
#define	NI_MAXHOST	 1025	/* max hostname returned */
#define	NI_MAXSERV	 32	/* max service name returned */

#define	NI_NOFQDN	 1	/* do not return FQDN */
#define	NI_NUMERICHOST   2	/* return numeric form of hostname */
#define	NI_NAMEREQD	 4	/* return error if hostname not found */
#define	NI_NUMERICSERV   8	/* return numeric form of service name */
#define	NI_DGRAM	 16	/* datagram service for getservbyname() */

/* error returns */
#define	EAI_ADDRFAMILY	 1	/* address family for host not supported */
#define	EAI_AGAIN	 2	/* temporary failure in name resolution */
#define	EAI_BADFLAGS	 3	/* invalid value for ai_flags */
#define	EAI_FAIL	 4	/* non-recoverable fail in name resolution */
#define	EAI_FAMILY	 5	/* ai_family not supported */
#define	EAI_MEMORY	 6	/* memory allocation failure */
#define	EAI_NODATA	 7	/* no address associated with host */
#define	EAI_NONAME	 8	/* host nor service provided, or not known */
#define	EAI_SERVICE	 9	/* service not supported for ai_socktype */
#define	EAI_SOCKTYPE	 10	/* ai_socktype not supported */
#define	EAI_SYSTEM	 11	/* system error returned in errno */

#endif
