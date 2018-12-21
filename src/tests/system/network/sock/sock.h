/* -*- c-basic-offset: 8; -*-
 *
 * Copyright (c) 1993 W. Richard Stevens.  All rights reserved.
 * Permission to use or modify this software and its documentation only for
 * educational purposes and without fee is hereby granted, provided that
 * the above copyright notice appear in all copies.  The author makes no
 * representations about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 */


#include "config.h"   /* configuration options for current OS */

#include <stdio.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <errno.h>
#include <sys/param.h>
#include <sys/socket.h>

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifndef	HAVE_ADDRINFO_STRUCT
#include "addrinfo.h"
#endif

#include <sys/uio.h>
#ifdef	__bsdi__
#include <machine/endian.h> /* required before tcp.h, for BYTE_ORDER */
#endif
#include <netinet/tcp.h>	   /* TCP_NODELAY */
#include <netdb.h>	   /* getservbyname(), gethostbyname() */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>



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

/* declare global variables */
extern int		bindport;
extern int		broadcast;
extern int		cbreak;
extern int		chunkwrite;
extern int		client;
extern int		connectudp;
extern int		crlf;
extern int		debug;
extern int		dofork;
extern int		dontroute;
extern char		foreignip[];
extern int		foreignport;
extern int		halfclose;
extern int		ignorewerr;
extern int		iptos;
extern int		ipttl;
extern char		joinip[];
extern int		keepalive;
extern long		linger;
extern int		listenq;
extern char		localip[];
extern int		maxseg;
extern int		mcastttl;
extern int		msgpeek;
extern int		nodelay;
extern int		nbuf;
extern int		onesbcast;
extern int		pauseclose;
extern int		pauseinit;
extern int		pauselisten;
extern int		pauserw;
extern int		reuseaddr;
extern int		reuseport;
extern int		readlen;
extern int		writelen;
extern int		recvdstaddr;
extern int		rcvbuflen;
extern int		sndbuflen;
extern long		rcvtimeo;
extern long		sndtimeo;
extern char	       *rbuf;
extern char	       *wbuf;
extern int		server;
extern int		sigio;
extern int		sourcesink;
extern int		sroute_cnt;
extern int		udp;
extern int		urgwrite;
extern int		verbose;
extern int		usewritev;

extern struct sockaddr_in	cliaddr, servaddr;

/* Earlier versions of gcc under SunOS 4.x have problems passing arguments
   that are structs (as opposed to pointers to structs).  This shows up
   with inet_ntoa, whose argument is a "struct in_addr". */

#if	defined(sun) && defined(__GNUC__) && defined(GCC_STRUCT_PROBLEM)
#define	INET_NTOA(foo)	inet_ntoa(&foo)
#else
#define	INET_NTOA(foo)	inet_ntoa(foo)
#endif

				/* function prototypes */
void	buffers(int);
int     cliopen(char *, char *);
int	crlf_add(char *, int, const char *, int);
int	crlf_strip(char *, int, const char *, int);
void	join_mcast(int, struct sockaddr_in *);
void	loop_tcp(int);
void	loop_udp(int);
void	pattern(char *, int);
int		servopen(char *, char *);
void	sink_tcp(int);
void	sink_udp(int);
void	source_tcp(int);
void	source_udp(int);
void	sroute_doopt(int, char *);
void	sroute_set(int);
void	sleep_us(unsigned int);
void	sockopts(int, int);
ssize_t	dowrite(int, const void *, size_t);

void	TELL_WAIT(void);
void	TELL_PARENT(pid_t);
void	WAIT_PARENT(void);
void	TELL_CHILD(pid_t);
void	WAIT_CHILD(void);

void	 err_dump(const char *, ...);
void	 err_msg(const char *, ...);
void	 err_quit(const char *, ...);
void	 err_ret(const char *, ...);
void	 err_sys(const char *, ...);

ssize_t	 writen(int, const void *, size_t);
