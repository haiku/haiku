/*
 * Copyright (c) 1983, 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <kernel/OS.h>

#include "sys/socket.h"
#include "sys/mbuf.h"

#include "net/if.h"
#include "net/route.h"
#include "net/if_dl.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "netdb.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "keywords.h"

int getaddr(int which, char *s, struct hostent **hpp);

union	sockunion {
	struct	sockaddr sa;
	struct	sockaddr_in sin;
#ifdef INET6
	struct	sockaddr_in6 sin6;
#endif
	struct	sockaddr_dl sdl;
} so_dst, so_gate, so_mask, so_genmask, so_ifa, so_ifp;

typedef union sockunion *sup;
pid_t	pid;
int	rtm_addrs, s;
int	forcehost, forcenet, doflush, nflag, af, qflag, tflag, keyword();
int	iflag, verbose, aflen = sizeof (struct sockaddr_in);
int	locking, lockrest, debugonly;
struct	rt_metrics rt_metrics;
u_long  rtm_inits;
uid_t	uid;

char	*routename __P((struct sockaddr *));
char	*netname __P((struct sockaddr *));
void	 flushroutes __P((int, char **));
int	 newroute __P((int, char **));
void	 monitor __P((void));
#ifdef INET6
static int prefixlen __P((char *));
#endif
void	 sockaddr __P((char *, struct sockaddr *));
void	 sodump __P((sup, char *));
void	 print_getmsg __P((struct rt_msghdr *, int));
void	 print_rtmsg __P((struct rt_msghdr *, int));
void	 pmsg_common __P((struct rt_msghdr *));
void	 pmsg_addrs __P((char *, int));
void	 bprintf __P((FILE *, int, u_char *));
void	 mask_addr __P((void));
//int	 getaddr __P((int, char *, struct hostent **));
int	 rtmsg __P((int, int));
int	 x25_makemask __P((void));

extern void show __P((int, char **));	/* XXX - from show.c */


int opterr = 1;
int optind = 1;
int optopt = 0;
int optreset = 0;
char *optarg;

#define BADCH (int)'?'
#define BADARG (int)':'
#define EMSG ""

int _getopt(int nargc, char *const *nargv, const char *ostr)
{
	static char *place = EMSG;
	char *oli;
	
	if (optreset || !*place) {
		optreset = 0;
		if (optind >= nargc || *(place = nargv[optind]) != '-') {
			place = EMSG;
			return -1;
		}
		if (place[1] && *++place=='-') {
			++optind;
			place=EMSG;
			return -1;
		}
	}
	if ((optopt = (int)*place++) == (int)':' || 
	    !(oli = strchr(ostr, optopt))) {
		if (optopt==(int)'-')
			return -1;
		if (!*place)
			++optind;
		if (opterr && *ostr != ':')
			fprintf(stderr, "illegal option --%c\n", optopt);
		return BADCH;
	}
	if (*++oli != ':') {
		optarg = NULL;
		if (!*place)
			++optind;
	} else {
		if (*place)
			optarg = place;
		else if (nargc <= ++optind) {
			place = EMSG;
			if (*ostr==':')
				return BADARG;
			if (opterr)
				printf("option requires an argument -- %c\n", optopt);
			return BADCH;
		} else 
			optarg = nargv[optind];
		place = EMSG;
		++optind;
	}
	return optopt;
}
 	
void usage(char *cp)
{
	printf("route for OpenBeOS!\n");
	if (cp)
		(void) fprintf(stderr, "route: botched keyword: %s\n", cp);
	(void) fprintf(stderr,
	    "usage: route [ -nqv ] cmd [[ -<modifiers> ] args ]\n");
	(void) fprintf(stderr,
	    "keywords: get, add, change, delete, show, flush, monitor.\n");
	exit(1);
	/* NOTREACHED */
}

void quit(char *s)
{
	int sverrno = errno;

	(void) fprintf(stderr, "route: ");
	if (s)
		(void) fprintf(stderr, "%s: ", s);
	(void) fprintf(stderr, "%s\n", strerror(sverrno));
	exit(1);
	/* NOTREACHED */
}

#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

int
main(argc, argv)
	int argc;
	char **argv;
{
	int ch;
	int rval = 0;

	if (argc < 2)
		usage(NULL);

	while ((ch = _getopt(argc, argv, "nqdtv")) != -1) {
		switch(ch) {
			case (int)'n':
				nflag = 1;
				break;
			case 'q':
				qflag = 1;
				break;
			case 'v':
				verbose = 1;
				break;
			case 't':
				tflag = 1;
				break;
			case 'd':
				debugonly = 1;
				break;
			default:
				usage(NULL);
		}
	}
	argc -= optind;
	argv += optind;

	pid = getpid();
	uid = getuid();
	if (tflag)
		s = open("/dev/null", O_WRONLY);
	else
		s = socket(PF_ROUTE, SOCK_RAW, 0);
	if (s < 0)
		quit("socket");
	if (*argv == NULL)
		goto no_cmd;
	switch (keyword(*argv)) {
	case K_GET:
		uid = 0;
		/* FALLTHROUGH */
	case K_CHANGE:
	case K_ADD:
	case K_DELETE:
		rval = newroute(argc, argv);
		break;
	case K_SHOW:
		uid = 0;
		show(argc, argv);
		break;
	case K_MONITOR:
		monitor();
		break;
	case K_FLUSH:
		flushroutes(argc, argv);
		break;
	no_cmd:
	default:
		usage(*argv);
	}
	exit(rval);
}

/*
 * Purge all entries in the routing tables not
 * associated with network interfaces.
 */
