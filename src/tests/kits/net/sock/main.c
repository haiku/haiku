/* -*- c-basic-offset: 8; -*-
 *
 * Copyright (c) 1993 W. Richard Stevens.  All rights reserved.
 * Permission to use or modify this software and its documentation only for
 * educational purposes and without fee is hereby granted, provided that
 * the above copyright notice appear in all copies.  The author makes no
 * representations about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 */

#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <string.h>
#include	"sock.h"

char	*host;		/* hostname or dotted-decimal string */
char	*port;

			/* DefinE global variables */
int		bindport;			/* 0 or TCP or UDP port number to bind */
							/* set by -b or -l options */
int		broadcast;			/* SO_BROADCAST */
int		cbreak;				/* set terminal to cbreak mode */
int		chunkwrite;			/* write in small chunks; not all-at-once */
int		client = 1;			/* acting as client is the default */
int		connectudp = 1;			/* connect UDP client */
int		crlf;				/* convert newline to CR/LF & vice versa */
int		debug;				/* SO_DEBUG */
int		dofork;				/* concurrent server, do a fork() */
int		dontroute;			/* SO_DONTROUTE */
char		foreignip[32];			/* foreign IP address, dotted-decimal string */
int		foreignport;			/* foreign port number */
int		halfclose;			/* TCP half close option */
int		ignorewerr;			/* true if write() errors should be ignored */
int		iptos = -1;			/* IP_TOS opton */
int		ipttl = -1;			/* IP_TTL opton */
char		joinip[32];			/* multicast IP address, dotted-decimal string */
int		keepalive;			/* SO_KEEPALIVE */
long		linger = -1;			/* 0 or positive turns on option */
int		listenq = 5;			/* listen queue for TCP Server */
char		localip[32];			/* local IP address, dotted-decimal string */
int		maxseg;				/* TCP_MAXSEG */
int		mcastttl;			/* multicast TTL */
int		msgpeek;			/* MSG_PEEK */
int		nodelay;			/* TCP_NODELAY (Nagle algorithm) */
int		nbuf = 1024;			/* number of buffers to write (sink mode) */
int		onesbcast;			/* set IP_ONESBCAST for 255.255.255.255 bcasts */
int		pauseclose;			/* #ms to sleep after recv FIN, before close */
int		pauseinit;			/* #ms to sleep before first read */
int		pauselisten;			/* #ms to sleep after listen() */
int		pauserw;			/* #ms to sleep before each read or write */
int		reuseaddr;			/* SO_REUSEADDR */
int		reuseport;			/* SO_REUSEPORT */
int		readlen = 1024;			/* default read length for socket */
int		writelen = 1024;		/* default write length for socket */
int		recvdstaddr;			/* IP_RECVDSTADDR option */
int		rcvbuflen;			/* size for SO_RCVBUF */
int		sndbuflen;			/* size for SO_SNDBUF */
long		rcvtimeo;			/* SO_RCVTIMEO */
long		sndtimeo;			/* SO_SNDTIMEO */
int		sroute_cnt;			/* count of #IP addresses in route */
char   		*rbuf;				/* pointer that is malloc'ed */
char   		*wbuf;				/* pointer that is malloc'ed */
int		server;				/* to act as server requires -s option */
int		sigio;				/* send SIGIO */
int		sourcesink;			/* source/sink mode */
int		udp;				/* use UDP instead of TCP */
int		urgwrite;			/* write urgent byte after this write */
int		verbose;			/* each -v increments this by 1 */
int		usewritev;			/* use writev() instead of write() */

struct sockaddr_in	cliaddr, servaddr;

static void	usage(const char *);

