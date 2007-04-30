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
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>		/* getopt_long */
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define RECV_BUF_SIZE	64 * 1024

/* SSM and protocol-independent API from RFC3678 */
#ifndef IP_ADD_SOURCE_MEMBERSHIP
# define IP_ADD_SOURCE_MEMBERSHIP 39
struct ip_mreq_source {
    struct in_addr imr_multiaddr;	/* multicast group */
    struct in_addr imr_sourceaddr;	/* source */
    struct in_addr imr_interface;	/* local IP address of the interface */
};
#endif /* IP_ADD_SOURCE_MEMBERSHIP */

#ifndef MCAST_JOIN_GROUP
# define MCAST_JOIN_GROUP 42
# define MCAST_JOIN_SOURCE_GROUP 46
struct group_req {
    uint32_t                gr_interface;	/* interface index */
    struct sockaddr_storage gr_group;		/* group address */
};
struct group_source_req {
    uint32_t                gsr_interface;	/* interface index */
    struct sockaddr_storage gsr_group;		/* group address */
    struct sockaddr_storage gsr_source;		/* source address */
};
#endif


#define v_printf(level, x...) \
    if (verbose >= level) \
	fprintf(stderr, x)

/* prototypes*/
//void chat(const int sock, const struct sockaddr_in6 saddr);
void chat(const int sock);
void dump(const int sock, int out_fd, const int dump_packets);
static int open_multicast_socket(const char *group, const char *port,
	const char *source);
static int open_packet_file(void *buf, int n, const char *addr);
static void alarm_exit(int signaln);
static void usage(void);
static void err_quit(const char *, ...);
static void err_sys(const char *, ...);

/* global variables */
static struct option longopts[] = {
    { "help",		no_argument,		NULL, 'h' },
    { "verbose",	no_argument,		NULL, 'v' },
    { "timeout",	required_argument,	NULL, 't' },
    { "output",		required_argument,	NULL, 'o' },
    { "source",		required_argument,	NULL, 's' },
    { NULL,		0,			NULL, 0   }
};

static int verbose = 0;

int main(int argc, char *argv[])
{
    int timeout = 0;
    int out_fd = STDOUT_FILENO;
    int dump_packets = 0;
    int ch, sock;
    char *source = NULL;

    while ((ch = getopt_long(argc, argv, "hvo:pt:s:", longopts, 0)) > 0) {
	switch (ch) {
	case 'o':
	    out_fd = open(optarg, O_CREAT | O_TRUNC | O_WRONLY, 0666);
	    if (out_fd < 0)
		err_sys("open");
	    break;
	case 'p':
	    dump_packets = 1;
	    break;
	case 't':
	    timeout = atoi(optarg);
	    break;
	case 's':
	    source = optarg;
	    break;
	case 'v':
	    verbose++;
	    break;
	default:
	    usage();
	}
    }
    argc -= optind;
    argv += optind;

    if (argc != 2)
	usage();

    sock = open_multicast_socket(argv[0], argv[1], source);

    if (timeout) {
	signal(SIGALRM, alarm_exit);
	alarm(timeout);
    }

    chat(sock);
    dump(sock, out_fd, dump_packets);
    exit(0);
}

//void chat(const int sock, const struct sockaddr_in6 saddr)
void chat(const int sock)
{
    fd_set in;

    while (1) {
	int res;

	FD_ZERO(&in);

	FD_SET(0, &in);
	FD_SET(sock, &in);

	res = select(sock + 1, &in, 0, 0, 0);
	if (res < 0)
	    break;

	if (FD_ISSET(0, &in)) {
	    char buffer[8192];
	    int red = read(0, buffer, sizeof(buffer) - 1);
	    buffer[red] = 0;
#if 0
	    if (sendto(sock, buffer, red + 1, 0, (struct sockaddr *) &saddr,
			sizeof(saddr)) < 0)
#else
	    if (send(sock, buffer, red + 1, 0) < 0)
#endif
		err_sys("send");
	}
	if (FD_ISSET(sock, &in)) {
	    struct sockaddr_in6 from;
	    socklen_t fromlen = sizeof(from);
	    char taddr[64];
	    char buffer[8192];

	    int red = recvfrom(sock, buffer, sizeof(buffer), 0,
		    (struct sockaddr *) &from, &fromlen);
	    buffer[red] = 0;

	    inet_ntop(AF_INET6, &from.sin6_addr, taddr, sizeof(taddr));

	    printf("<%s> %s\n", taddr, buffer);
	}
    }
}

void dump(const int sock, int out_fd, const int dump_packets) {
    int n;
    struct sockaddr_storage sraddr;
    struct sockaddr *raddr = (struct sockaddr *)&sraddr;
    socklen_t rlen = sizeof(struct sockaddr_storage);
    char buf[RECV_BUF_SIZE];

    while ((n = recvfrom(sock, buf, sizeof(buf), 0, raddr, &rlen)) > 0) {
	char address[NI_MAXHOST], port[NI_MAXSERV];

	if (verbose >= 2) {
	    if (getnameinfo(raddr, rlen, address, sizeof(address), port,
			sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV) < 0)
		err_sys("getnameinfo");
	    printf("Received %d bytes from [%s]:%s.\n", n, address, port);
	}
	if (dump_packets)
	    out_fd = open_packet_file(buf, n, address);
	if (write(out_fd, buf, n) != n)
	    err_sys("write");
	if (dump_packets)
	    if (close(out_fd) < 0)
		err_sys("close");
    }
    if (n < 0)
	err_sys("recvfrom");

    if (!dump_packets && out_fd != STDOUT_FILENO)
	if (close(out_fd) < 0)
	    err_sys("close");

    v_printf(1, "End of stream.\n");
}