void
flushroutes(argc, argv)
	int argc;
	char *argv[];
{
	size_t needed;
	int mib[6], rlen, seqno;
	char *buf = NULL, *next, *lim = NULL;
	struct rt_msghdr *rtm;
	struct sockaddr *sa;

	if (uid) {
		errno = EACCES;
		quit("must be root to alter routing table");
	}
	shutdown(s, 0); /* Don't want to read back our messages */
	if (argc > 1) {
		argv++;
		if (argc == 2 && **argv == '-')
		    switch (keyword(*argv + 1)) {
			case K_INET:
				af = AF_INET;
				break;
#ifdef INET6
			case K_INET6:
				af = AF_INET6;
				break;
#endif
			case K_LINK:
				af = AF_LINK;
				break;
			default:
				goto bad;
		} else
bad:			usage(*argv);
	}
	mib[0] = PF_ROUTE;
	mib[1] = 0;		/* protocol */
	mib[2] = 0;		/* wildcard address family */
	mib[3] = NET_RT_DUMP;
	mib[4] = 0;		/* no flags */
	if (sysctl(mib, 5, NULL, &needed, NULL, 0) < 0)
		quit("route-sysctl-estimate");
	if (needed) {
		if ((buf = malloc(needed)) == NULL)
			quit("malloc");
		if (sysctl(mib, 5, buf, &needed, NULL, 0) < 0)
			quit("actual retrieval of routing table");
		lim = buf + needed;
	}
	if (verbose) {
		(void) printf("Examining routing table from sysctl\n");
		 if (af)
			printf("(address family %s)\n", (*argv + 1));
	}
	if (buf == NULL)
		return;

	seqno = 0;		/* ??? */
	for (next = buf; next < lim; next += rtm->rtm_msglen) {
		rtm = (struct rt_msghdr *)next;
		if (verbose)
			print_rtmsg(rtm, rtm->rtm_msglen);
		if ((rtm->rtm_flags & (RTF_GATEWAY|RTF_STATIC|RTF_LLINFO)) == 0)
			continue;
		sa = (struct sockaddr *)(rtm + 1);
		if (af) {
			if (sa->sa_family != af)
				continue;
		}
		if (debugonly)
			continue;
		rtm->rtm_type = RTM_DELETE;
		rtm->rtm_seq = seqno;
		rlen = write(s, next, rtm->rtm_msglen);
printf("write gave %d\n", rlen);
		if (rlen < (int)rtm->rtm_msglen) {
			(void) fprintf(stderr,
			    "route: write to routing socket: %s\n",
			    strerror(errno));
			(void) printf("got only %d for rlen\n", rlen);
			break;
		}
		seqno++;
		if (qflag)
			continue;
		if (verbose)
			print_rtmsg(rtm, rlen);
		else {
			struct sockaddr *sa = (struct sockaddr *)(rtm + 1);
			(void) printf("%-20.20s ", rtm->rtm_flags & RTF_HOST ?
			    routename(sa) : netname(sa));
			sa = (struct sockaddr *)(ROUNDUP(sa->sa_len) + (char *)sa);
			(void) printf("%-20.20s ", routename(sa));
			(void) printf("done\n");
		}
	}
	free(buf);
}

static char hexlist[] = "0123456789abcdef";

char *
any_ntoa(sa)
	const struct sockaddr *sa;
{
	static char obuf[240];
	const char *in = sa->sa_data;
	char *out = obuf;
	int len = sa->sa_len;

	*out++ = 'Q';
	do {
		*out++ = hexlist[(*in >> 4) & 15];
		*out++ = hexlist[(*in++)    & 15];
		*out++ = '.';
	} while (--len > 0 && (out + 3) < &obuf[sizeof obuf-1]);
	out[-1] = '\0';
	return (obuf);
}

char *routename(struct sockaddr *sa)
{
	char *cp = NULL;
	static char line[MAXHOSTNAMELEN];
	struct hostent *hp;
	static char domain[MAXHOSTNAMELEN];
	static int first = 1;
	char *ns_print();
	char *ipx_print();

	if (first) {
		first = 0;
		if (gethostname(domain, sizeof domain) == 0 &&
		    (cp = strchr(domain, '.')))
			(void) strcpy(domain, cp + 1);
		else
			domain[0] = 0;
		cp = NULL;
	}
	if (sa->sa_len == 0)
		strcpy(line, "default");
	else switch (sa->sa_family) {

	case AF_INET:
	    {	struct in_addr in;
		in = ((struct sockaddr_in *)sa)->sin_addr;

		if (in.s_addr == INADDR_ANY || sa->sa_len < 4)
			cp = "default";
		if (!cp && !nflag) {
			if ((hp = gethostbyaddr((char *)&in.s_addr,
			    sizeof (in.s_addr), AF_INET)) != NULL) {
				if ((cp = strchr(hp->h_name, '.')) &&
				    !strcmp(cp + 1, domain))
					*cp = 0;
				cp = hp->h_name;
			}
		}
		strncpy(line, cp ? cp : inet_ntoa(in), sizeof line);
		break;
	    }

#ifdef INET6
	case AF_INET6:
	    {
		struct sockaddr_in6 sin6;
		int niflags;

#ifdef NI_WITHSCOPEID
		niflags = NI_WITHSCOPEID;
#else
		niflags = 0;
#endif
		if (nflag)
			niflags |= NI_NUMERICHOST;
		memset(&sin6, 0, sizeof(sin6));
		memcpy(&sin6, sa, sa->sa_len);
		sin6.sin6_len = sizeof(struct sockaddr_in6);
		sin6.sin6_family = AF_INET6;
#ifdef __KAME__
		if (sa->sa_len == sizeof(struct sockaddr_in6) &&
		    (IN6_IS_ADDR_LINKLOCAL(&sin6.sin6_addr) ||
		     IN6_IS_ADDR_MC_LINKLOCAL(&sin6.sin6_addr)) &&
		    sin6.sin6_scope_id == 0) {
			sin6.sin6_scope_id =
			    ntohs(*(uint16 *)&sin6.sin6_addr.s6_addr[2]);
			sin6.sin6_addr.s6_addr[2] = 0;
			sin6.sin6_addr.s6_addr[3] = 0;
		}
#endif
		if (getnameinfo((struct sockaddr *)&sin6, sin6.sin6_len,
		    line, sizeof(line), NULL, 0, niflags) != 0)
			strncpy(line, "invalid", sizeof(line));
		break;
	    }
#endif

	case AF_LINK:
		return (link_ntoa((struct sockaddr_dl *)sa));

	default:
		(void) snprintf(line, sizeof line, "(%d) %s",
		    sa->sa_family, any_ntoa(sa));
		break;
	}
	return (line);
}