int
main(int argc, char *argv[])
{
	int	c, fd;
	char	*ptr;

	if (argc < 2)
		usage("");

	opterr = 0;		/* don't want getopt() writing to stderr */
	while ( (c = getopt(argc, argv, "2b:cf:g:hij:kl:n:op:q:r:st:uvw:x:y:ABCDEFG:H:IJ:KL:NO:P:Q:R:S:TU:VWX:YZ")) != -1) {
		switch (c) {
#ifdef	IP_ONESBCAST
		case '2':			/* use 255.255.255.255 as broadcast address */
			onesbcast = 1;
			break;
#endif

		case 'b':
			bindport = atoi(optarg);
			break;

		case 'c':			/* convert newline to CR/LF & vice versa */
			crlf = 1;
			break;

		case 'f':			/* foreign IP address and port#: a.b.c.d.p */
			if ( (ptr = strrchr(optarg, '.')) == NULL)
				usage("invalid -f option");

			*ptr++ = 0;					/* null replaces final period */
			foreignport = atoi(ptr);	/* port number */
			strcpy(foreignip, optarg);	/* save dotted-decimal IP */
			break;

		case 'g':			/* loose source route */
			sroute_doopt(0, optarg);
			break;

		case 'h':			/* TCP half-close option */
			halfclose = 1;
			break;

		case 'i':			/* source/sink option */
			sourcesink = 1;
			break;

#ifdef	IP_ADD_MEMBERSHIP
		case 'j':			/* join multicast group a.b.c.d */
			strcpy(joinip, optarg);	/* save dotted-decimal IP */
			break;
#endif

		case 'k':			/* chunk-write option */
			chunkwrite = 1;
			break;

		case 'l':			/* local IP address and port#: a.b.c.d.p */
			if ( (ptr = strrchr(optarg, '.')) == NULL)
				usage("invalid -l option");

			*ptr++ = 0;					/* null replaces final period */
			bindport = atoi(ptr);		/* port number */
			strcpy(localip, optarg);	/* save dotted-decimal IP */
			break;

		case 'n':			/* number of buffers to write */
			nbuf = atol(optarg);
			break;

		case 'o':			/* do not connect UDP client */
			connectudp = 0;
			break;

		case 'p':			/* pause before each read or write */
			pauserw = atoi(optarg);
			break;

		case 'q':			/* listen queue for TCP server */
			listenq = atoi(optarg);
			break;

		case 'r':			/* read() length */
			readlen = atoi(optarg);
			break;

		case 's':			/* server */
			server = 1;
			client = 0;
			break;

#ifdef	IP_MULTICAST_TTL
		case 't':			/* IP_MULTICAST_TTL */
			mcastttl = atoi(optarg);
			break;
#endif

		case 'u':			/* use UDP instead of TCP */
			udp = 1;
			break;

		case 'v':			/* output what's going on */
			verbose++;
			break;

		case 'w':			/* write() length */
			writelen = atoi(optarg);
			break;

		case 'x':			/* SO_RCVTIMEO socket option */
			rcvtimeo = atol(optarg);
			break;

		case 'y':			/* SO_SNDTIMEO socket option */
			sndtimeo = atol(optarg);
			break;

		case 'A':			/* SO_REUSEADDR socket option */
			reuseaddr = 1;
			break;

		case 'B':			/* SO_BROADCAST socket option */
			broadcast = 1;
			break;

		case 'C':			/* set standard input to cbreak mode */
			cbreak = 1;
			break;

		case 'D':			/* SO_DEBUG socket option */
			debug = 1;
			break;

		case 'E':			/* IP_RECVDSTADDR socket option */
			recvdstaddr = 1;
			break;

		case 'F':			/* concurrent server, do a fork() */
			dofork = 1;
			break;

		case 'G':			/* strict source route */
			sroute_doopt(1, optarg);
			break;

#ifdef	IP_TOS
		case 'H':			/* IP_TOS socket option */
			iptos = atoi(optarg);
			break;
#endif

		case 'I':			/* SIGIO signal */
			sigio = 1;
			break;

#ifdef	IP_TTL
		case 'J':			/* IP_TTL socket option */
			ipttl = atoi(optarg);
			break;
#endif

		case 'K':			/* SO_KEEPALIVE socket option */
			keepalive = 1;
			break;

		case 'L':			/* SO_LINGER socket option */
			linger = atol(optarg);
			break;

		case 'N':			/* SO_NODELAY socket option */
			nodelay = 1;
			break;

		case 'O':			/* pause before listen(), before first accept() */
			pauselisten = atoi(optarg);
			break;

		case 'P':			/* pause before first read() */
			pauseinit = atoi(optarg);
			break;

		case 'Q':			/* pause after receiving FIN, but before close() */
			pauseclose = atoi(optarg);
			break;

		case 'R':			/* SO_RCVBUF socket option */
			rcvbuflen = atoi(optarg);
			break;

		case 'S':			/* SO_SNDBUF socket option */
			sndbuflen = atoi(optarg);
			break;

#ifdef	SO_REUSEPORT
		case 'T':			/* SO_REUSEPORT socket option */
			reuseport = 1;
			break;
#endif

		case 'U':			/* when to write urgent byte */
			urgwrite = atoi(optarg);
			break;

		case 'V':			/* use writev() instead of write() */
			usewritev = 1;
			chunkwrite = 1;	/* implies this option too */
			break;

		case 'W':			/* ignore write errors */
			ignorewerr = 1;
			break;

		case 'X':			/* TCP maximum segment size option */
			maxseg = atoi(optarg);
			break;

		case 'Y':			/* SO_DONTROUTE socket option */
			dontroute = 1;
			break;

		case 'Z':			/* MSG_PEEK option */
			msgpeek = MSG_PEEK;
			break;

		case '?':
			usage("unrecognized option");
		}
	}

		/* check for options that don't make sense */
	if (udp && halfclose)
		usage("can't specify -h and -u");
	if (udp && debug)
		usage("can't specify -D and -u");
	if (udp && linger >= 0)
		usage("can't specify -L and -u");
	if (udp && nodelay)
		usage("can't specify -N and -u");
#ifdef	notdef
	if (udp == 0 && broadcast)
		usage("can't specify -B with TCP");
#endif
	if (udp == 0 && foreignip[0] != 0)
		usage("can't specify -f with TCP");

	if (client) {
		if (optind != argc-2)
			usage("missing <hostname> and/or <port>");
		host = argv[optind];
		port = argv[optind+1];

	} else {
			/* If server specifies host and port, then local address is
			   bound to the "host" argument, instead of being wildcarded. */
		if (optind == argc-2) {
			host = argv[optind];
			port = argv[optind+1];
		} else if (optind == argc-1) {
			host = NULL;
			port = argv[optind];
		} else
			usage("missing <port>");
	}

	if (client)
		fd = cliopen(host, port);
	else
		fd = servopen(host, port);

	if (sourcesink) {		/* ignore stdin/stdout */
		if (client) {
			if (udp)
				source_udp(fd);
			else
				source_tcp(fd);
		} else {
			if (udp)
				sink_udp(fd);
			else
				sink_tcp(fd);
		}

	} else {				/* copy stdin/stdout to/from socket */
		if (udp)
			loop_udp(fd);
		else
			loop_tcp(fd);
	}

	exit(0);
}

