/*
 * Copyright 2003-2004 by Marco d'Itri <md@linux.it>
 * This software is distributed under the terms of the GNU GPL. If we meet some
 * day, and you think this stuff is worth it, you can buy me a beer in return.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>		/* strerror */
#include <errno.h>
#include <string.h>
#include <getopt.h>		/* getopt_long */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/utsname.h>

int open_multicast_socket(const char *group, const char *port,
	const char *source, const int ttl);
static void err_sys(const char *fmt, ...);
static void err_quit(const char *fmt, ...);
static void usage(void);

/* global variables */
static struct option longopts[] = {
    { "help",		no_argument,		NULL, 'h' },
    { "input",		no_argument,		NULL, 'i' },
    { "size",		required_argument,	NULL, 's' },
    { "bind",		required_argument,	NULL, 'b' },
    { "ttl",		required_argument,	NULL, 't' },
    { NULL,		0,			NULL, 0   }
};

int main(int argc, char *argv[])
{
    int fd, len, ch;
    int opt_input = 0;
    int ttl = 128;
    int sndbuf_size = 1280;
    char *source = NULL;

    while ((ch = getopt_long(argc, argv, "his:b:t:", longopts, 0)) > 0) {
	switch (ch) {
	case 's':
	    sndbuf_size = atoi(optarg);
	    break;
	case 'i':
	    opt_input = 1;
	    break;
	case 'b':
	    source = optarg;
	    break;
	case 't':
	    ttl = atoi(optarg);
	    break;
	default:
	    usage();
	}
    }
    argc -= optind;
    argv += optind;

    if (argc != 2)
	usage();

    fd = open_multicast_socket(argv[0], argv[1], source, ttl);

    if (opt_input) {
	char buf[sndbuf_size];

	while ((len = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
	    if (send(fd, buf, len, 0) < 0)
		err_sys("send");
	}
    } else {
	struct utsname utsname;
	char buf[1024];

	uname(&utsname);
	snprintf(buf, sizeof(buf), "Hello from %s!\n", utsname.nodename);
	len = strlen(buf);

	while (1) {
	    if (send(fd, buf, len, 0) < 0)
		err_sys("send");

	    sleep(1);
	}
    }

    exit(0);
}

int open_multicast_socket(const char *group, const char *port,
	const char *source, const int ttl)
{
    int fd;
    int loop = 1;

#ifdef AF_INET6
    int err;
    struct addrinfo hints, *res, *ai;
    int level, sockopt_h, sockopt_l;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((err = getaddrinfo(group, port, &hints, &res)) != 0)
	err_quit("getaddrinfo(%s, %s): %s", source, port, gai_strerror(err));
    for (ai = res; ai; ai = ai->ai_next) {
	if ((fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) < 0)
	    continue;		/* ignore */
	if (connect(fd, ai->ai_addr, ai->ai_addrlen) == 0)
	    break;		/* success */
	close(fd);
    }

    if (!ai)
	err_sys("connect");

    switch (ai->ai_family) {
    case AF_INET:
	level = IPPROTO_IP;
	sockopt_h = IP_MULTICAST_TTL;
	sockopt_l = IP_MULTICAST_LOOP;
	break;
    case AF_INET6:
	level = IPPROTO_IPV6;
	sockopt_h = IPV6_MULTICAST_HOPS;
	sockopt_l = IPV6_MULTICAST_LOOP;
	break;
    default:
	err_quit("FATAL: family %d is not known", ai->ai_family);
    }

    if (source) {
	struct addrinfo shints, *sai;
	int yes = 1;

	memset(&shints, 0, sizeof(shints));
	shints.ai_family = ai->ai_family;
	shints.ai_protocol = ai->ai_protocol;

	if ((err = getaddrinfo(source, port, &shints, &sai)) != 0)
	    err_quit("getaddrinfo(%s, %s): %s", source, port,
		    gai_strerror(err));

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
	    err_sys("setsockopt(SO_REUSEADDR)");

	if (bind(fd, sai->ai_addr, sai->ai_addrlen) < 0)
	    err_sys("bind");

	freeaddrinfo(sai);
	/*
	struct ip_addr ifaddr = inet_addr();
	setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, &ifaddr, sizeof(struct in_addr));
	setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifidx, sizeof(unsigned int));
	man 7 netdevice
	*/
    }

    freeaddrinfo(res);

#else
    struct sockaddr_in addr;
    struct hostent *hostinfo;
    const int level = IPPROTO_IP;
    const int sockopt_h = IP_MULTICAST_TTL;
    const int sockopt_l = IP_MULTICAST_LOOP;

    if ((hostinfo = gethostbyname(group)) == NULL)
	err_quit("Host %s not found.", group);

    /* create what looks like an ordinary UDP socket */
    if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0)
	err_sys("socket");

    /* set up destination address */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr = *(struct in_addr *) hostinfo->h_addr;
    addr.sin_port = htons(atoi(port));

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	err_sys("connect");

    if (source) { /* XXX */
	int yes = 1;

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
	    err_sys("setsockopt(SO_REUSEADDR)");
    }
#endif

    if (setsockopt(fd, level, sockopt_h, &ttl, sizeof(ttl)) < 0)
	err_sys("setsockopt(IP_MULTICAST_TTL)");

    if (loop == 0)
	if (setsockopt(fd, level, sockopt_l, &loop, sizeof(loop)) < 0)
	    err_sys("setsockopt(IP_MULTICAST_LOOP)");

    return fd;
}


static void usage(void)
{
    fprintf(stderr,
"Usage: multisend [OPTIONS...] GROUP PORT\n"
"\n"
"  -b, --bind=ADDR    bind the socket to address ADDR\n"
"  -t, --ttl=NUM      set the TTL to NUM\n"
"  -h, --help         display this help and exit\n"
"\n"
"If --ttl is not specified, 128 is used.\n"
);
    exit(0);
}

/* Error routines */
static void err_sys(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, ": %s\n", strerror(errno));
    va_end(ap);
    exit(1);
}

static void err_quit(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fputs("\n", stderr);
    va_end(ap);
    exit(1);
}

#if 0
/* local network */
#define HELLO_GROUP "224.0.0.3"
/* ? */
#define HELLO_GROUP "225.0.0.37"
/* GLOP */
#define HELLO_GROUP "233.49.235.42"
/* fw */
#define HELLO_GROUP "239.113.42.66"
/* Organization-Local */
#define HELLO_GROUP "239.192.42.66"
/* Site-Local */
#define HELLO_GROUP "239.252.42.66"
/* Source Specific Multicast */
#define HELLO_GROUP "232.1.1.1"
/* node local */
#define HELLO_ADDR "FF01::1111"
#endif