/*
 * Return the name of the network whose address is given.
 * The address is assumed to be that of a net or subnet, not a host.
 */
char *
netname(sa)
	struct sockaddr *sa;
{
	char *cp = NULL;
	static char line[MAXHOSTNAMELEN];
	struct netent *np = 0;
	in_addr_t net, mask, subnetshift;
	char *ns_print();
	char *ipx_print();

	switch (sa->sa_family) {
		case AF_INET:
	    {
			struct in_addr in = ((struct sockaddr_in *)sa)->sin_addr;
			in.s_addr = ntohl(in.s_addr);
			if (in.s_addr == 0)
				cp = "default";
			else if (!nflag) {
				if (IN_CLASSA(in.s_addr)) {
					mask = IN_CLASSA_NET;
					subnetshift = 8;
				} else if (IN_CLASSB(in.s_addr)) {
					mask = IN_CLASSB_NET;
					subnetshift = 8;
				} else {
					mask = IN_CLASSC_NET;
					subnetshift = 4;
				}
				/*
				 * If there are more bits than the standard mask
				 * would suggest, subnets must be in use.
				 * Guess at the subnet mask, assuming reasonable
				 * width subnet fields.
				 */
				while (in.s_addr &~ mask)
					mask = (int)mask >> subnetshift;

				net = in.s_addr & mask;
				while ((mask & 1) == 0)
					mask >>= 1, net >>= 1;
				np = getnetbyaddr(net, AF_INET);
				if (np)
					cp = np->n_name;
			}
			in = ((struct sockaddr_in *)sa)->sin_addr;
			strncpy(line, cp ? cp : inet_ntoa(in), sizeof line);
			break;
	    }
#ifdef INET6
	case AF_INET6:
	    {
		struct sockaddr_in6 sin6;
		int niflags;

#ifdef NI_WITHSCOPEID
		niflags = NI_WITHSCOPEID;
#else
		niflags = 0;
#endif
		if (nflag)
			niflags |= NI_NUMERICHOST;
		memset(&sin6, 0, sizeof(sin6));
		memcpy(&sin6, sa, sa->sa_len);
		sin6.sin6_len = sizeof(struct sockaddr_in6);
		sin6.sin6_family = AF_INET6;
#ifdef __KAME__
		if (sa->sa_len == sizeof(struct sockaddr_in6) &&
		    (IN6_IS_ADDR_LINKLOCAL(&sin6.sin6_addr) ||
		     IN6_IS_ADDR_MC_LINKLOCAL(&sin6.sin6_addr)) &&
		    sin6.sin6_scope_id == 0) {
			sin6.sin6_scope_id =
			    ntohs(*(uint16 *)&sin6.sin6_addr.s6_addr[2]);
			sin6.sin6_addr.s6_addr[2] = 0;
			sin6.sin6_addr.s6_addr[3] = 0;
		}
#endif

		if (getnameinfo((struct sockaddr *)&sin6, sin6.sin6_len,
		    line, sizeof(line), NULL, 0, niflags) != 0)
			strncpy(line, "invalid", sizeof(line));
		break;
	    }
#endif

	case AF_LINK:
		return (link_ntoa((struct sockaddr_dl *)sa));

	default:
		snprintf(line, sizeof line, "af %d: %s",
		    sa->sa_family, any_ntoa(sa));
		break;
	}
	return (line);
}

void
set_metric(value, key)
	char *value;
	int key;
{
	int flag = 0;
	u_long noval, *valp = &noval;

	switch (key) {
#define caseof(x, y, z)	case x: valp = &rt_metrics.z; flag = y; break
	caseof(K_MTU, RTV_MTU, rmx_mtu);
	caseof(K_HOPCOUNT, RTV_HOPCOUNT, rmx_hopcount);
	caseof(K_EXPIRE, RTV_EXPIRE, rmx_expire);
	caseof(K_RECVPIPE, RTV_RPIPE, rmx_recvpipe);
	caseof(K_SENDPIPE, RTV_SPIPE, rmx_sendpipe);
	caseof(K_SSTHRESH, RTV_SSTHRESH, rmx_ssthresh);
	caseof(K_RTT, RTV_RTT, rmx_rtt);
	caseof(K_RTTVAR, RTV_RTTVAR, rmx_rttvar);
	}
	rtm_inits |= flag;
	if (lockrest || locking)
		rt_metrics.rmx_locks |= flag;
	if (locking)
		locking = 0;
	*valp = atoi(value);
}

