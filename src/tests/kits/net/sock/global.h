/* -*- c-basic-offset: 8; -*- */
#ifndef	GLOBAL_H
#define	GLOBAL_H

#include "config.h"   /* configuration options for current OS */

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <errno.h>
#include <sys/socket.h>

#ifndef	HAVE_ADDRINFO_STRUCT
#include "addrinfo.h"
#endif

/* Older resolvers do not have gethostbyname2() */
#ifndef	HAVE_GETHOSTBYNAME2
#define	gethostbyname2(host,family)		gethostbyname((host))
#endif

/* This avoids a warning with glibc compilation */
#ifndef errno
extern int errno;
#endif


/* Miscellaneous constants */
#define	MAXLINE	     4096	/* max text line length */
#define	MAXSOCKADDR  128	/* max socket address structure size */
#define	BUFFSIZE     8192	/* buffer size for reads and writes */

/* stdin and stdout file descriptors */
#define STDIN_FILENO  0
#define STDOUT_FILENO 1

#define	min(a,b)	((a) < (b) ? (a) : (b))
#define	max(a,b)	((a) > (b) ? (a) : (b))


#endif