static void
usage(const char *msg)
{
	err_msg(
"usage: sock [ options ] <host> <port>              (for client; default)\n"
"       sock [ options ] -s [ <IPaddr> ] <port>     (for server)\n"
"       sock [ options ] -i <host> <port>           (for \"source\" client)\n"
"       sock [ options ] -i -s [ <IPaddr> ] <port>  (for \"sink\" server)\n"
"options: -b n  bind n as client's local port number\n"
"         -c    convert newline to CR/LF & vice versa\n"
"         -f a.b.c.d.p  foreign IP address = a.b.c.d, foreign port# = p\n"
"         -g a.b.c.d  loose source route\n"
"         -h    issue TCP half close on standard input EOF\n"
"         -i    \"source\" data to socket, \"sink\" data from socket (w/-s)\n"
#ifdef	IP_ADD_MEMBERSHIP
"         -j a.b.c.d  join multicast group\n"
#endif
"         -k    write or writev in chunks\n"
"         -l a.b.c.d.p  client's local IP address = a.b.c.d, local port# = p\n"
"         -n n  #buffers to write for \"source\" client (default 1024)\n"
"         -o    do NOT connect UDP client\n"
"         -p n  #ms to pause before each read or write (source/sink)\n"
"         -q n  size of listen queue for TCP server (default 5)\n"
"         -r n  #bytes per read() for \"sink\" server (default 1024)\n"
"         -s    operate as server instead of client\n"
#ifdef	IP_MULTICAST_TTL
"         -t n  set multicast ttl\n"
#endif
"         -u    use UDP instead of TCP\n"
"         -v    verbose\n"
"         -w n  #bytes per write() for \"source\" client (default 1024)\n"
"         -x n  #ms for SO_RCVTIMEO (receive timeout)\n"
"         -y n  #ms for SO_SNDTIMEO (send timeout)\n"
"         -A    SO_REUSEADDR option\n"
"         -B    SO_BROADCAST option\n"
"         -C    set terminal to cbreak mode\n"
"         -D    SO_DEBUG option\n"
"         -E    IP_RECVDSTADDR option\n"
"         -F    fork after connection accepted (TCP concurrent server)\n"
"         -G a.b.c.d  strict source route\n"
#ifdef	IP_TOS
"         -H n  IP_TOS option (16=min del, 8=max thru, 4=max rel, 2=min$)\n"
#endif
"         -I    SIGIO signal\n"
#ifdef	IP_TTL
"         -J n  IP_TTL option\n"
#endif
"         -K    SO_KEEPALIVE option\n"
"         -L n  SO_LINGER option, n = linger time\n"
"         -N    TCP_NODELAY option\n"
"         -O n  #ms to pause after listen, but before first accept\n"
"         -P n  #ms to pause before first read or write (source/sink)\n"
"         -Q n  #ms to pause after receiving FIN, but before close\n"
"         -R n  SO_RCVBUF option\n"
"         -S n  SO_SNDBUF option\n"
#ifdef	SO_REUSEPORT
"         -T    SO_REUSEPORT option\n"
#endif
"         -U n  enter urgent mode before write number n (source only)\n"
"         -V    use writev() instead of write(); enables -k too\n"
"         -W    ignore write errors for sink client\n"
"         -X n  TCP_MAXSEG option (set MSS)\n"
"         -Y    SO_DONTROUTE option\n"
"         -Z    MSG_PEEK\n"
#ifdef	IP_ONESBCAST
"         -2    IP_ONESBCAST option (255.255.255.255 for broadcast\n"
#endif
);

	if (msg[0] != 0)
		err_quit("%s", msg);
	exit(1);
}