int newroute(int argc, char **argv)
{
	char *cmd, *dest = "", *gateway = "", *err;
	int ishost = 0, ret = 0, attempts, oerrno, flags = RTF_STATIC;
	int key;
	struct hostent *hp = 0;

	if (uid) {
		errno = EACCES;
		quit("must be root to alter routing table");
	}
	cmd = argv[0];
	if (*cmd != 'g')
		shutdown(s, 0); /* Don't want to read back our messages */
	while (--argc > 0) {
		if (**(++argv)== '-') {
			switch (key = keyword(1 + *argv)) {
			case K_LINK:
				af = AF_LINK;
				aflen = sizeof(struct sockaddr_dl);
				break;
			case K_INET:
				af = AF_INET;
				aflen = sizeof(struct sockaddr_in);
				break;
#ifdef INET6
			case K_INET6:
				af = AF_INET6;
				aflen = sizeof(struct sockaddr_in6);
				break;
#endif
			case K_SA:
				af = PF_ROUTE;
				aflen = sizeof(union sockunion);
				break;
			case K_IFACE:
			case K_INTERFACE:
				iflag++;
				break;
			case K_NOSTATIC:
				flags &= ~RTF_STATIC;
				break;
			case K_LLINFO:
				flags |= RTF_LLINFO;
				break;
			case K_LOCK:
				locking = 1;
				break;
			case K_LOCKREST:
				lockrest = 1;
				break;
			case K_HOST:
				forcehost++;
				break;
			case K_REJECT:
				flags |= RTF_REJECT;
				break;
			case K_BLACKHOLE:
				flags |= RTF_BLACKHOLE;
				break;
			case K_PROTO1:
				flags |= RTF_PROTO1;
				break;
			case K_PROTO2:
				flags |= RTF_PROTO2;
				break;
			case K_CLONING:
				flags |= RTF_CLONING;
				break;
			case K_XRESOLVE:
				flags |= RTF_XRESOLVE;
				break;
			case K_STATIC:
				flags |= RTF_STATIC;
				break;
			case K_IFA:
				if (!--argc)
					usage(1+*argv);
				(void) getaddr(RTA_IFA, *++argv, 0);
				break;
			case K_IFP:
				if (!--argc)
					usage(1+*argv);
				(void) getaddr(RTA_IFP, *++argv, 0);
				break;
			case K_GENMASK:
				if (!--argc)
					usage(1+*argv);
				(void) getaddr(RTA_GENMASK, *++argv, 0);
				break;
			case K_GATEWAY:
				if (!--argc)
					usage(1+*argv);
				(void) getaddr(RTA_GATEWAY, *++argv, 0);
				break;
			case K_DST:
				if (!--argc)
					usage(1+*argv);
				ishost = getaddr(RTA_DST, *++argv, &hp);
				dest = *argv;
				break;
			case K_NETMASK:
				if (!--argc)
					usage(1+*argv);
				(void) getaddr(RTA_NETMASK, *++argv, 0);
				/* FALLTHROUGH */
			case K_NET:
				forcenet++;
				break;
#ifdef INET6
			case K_PREFIXLEN:
				argc--;
				if (prefixlen(*++argv) == 128) {
					forcenet = 0;
					ishost = 1;
				} else {
					forcenet = 1;
					ishost = 0;
				}
				break;
#endif
			case K_MTU:
			case K_HOPCOUNT:
			case K_EXPIRE:
			case K_RECVPIPE:
			case K_SENDPIPE:
			case K_SSTHRESH:
			case K_RTT:
			case K_RTTVAR:
				if (!--argc)
					usage(1+*argv);
				set_metric(*++argv, key);
				break;
			default:
				usage(1+*argv);
			}
		} else {
			if ((rtm_addrs & RTA_DST) == 0) {
				dest = *argv;
				ishost = getaddr(RTA_DST, *argv, &hp);
			} else if ((rtm_addrs & RTA_GATEWAY) == 0) {
				gateway = *argv;
				(void) getaddr(RTA_GATEWAY, *argv, &hp);
			} else {
				int hops = atoi(*argv);

				if (hops == 0) {
				    if (!qflag && strcmp(*argv, "0") == 0)
					printf("%s,%s",
					    "old usage of trailing 0",
					    "assuming route to if\n");
				    else
					usage(NULL);
				    iflag = 1;
				    continue;
				} else if (hops > 0 && hops < 10) {
				    if (!qflag) {
					printf("old usage of trailing digit, ");
					printf("assuming route via gateway\n");
				    }
				    iflag = 0;
				    continue;
				}
				(void) getaddr(RTA_NETMASK, *argv, 0);
			}
		}
	}
	if (forcehost)
		ishost = 1;
	if (forcenet)
		ishost = 0;
	flags |= RTF_UP;
	if (ishost)
		flags |= RTF_HOST;
	if (iflag == 0)
		flags |= RTF_GATEWAY;
	for (attempts = 1; ; attempts++) {
		errno = 0;
		if ((ret = rtmsg(*cmd, flags)) == 0)
			break;
		if (errno != ENETUNREACH && errno != ESRCH)
			break;
		if (af == AF_INET && *gateway && hp && hp->h_addr_list[1]) {
			hp->h_addr_list++;
			memcpy(&so_gate.sin.sin_addr, hp->h_addr_list[0],
			    hp->h_length);
		} else
			break;
	}
	if (*cmd == 'g')
		exit(0);
	oerrno = errno;
	if (!qflag) {
		(void) printf("%s %s %s", cmd, ishost? "host" : "net", dest);
		if (*gateway) {
			(void) printf(": gateway %s", gateway);
			if (attempts > 1 && ret == 0 && af == AF_INET)
			    (void) printf(" (%s)",
				inet_ntoa(so_gate.sin.sin_addr));
		}
		if (ret == 0)
			(void) printf("\n");
	}
	if (ret != 0) {
		switch (oerrno) {
		case ESRCH:
			err = "not in table";
			break;
		case EBUSY:
			err = "entry in use";
			break;
		case ENOBUFS:
			err = "routing table overflow";
			break;
		default:
			err = strerror(oerrno);
			break;
		}
		(void) printf(": %s\n", err);
	}
	return (ret != 0);
}