static int open_multicast_socket(const char *group, const char *port,
	const char *source)
{
    int fd;
    int yes = 1;
#ifdef AF_INET6
    int err;
    struct addrinfo hints, *res, *ai;
    int level;
    struct group_req gr;
    struct group_source_req gsr;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((err = getaddrinfo(group, port, &hints, &res)) != 0)
	err_quit("getaddrinfo(%s, %s): %s", source, port, gai_strerror(err));
    for (ai = res; ai; ai = ai->ai_next) {
	if ((fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) < 0)
	    continue;		/* ignore */
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
	    err_sys("setsockopt(SO_REUSEADDR)");
	if (bind(fd, ai->ai_addr, ai->ai_addrlen) == 0)
	    break;		/* success */
	close(fd);
    }

    if (connect(fd, ai->ai_addr, ai->ai_addrlen) < 0)
	err_sys("connect");

    if (!ai)
	err_sys("bind");

    switch (ai->ai_family) {
    case AF_INET:
	level = IPPROTO_IP;
	break;
    case AF_INET6:
	level = IPPROTO_IPV6;
	break;
    default:
	err_quit("FATAL: family %d is not known", ai->ai_family);
    }

    if (source) {
	struct addrinfo shints, *sai;

	memset(&shints, 0, sizeof(shints));
	shints.ai_family = ai->ai_family;
	shints.ai_protocol = ai->ai_protocol;

	if ((err = getaddrinfo(source, port, &shints, &sai)) != 0)
	    err_quit("getaddrinfo(%s, %s): %s", source, port,
		    gai_strerror(err));

	memcpy(&gsr.gsr_group, ai->ai_addr, ai->ai_addrlen);
	memcpy(&gsr.gsr_source, sai->ai_addr, sai->ai_addrlen);
	gsr.gsr_interface = 0;
	if (setsockopt(fd, level, MCAST_JOIN_SOURCE_GROUP, &gsr,
		    sizeof(gsr)) < 0)
	    err_sys("setsockopt(MCAST_JOIN_SOURCE_GROUP)");
	freeaddrinfo(sai);
    } else {
	memcpy(&gr.gr_group, ai->ai_addr, ai->ai_addrlen);
	gr.gr_interface = 0;
	if (setsockopt(fd, level, MCAST_JOIN_GROUP, &gr, sizeof(gr)) < 0)
	    err_sys("setsockopt(MCAST_JOIN_GROUP)");
    }

    freeaddrinfo(res);
#else
    struct hostent *hostinfo;
    struct sockaddr_in saddr;
    struct ip_mreq mr;
    struct ip_mreq_source mrs;

    if ((hostinfo = gethostbyname(group)) == NULL)
	err_quit("Host %s not found.", group);

    if (!IN_MULTICAST(ntohl(*hostinfo->h_addr)))
	err_quit("%s is not a multicast address", group);

    if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0)
	err_sys("socket");

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
	err_sys("setsockopt(SO_REUSEADDR)");

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr = *(struct in_addr *) hostinfo->h_addr;
    saddr.sin_port = htons(atoi(port));
    if (bind(fd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
	err_sys("bind");

    if (source) {
	mrs.imr_multiaddr = saddr.sin_addr;
	mrs.imr_sourceaddr.s_addr = inet_addr(source);
	mrs.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(fd, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP, &mrs,
		    sizeof(mrs)) < 0)
	    err_sys("setsockopt(IP_ADD_SOURCE_MEMBERSHIP)");
    } else {
	mr.imr_multiaddr = saddr.sin_addr;
	mr.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mr, sizeof(mr))<0)
	    err_sys("setsockopt(IP_ADD_MEMBERSHIP)");
    }
#endif

    return fd;
}

static int open_packet_file(void *buf, int n, const char *address)
{
    static unsigned long int counter = 0;
    char filename[1024];
    int fd;

    snprintf(filename, sizeof(filename), "pkt-%lu-%s", counter++, address);
    fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    if (fd < 0)
	err_sys("open");
    return fd;
}

static void alarm_exit(int signaln)
{
    v_printf(1, "Timeout.\n");
    exit(0);
}

static void usage(void)
{
    fprintf(stderr,
"Usage: multicat [OPTIONS...] GROUP PORT\n"
"\n"
"  -o, --output=FILE  write the stream to FILE\n"
"  -p                 write every packet to a new file\n"
"  -t, --timeout=NUM  the program will exit after NUM seconds\n"
"  -s, --source=ADDR  join the SSM source group ADDR\n"
"  -v, --verbose      tell me more. Use multiple times for more details\n"
"  -h, --help         display this help and exit\n"
"\n"
"If --output is not specified, standard output is used.\n"
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