void inet_makenetandmask(uint32 net, struct sockaddr_in *sin, int bits)
{
	uint32 addr, mask = 0;
	char *cp;

	rtm_addrs |= RTA_NETMASK;
	if (net == 0)
		mask = addr = 0;
	else if (bits) {
		addr = net;
		mask = 0xffffffff << (32 - bits);
	} else if (net < 128) {
		addr = net << IN_CLASSA_NSHIFT;
		mask = IN_CLASSA_NET;
	} else if (net < 65536) {
		addr = net << IN_CLASSB_NSHIFT;
		mask = IN_CLASSB_NET;
	} else if (net < 16777216L) {
		addr = net << IN_CLASSC_NSHIFT;
		mask = IN_CLASSC_NET;
	} else {
		addr = net;
		if ((addr & IN_CLASSA_HOST) == 0)
			mask =  IN_CLASSA_NET;
		else if ((addr & IN_CLASSB_HOST) == 0)
			mask =  IN_CLASSB_NET;
		else if ((addr & IN_CLASSC_HOST) == 0)
			mask =  IN_CLASSC_NET;
		else
			mask = -1;
	}
	sin->sin_addr.s_addr = htonl(addr);
	sin = &so_mask.sin;
	sin->sin_addr.s_addr = htonl(mask);
	sin->sin_len = 0;
	sin->sin_family = 0;
	cp = (char *)(&sin->sin_addr + 1);
	while (*--cp == 0 && cp > (char *)sin)
		;
	sin->sin_len = 1 + cp - (char *)sin;
}

/*
 * Interpret an argument as a network address of some kind,
 * returning 1 if a host address, 0 if a network address.
 */
int getaddr(int which, char *s, struct hostent **hpp)
{
	sup su = NULL;
	struct hostent *hp;
	struct netent *np;
	u_long val;
	char *q, qs;
	int afamily;

	if (af == 0) {
		af = AF_INET;
		aflen = sizeof(struct sockaddr_in);
	}
	afamily = af;	/* local copy of af so we can change it */
	rtm_addrs |= which;
	switch (which) {
	case RTA_DST:
		su = &so_dst;
		break;
	case RTA_GATEWAY:
		su = &so_gate;
		break;
	case RTA_NETMASK:
		su = &so_mask;
		break;
	case RTA_GENMASK:
		su = &so_genmask;
		break;
	case RTA_IFP:
		su = &so_ifp;
		afamily = AF_LINK;
		break;
	case RTA_IFA:
		su = &so_ifa;
		su->sa.sa_family = af;
		break;
	default:
		usage("Internal Error");
		/*NOTREACHED*/
	}
	su->sa.sa_len = aflen;
	su->sa.sa_family = afamily; /* cases that don't want it have left already */
	if (strcmp(s, "default") == 0) {
		switch (which) {
		case RTA_DST:
			forcenet++;
			(void) getaddr(RTA_NETMASK, s, 0);
			break;
		case RTA_NETMASK:
		case RTA_GENMASK:
			su->sa.sa_len = 0;
		}
		return (0);
	}
	switch (afamily) {
#ifdef INET6
	case AF_INET6:
	    {
		struct addrinfo hints, *res;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = afamily;	/*AF_INET6*/
		hints.ai_flags = AI_NUMERICHOST;
		hints.ai_socktype = SOCK_DGRAM;		/*dummy*/
		if (getaddrinfo(s, "0", &hints, &res) != 0 ||
		    res->ai_family != AF_INET6 ||
		    res->ai_addrlen != sizeof(su->sin6)) {
			(void) fprintf(stderr, "%s: bad value\n", s);
			exit(1);
		}
		memcpy(&su->sin6, res->ai_addr, sizeof(su->sin6));
#ifdef __KAME__
		if ((IN6_IS_ADDR_LINKLOCAL(&su->sin6.sin6_addr) ||
		     IN6_IS_ADDR_MC_LINKLOCAL(&su->sin6.sin6_addr)) &&
		    su->sin6.sin6_scope_id) {
			*(uint16 *)&su->sin6.sin6_addr.s6_addr[2] =
				htons(su->sin6.sin6_scope_id);
			su->sin6.sin6_scope_id = 0;
		}
#endif
		if (which == RTA_DST)
			inet6_makenetandmask(&su->sin6);
		freeaddrinfo(res);
		return (0);
	    }
#endif

	case AF_LINK:
		link_addr(s, &su->sdl);
		return (1);

	case PF_ROUTE:
		su->sa.sa_len = sizeof(*su);
		sockaddr(s, &su->sa);
		return (1);

	case AF_INET:
	default:
		break;
	}

	if (hpp == NULL)
		hpp = &hp;
	*hpp = NULL;

	q = strchr(s,'/');
	if (q && which == RTA_DST) {
		qs = *q;
		*q = '\0';
		val = inet_addr(s);
		if (val != INADDR_NONE) {
			inet_makenetandmask(htonl(val), &su->sin,
			    strtoul(q+1, 0, 0));
			return (0);
		}
		*q =qs;
	}
	if (((val = inet_addr(s)) != INADDR_NONE) &&
	    (which != RTA_DST || forcenet == 0)) {
		su->sin.sin_addr.s_addr = val;
		if (inet_lnaof(su->sin.sin_addr) != INADDR_ANY)
			return (1);
		else {
			val = ntohl(val);
			goto netdone;
		}
	}
	if ((val = inet_network(s)) != INADDR_NONE ||
	    (forcehost == 0 && (np = getnetbyname(s)) != NULL &&
	    (val = np->n_net) != 0)) {
netdone:
		if (which == RTA_DST)
			inet_makenetandmask(val, &su->sin, 0);
		return (0);
	}
	hp = gethostbyname(s);
	if (hp) {
		*hpp = hp;
		su->sin.sin_family = hp->h_addrtype;
		memcpy(&su->sin.sin_addr, hp->h_addr, hp->h_length);
		return (1);
	}
	(void) fprintf(stderr, "route: %s: bad value\n", s);
	exit(1);
}

#ifdef INET6
int
prefixlen(s)
	char *s;
{
	int len = atoi(s), q, r;

	rtm_addrs |= RTA_NETMASK;
	if (len < -1 || len > 129) {
		(void) fprintf(stderr, "%s: bad value\n", s);
		exit(1);
	}

	q = len >> 3;
	r = len & 7;
	so_mask.sin6.sin6_family = AF_INET6;
	so_mask.sin6.sin6_len = sizeof(struct sockaddr_in6);
	memset((void *)&so_mask.sin6.sin6_addr, 0,
		sizeof(so_mask.sin6.sin6_addr));
	if (q > 0)
		memset((void *)&so_mask.sin6.sin6_addr, 0xff, q);
	if (r > 0)
		*((u_char *)&so_mask.sin6.sin6_addr + q) = (0xff00 >> r) & 0xff;
	return (len);
}
#endif

short ns_nullh[] = {0,0,0};
short ns_bh[] = {-1,-1,-1};


short ipx_nullh[] = {0,0,0};
short ipx_bh[] = {-1,-1,-1};

void
interfaces()
{
	size_t needed;
	int mib[6];
	char *buf = NULL, *lim, *next;
	struct rt_msghdr *rtm;

	mib[0] = PF_ROUTE;
	mib[1] = 0;		/* protocol */
	mib[2] = 0;		/* wildcard address family */
	mib[3] = NET_RT_IFLIST;
	mib[4] = 0;		/* no flags */
	if (sysctl(mib, 5, NULL, &needed, NULL, 0) < 0)
		quit("route-sysctl-estimate");
	if (needed) {
		if ((buf = malloc(needed)) == NULL)
			quit("malloc");
		if (sysctl(mib, 5, buf, &needed, NULL, 0) < 0)
			quit("actual retrieval of interface table");
		lim = buf + needed;
		for (next = buf; next < lim; next += rtm->rtm_msglen) {
			rtm = (struct rt_msghdr *)next;
			print_rtmsg(rtm, rtm->rtm_msglen);
		}
		free(buf);
	}
}

void
monitor()
{
	int n;
	char msg[2048];

	verbose = 1;
	if (debugonly) {
		interfaces();
		exit(0);
	}
	for(;;) {
		time_t now;
		n = read(s, msg, 2048);
		now = time(NULL);
		(void) printf("got message of size %d on %s", n, ctime(&now));
		print_rtmsg((struct rt_msghdr *)msg, n);
	}
}

struct {
	struct	rt_msghdr m_rtm;
	char	m_space[512];
} m_rtmsg;

int
rtmsg(cmd, flags)
	int cmd, flags;
{
	static int seq;
	int rlen;
	char *cp = m_rtmsg.m_space;
	int l;

#define NEXTADDR(w, u) \
	if (rtm_addrs & (w)) {\
	    l = ROUNDUP(u.sa.sa_len); memcpy(cp, &(u), l); cp += l;\
	    if (verbose) sodump(&(u),"u");\
	}

	errno = 0;
	memset(&m_rtmsg, 0, sizeof(m_rtmsg));
	if (cmd == 'a')
		cmd = RTM_ADD;
	else if (cmd == 'c')
		cmd = RTM_CHANGE;
	else if (cmd == 'g') {
		cmd = RTM_GET;
		if (so_ifp.sa.sa_family == 0) {
			so_ifp.sa.sa_family = AF_LINK;
			so_ifp.sa.sa_len = sizeof(struct sockaddr_dl);
			rtm_addrs |= RTA_IFP;
		}
	} else
		cmd = RTM_DELETE;
#define rtm m_rtmsg.m_rtm
	rtm.rtm_type = cmd;
	rtm.rtm_flags = flags;
	rtm.rtm_version = RTM_VERSION;
	rtm.rtm_seq = ++seq;
	rtm.rtm_addrs = rtm_addrs;
	rtm.rtm_rmx = rt_metrics;
	rtm.rtm_inits = rtm_inits;

	if (rtm_addrs & RTA_NETMASK)
		mask_addr();
	NEXTADDR(RTA_DST, so_dst);
	NEXTADDR(RTA_GATEWAY, so_gate);
	NEXTADDR(RTA_NETMASK, so_mask);
	NEXTADDR(RTA_GENMASK, so_genmask);
	NEXTADDR(RTA_IFP, so_ifp);
	NEXTADDR(RTA_IFA, so_ifa);
	rtm.rtm_msglen = l = cp - (char *)&m_rtmsg;
	if (verbose)
		print_rtmsg(&rtm, l);
	if (debugonly)
		return (0);
	if ((rlen = write(s, (char *)&m_rtmsg, l)) < 0) {
		perror("writing to routing socket");
		return (-1);
	}
	if (cmd == RTM_GET) {
		do {
			l = read(s, (char *)&m_rtmsg, sizeof(m_rtmsg));
		} while (l > 0 && (rtm.rtm_seq != seq));
		if (l < 0)
			(void) fprintf(stderr,
			    "route: read from routing socket: %s\n",
			    strerror(errno));
		else
			print_getmsg(&rtm, l);
	}
#undef rtm
	return (0);
}

void mask_addr()
{
	int olen = so_mask.sa.sa_len;
	char *cp1 = olen + (char *)&so_mask, *cp2;

	for (so_mask.sa.sa_len = 0; cp1 > (char *)&so_mask; )
		if (*--cp1 != 0) {
			so_mask.sa.sa_len = 1 + cp1 - (char *)&so_mask;
			break;
		}
	if ((rtm_addrs & RTA_DST) == 0)
		return;
	switch (so_dst.sa.sa_family) {
	case AF_INET:
#ifdef INET6
	case AF_INET6:
#endif
	case 0:
		return;
	}
	cp1 = so_mask.sa.sa_len + 1 + (char *)&so_dst;
	cp2 = so_dst.sa.sa_len + 1 + (char *)&so_dst;
	while (cp2 > cp1)
		*--cp2 = 0;
	cp2 = so_mask.sa.sa_len + 1 + (char *)&so_mask;
	while (cp1 > (char*)so_dst.sa.sa_data)
		*--cp1 &= *--cp2;
}

char *msgtypes[] = {
	"",
	"RTM_ADD: Add Route",
	"RTM_DELETE: Delete Route",
	"RTM_CHANGE: Change Metrics or flags",
	"RTM_GET: Report Metrics",
	"RTM_LOSING: Kernel Suspects Partitioning",
	"RTM_REDIRECT: Told to use different route",
	"RTM_MISS: Lookup failed on this address",
	"RTM_LOCK: fix specified metrics",
	"RTM_OLDADD: caused by SIOCADDRT",
	"RTM_OLDDEL: caused by SIOCDELRT",
	"RTM_RESOLVE: Route created by cloning",
	"RTM_NEWADDR: address being added to iface",
	"RTM_DELADDR: address being removed from iface",
	"RTM_IFINFO: iface status change",
	0,
};

char metricnames[] =
"\011pksent\010rttvar\7rtt\6ssthresh\5sendpipe\4recvpipe\3expire\2hopcount\1mtu";
char routeflags[] =
"\1UP\2GATEWAY\3HOST\4REJECT\5DYNAMIC\6MODIFIED\7DONE\010MASK_PRESENT\011CLONING\012XRESOLVE\013LLINFO\014STATIC\017PROTO2\020PROTO1";
char ifnetflags[] =
"\1UP\2BROADCAST\3DEBUG\4LOOPBACK\5PTP\6NOTRAILERS\7RUNNING\010NOARP\011PPROMISC\012ALLMULTI\013OACTIVE\014SIMPLEX\015LINK0\016LINK1\017LINK2\020MULTICAST";
char addrnames[] =
"\1DST\2GATEWAY\3NETMASK\4GENMASK\5IFP\6IFA\7AUTHOR\010BRD";

void print_rtmsg(struct rt_msghdr *rtm, int msglen)
{
	struct if_msghdr *ifm;
	struct ifa_msghdr *ifam;

	if (verbose == 0)
		return;
	if (rtm->rtm_version != RTM_VERSION) {
		(void) printf("routing message version %d not understood\n",
		    rtm->rtm_version);
		return;
	}
	(void)printf("%s: len %d, ", msgtypes[rtm->rtm_type], rtm->rtm_msglen);
	switch (rtm->rtm_type) {
	case RTM_IFINFO:
		ifm = (struct if_msghdr *)rtm;
		(void) printf("if# %d, flags:", ifm->ifm_index);
		bprintf(stdout, ifm->ifm_flags, ifnetflags);
		pmsg_addrs((char *)(ifm + 1), ifm->ifm_addrs);
		break;
	case RTM_NEWADDR:
	case RTM_DELADDR:
		ifam = (struct ifa_msghdr *)rtm;
		(void) printf("metric %d, flags:", ifam->ifam_metric);
		bprintf(stdout, ifam->ifam_flags, routeflags);
		pmsg_addrs((char *)(ifam + 1), ifam->ifam_addrs);
		break;
	default:
		(void) printf("seq %d, errno %d, flags:",
			rtm->rtm_seq, rtm->rtm_errno);
		bprintf(stdout, rtm->rtm_flags, routeflags);
		pmsg_common(rtm);
	}
}

void print_getmsg(struct rt_msghdr *rtm, int msglen)
{
	struct sockaddr *dst = NULL, *gate = NULL, *mask = NULL;
	struct sockaddr_dl *ifp = NULL;
	struct sockaddr *sa;
	char *cp;
	int i;

	(void) printf("   route to: %s\n", routename(&so_dst.sa));
	if (rtm->rtm_version != RTM_VERSION) {
		(void)fprintf(stderr,
		    "routing message version %d not understood\n",
		    rtm->rtm_version);
		return;
	}
	if (rtm->rtm_msglen > msglen) {
		(void)fprintf(stderr,
		    "message length mismatch, in packet %d, returned %d\n",
		    rtm->rtm_msglen, msglen);
	}
	if (rtm->rtm_errno)  {
		(void) fprintf(stderr, "RTM_GET: %s (errno %d)\n",
		    strerror(rtm->rtm_errno), rtm->rtm_errno);
		return;
	}
	cp = ((char *)(rtm + 1));
	if (rtm->rtm_addrs)
		for (i = 1; i; i <<= 1)
			if (i & rtm->rtm_addrs) {
				sa = (struct sockaddr *)cp;
				switch (i) {
				case RTA_DST:
					dst = sa;
					break;
				case RTA_GATEWAY:
					gate = sa;
					break;
				case RTA_NETMASK:
					mask = sa;
					break;
				case RTA_IFP:
					if (sa->sa_family == AF_LINK &&
					   ((struct sockaddr_dl *)sa)->sdl_nlen)
						ifp = (struct sockaddr_dl *)sa;
					break;
				}
				ADVANCE(cp, sa);
			}
	if (dst && mask)
		mask->sa_family = dst->sa_family;	/* XXX */
	if (dst)
		(void)printf("destination: %s\n", routename(dst));
	if (mask) {
		int savenflag = nflag;

		nflag = 1;
		(void)printf("       mask: %s\n", routename(mask));
		nflag = savenflag;
	}
	if (gate && rtm->rtm_flags & RTF_GATEWAY)
		(void)printf("    gateway: %s\n", routename(gate));
	if (ifp)
		(void)printf("  interface: %.*s\n",
		    ifp->sdl_nlen, ifp->sdl_data);
	(void)printf("      flags: ");
	bprintf(stdout, rtm->rtm_flags, routeflags);

#define lock(f)	((rtm->rtm_rmx.rmx_locks & __CONCAT(RTV_,f)) ? 'L' : ' ')
#define msec(u)	(((u) + 500) / 1000)		/* usec to msec */

	(void) printf("\n%s\n", "\
 recvpipe  sendpipe  ssthresh  rtt,msec    rttvar  hopcount      mtu     expire");
	printf("%8d%c ", (int)rtm->rtm_rmx.rmx_recvpipe, lock(RPIPE));
	printf("%8d%c ", (int)rtm->rtm_rmx.rmx_sendpipe, lock(SPIPE));
	printf("%8d%c ", (int)rtm->rtm_rmx.rmx_ssthresh, lock(SSTHRESH));
	printf("%8d%c ", (int)msec(rtm->rtm_rmx.rmx_rtt), lock(RTT));
	printf("%8d%c ", (int)msec(rtm->rtm_rmx.rmx_rttvar), lock(RTTVAR));
	printf("%8d%c ", (int)rtm->rtm_rmx.rmx_hopcount, lock(HOPCOUNT));
	printf("%8d%c ", (int)rtm->rtm_rmx.rmx_mtu, lock(MTU));
	if (rtm->rtm_rmx.rmx_expire)
		rtm->rtm_rmx.rmx_expire -= time(0);
	printf("%8d%c\n", (int)rtm->rtm_rmx.rmx_expire, lock(EXPIRE));
#undef lock
#undef msec
#define	RTA_IGN	(RTA_DST|RTA_GATEWAY|RTA_NETMASK|RTA_IFP|RTA_IFA|RTA_BRD)
	if (verbose)
		pmsg_common(rtm);
	else if (rtm->rtm_addrs &~ RTA_IGN) {
		(void) printf("sockaddrs: ");
		bprintf(stdout, rtm->rtm_addrs, addrnames);
		putchar('\n');
	}
#undef	RTA_IGN
}

void pmsg_common(struct rt_msghdr *rtm)
{
	(void) printf("\nlocks: ");
	bprintf(stdout, rtm->rtm_rmx.rmx_locks, metricnames);
	(void) printf(" inits: ");
	bprintf(stdout, rtm->rtm_inits, metricnames);
	pmsg_addrs(((char *)(rtm + 1)), rtm->rtm_addrs);
}

void pmsg_addrs(char *cp, int addrs)
{
	struct sockaddr *sa;
	int i;

	if (addrs == 0)
		return;
	(void) printf("\nsockaddrs: ");
	bprintf(stdout, addrs, addrnames);
	(void) putchar('\n');
	for (i = 1; i; i <<= 1)
		if (i & addrs) {
			sa = (struct sockaddr *)cp;
			(void) printf(" %s", routename(sa));
			ADVANCE(cp, sa);
		}
	(void) putchar('\n');
	(void) fflush(stdout);
}

void bprintf(FILE *fp, int b, u_char *s)
{
	int i;
	int gotsome = 0;

	if (b == 0)
		return;
	while ((i = *s++)) {
		if ((b & (1 << (i-1)))) {
			if (gotsome == 0)
				i = '<';
			else
				i = ',';
			(void) putc(i, fp);
			gotsome = 1;
			for (; (i = *s) > 32; s++)
				(void) putc(i, fp);
		} else
			while (*s > 32)
				s++;
	}
	if (gotsome)
		(void) putc('>', fp);
}

int keyword(char *cp)
{
	struct keytab *kt = keywords;

	while (kt->kt_cp && strcmp(kt->kt_cp, cp))
		kt++;
	return (kt->kt_i);
}

void sodump(sup su, char *which)
{
	char ntop_buf[NI_MAXHOST];	/*for inet_ntop()*/

	switch (su->sa.sa_family) {
	case AF_LINK:
		(void) printf("%s: link %s; ",
		    which, link_ntoa(&su->sdl));
		break;
	case AF_INET:
		(void) printf("%s: inet %s; ",
		    which, inet_ntop(AF_INET, &su->sin.sin_addr,
		                     ntop_buf, sizeof(ntop_buf)));
		break;
#ifdef INET6
	case AF_INET6:
		(void) printf("%s: inet6 %s; ",
		    which, inet_ntop(AF_INET6, &su->sin6.sin6_addr,
				     ntop_buf, sizeof(ntop_buf)));
		break;
#endif
	}
	(void) fflush(stdout);
}

/* States*/
#define VIRGIN	0
#define GOTONE	1
#define GOTTWO	2
/* Inputs */
#define	DIGIT	(4*0)
#define	END	(4*1)
#define DELIM	(4*2)

void sockaddr(char *addr, struct sockaddr *sa)
{
	char *cp = (char *)sa;
	int size = sa->sa_len;
	char *cplim = cp + size;
	int byte = 0, state = VIRGIN, new = 0;

	memset(cp, 0, size);
	cp++;
	do {
		if ((*addr >= '0') && (*addr <= '9')) {
			new = *addr - '0';
		} else if ((*addr >= 'a') && (*addr <= 'f')) {
			new = *addr - 'a' + 10;
		} else if ((*addr >= 'A') && (*addr <= 'F')) {
			new = *addr - 'A' + 10;
		} else if (*addr == 0)
			state |= END;
		else
			state |= DELIM;
		addr++;
		switch (state /* | INPUT */) {
		case GOTTWO | DIGIT:
			*cp++ = byte; /*FALLTHROUGH*/
		case VIRGIN | DIGIT:
			state = GOTONE; byte = new; continue;
		case GOTONE | DIGIT:
			state = GOTTWO; byte = new + (byte << 4); continue;
		default: /* | DELIM */
			state = VIRGIN; *cp++ = byte; byte = 0; continue;
		case GOTONE | END:
		case GOTTWO | END:
			*cp++ = byte; /* FALLTHROUGH */
		case VIRGIN | END:
			break;
		}
		break;
	} while (cp < cplim);
	sa->sa_len = cp - (char *)sa;
}
