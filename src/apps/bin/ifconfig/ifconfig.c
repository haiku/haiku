/* ifconfig.c */

#include <stdio.h>
#include <unistd.h>
#include <kernel/OS.h>
#include <stdlib.h>	/* for exit() */
#include <errno.h>
#include <string.h>

#include "netinet/in.h"
#include "sys/socket.h"
#include "sys/sockio.h"
#include "arpa/inet.h"
#include "net/if.h"
#include "net/if_dl.h"

#define version           "0.2 pre-alpha"
#define	NEXTARG		0xffffff

/* globals */
int sock;                    /* the socket we're using */
int flags = 0;               /* interface flags */
int setaddr;
struct ifreq ifr;            /* our interface information */
struct ifreq ridreq;         /* ?? */
struct ifaliasreq addreq;    /* aliases details */
struct	sockaddr_in	netmask; /* the netmask */
int mflag;
int newaddr = 0;             /* do we have a new address? */
int doalias;                 /* do we process alias information */
int clearaddr;               /* are we clearing the address? */
char	name[IFNAMSIZ];      /* the name of the interface we're using */

/* we only deal with a single interface at once, so these globals
 * refer to their respective information for that interface
 */
int af = AF_INET;            /* address family we're dealing with */
int metric;                  /* the metric */
int mtu;                     /* mtu vlaue */

/* forward declarations... */
int getinfo(struct ifreq *ifr);
void setifaddr(char *addr, int param);
void setifflags(char *vname, int value);
void setifnetmask(char *addr);
void setifmetric(char *val);
void setifmtu(char *val, int d);
void in_status(int force);
void in_getaddr(char *s, int which);
void in_getprefix(char *plen, int which);
void status(int link);
void printif(struct ifreq *ifrm, int ifaliases);

/* The commands we recognise... */
struct _cmd_ {
	char *c_name;          /* what the command line will say */
	int c_parameter;       /* the parameter, NEXTARG = next argv */
	void (*c_func)();      /* the function to call */
} cmds [] = {
	{"up",      IFF_UP,   setifflags},
	{"down",   -IFF_UP,   setifflags},
	{"netmask", NEXTARG,  setifnetmask},
	{"metric",  NEXTARG,  setifmetric},
	{"mtu",     NEXTARG,  setifmtu},
	{NULL,      /*src*/0, setifaddr },
//	{NULL,      /*dst*/0, setifdstaddr },
	{NULL, 0, NULL}
};

/* Known address families - thus far ONLY AF_INET */
/* XXX - it'd be very cool if we could have a way of doing this
 * by modules/add-ons :)
 */
const struct afswtch {
	char *af_name;
	short af_af;
	void (*af_status)();
	void (*af_getaddr)();
	void (*af_getprefix)();
	u_long af_difaddr;
	u_long af_aifaddr;
	caddr_t af_ridreq;
	caddr_t af_addreq;
} afs[] = {
#define C(x) ((caddr_t) &x)
	{ "inet", AF_INET, in_status, in_getaddr, in_getprefix,
	     SIOCDIFADDR, SIOCAIFADDR, C(ridreq), C(addreq) },
	{ 0,	0,	    0,		0 }
};

const struct afswtch *afp;	/*the address family being set or asked about*/

void usage()
{
	fprintf(stderr, "OpenBeOS Network Team: ifconfig: version %s\n", version);
	fprintf(stderr, "usage: ifconfig [ -m ] [ -a ] [ -A ] [ interface ]\n"
		"\t[ [af] [ address [ dest_addr ] ] [ up ] [ down ] "
		"[ netmask mask ] ]\n"
		"\t[ metric n ]\n"
		"\t[ mtu n ]\n"
		"       ifconfig [-a | -A | -am | -Am] [ af ]\n"
		"       ifconfig -m interface [af]\n");
	exit(1);
}

static void err(int exitval, char *where)
{
	printf("error: %s: error %d [%s]\n", where, errno, strerror(errno));
	exit(exitval);
}

static void errx(int exitval, char *fmt_string, char *value)
{
	printf("error: ");
	printf(fmt_string, value);
	exit(exitval);
}

static void warn(char *what)
{
	printf("warning: %s: error %d [%s]\n", what, errno, strerror(errno));
}
	
int main(int argc, char **argv)
{
	int aflag = 0, ifaliases = 0;
	const struct afswtch *rafp = NULL;
	int i;
	
	mflag = 0;
	
	if (argc < 2)
		usage();
	argc--, argv++; /* skip app name */

	/* handle common optins and get the interface name */
	if (!strcmp(*argv, "-a"))
		aflag = 1;
	else if (!strcmp(*argv, "-A")) {
		aflag = 1;
		ifaliases = 1;
	} else if (!strcmp(*argv, "-ma") || !strcmp(*argv, "-am")) {
		aflag = 1;
		mflag = 1;
	}
	else if (!strcmp(*argv, "-mA") || !strcmp(*argv, "-Am")) {
		aflag = 1;
		ifaliases = 1;
		mflag = 1;
	}
	else if (!strcmp(*argv, "-m")) {
		mflag = 1;
		argc--, argv++;
		if (argc < 1)
			usage();
		(void) strcpy(name, *argv);
	} else
		(void) strcpy(name, *argv);
	argc--, argv++;

	/* Try to establish the address family we're talking about
	 * and set the appropriate structure of commands into the
	 * pointer afp;
	 */
	if (argc > 0) {
		for (afp = rafp = afs; rafp->af_name; rafp++)
			if (strcmp(rafp->af_name, *argv) == 0) {
				afp = rafp; argc--; argv++;
				break;
			}
		rafp = afp;
		af = ifr.ifr_addr.sa_family = rafp->af_af;
	}

	if (argc > 0) {
		for (afp = rafp = afs; rafp->af_name; rafp++)
			if (strcmp(rafp->af_name, *argv) == 0) {
				printf("afp: %s\n", *argv);
				afp = rafp; argc--; argv++;
				break;
			}
		rafp = afp;
		af = ifr.ifr_addr.sa_family = rafp->af_af;
	}

	if (aflag) {
		if (argc > 0)
			usage();
		printif(NULL, ifaliases);
		exit(0);
	}

	/* put the name in the ifr structure */
	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	/* if there's no more arguments then print information
	 * on the named interface */
	if (argc == 0) {
		printif(&ifr, 1);
		exit(0);
	}

	/* we've probably got something to do, so get the information */
	if (getinfo(&ifr) < 0)
		exit(1);

	while (argc > 0) {
		const struct _cmd_ *p;

		for (p = cmds; p->c_name; p++)
			if (strcmp(*argv, p->c_name) == 0)
				break;
			/* we've found a command! */
		if (p->c_name == NULL && setaddr) {
			printf("pc->name = NULL!\n");
			for (i = setaddr; i > 0; i--) {
				p++;
				if (p->c_func == NULL)
					err(1, "extra address not accepted");
			}
		}
		if (p->c_func) {
			if (p->c_parameter == NEXTARG) {
				if (argv[1] == NULL)
					errx(1, "'%s' requires argument",
					    p->c_name);
				(*p->c_func)(argv[1]);
				argc--, argv++;
			} else
				(*p->c_func)(*argv, p->c_parameter);
		} 

		argc--, argv++;
	}
	if (clearaddr) {
		int ret;
		strncpy(rafp->af_ridreq, name, sizeof(ifr.ifr_name));
		if ((ret = ioctl(sock, rafp->af_difaddr, rafp->af_ridreq)) < 0) {
			if (errno == EADDRNOTAVAIL && (doalias >= 0)) {
				/* means no previous address for interface */
			} else
				warn("SIOCDIFADDR");
		}
	}
	if (newaddr) {
		strncpy(rafp->af_addreq, name, sizeof(ifr.ifr_name));
		if (ioctl(sock, rafp->af_aifaddr, rafp->af_addreq) < 0)
			warn("SIOCAIFADDR");
	}
	exit(0);
}

static void getsock(int naf)
{
	static int oaf = -1;

	if (oaf == naf)
		return;
	if (oaf != -1)
		close(sock);
	sock = socket(naf, SOCK_DGRAM, 0);
	if (sock < 0)
		oaf = -1;
	else
		oaf = naf;
}

int getinfo(struct ifreq *ifr)
{
	getsock(af);
	if (sock < 0)
		err(1, "socket");
	if (ioctl(sock, SIOCGIFFLAGS, (caddr_t)ifr) < 0) {
		warn("SIOCGIFFLAGS");
		return (-1);
	}
	flags = ifr->ifr_flags;
	if (ioctl(sock, SIOCGIFMETRIC, (caddr_t)ifr) < 0) {
		warn("SIOCGIFMETRIC");
		metric = 0;
	} else
		metric = ifr->ifr_metric;
	if (ioctl(sock, SIOCGIFMTU, (caddr_t)ifr) < 0)
		mtu = 0;
	else
		mtu = ifr->ifr_mtu;
	return (0);
}

void printif(struct ifreq *ifrm, int ifaliases)
{
	char *inbuf = NULL;
	struct ifconf ifc;
	struct ifreq ifreq, *ifrp;
	int i, siz, len = 8192;
	int count = 0, noinet = 1;
	char ifrbuf[8192];

	getsock(af);
	if (sock < 0)
		err(1, "socket");
	while (1) {
		ifc.ifc_len = len;
		ifc.ifc_buf = inbuf = realloc(inbuf, len);
		if (inbuf == NULL)
			err(1, "malloc");
		if (ioctl(sock, SIOCGIFCONF, &ifc) < 0)
			err(1, "SIOCGIFCONF");
		if (ifc.ifc_len + sizeof(ifreq) < len)
			break;
		len *= 2;
	}
	ifrp = ifc.ifc_req;
	ifreq.ifr_name[0] = '\0';
	for (i = 0; i < ifc.ifc_len; ) {
		ifrp = (struct ifreq *)((caddr_t)ifc.ifc_req + i);
		memcpy(ifrbuf, ifrp, sizeof(*ifrp));
		siz = ((struct ifreq *)ifrbuf)->ifr_addr.sa_len;
		if (siz < sizeof(ifrp->ifr_addr))
			siz = sizeof(ifrp->ifr_addr);
		siz += sizeof(ifrp->ifr_name);
		i += siz;
		/* avoid alignment issue */
		if (sizeof(ifrbuf) < siz)
			err(1, "ifr too big");
		memcpy(ifrbuf, ifrp, siz);
		ifrp = (struct ifreq *)ifrbuf;

		if (ifrm && strncmp(ifrm->ifr_name, ifrp->ifr_name,
		    sizeof(ifrp->ifr_name)))
			continue;
		strncpy(name, ifrp->ifr_name, sizeof(ifrp->ifr_name));
		if (ifrp->ifr_addr.sa_family == AF_LINK) {
			ifreq = ifr = *ifrp;
			if (getinfo(&ifreq) < 0)
				continue;
			status(1);
			count++;
			noinet = 1;
			continue;
		}	    
		if (!strncmp(ifreq.ifr_name, ifrp->ifr_name,
		    sizeof(ifrp->ifr_name))) {
			const struct afswtch *p;

			if (ifrp->ifr_addr.sa_family == AF_INET &&
			    ifaliases == 0 && noinet == 0)
				continue;
			ifr = *ifrp;
			if ((p = afp) != NULL) {
				if (ifr.ifr_addr.sa_family == p->af_af)
					(*p->af_status)(1);
			} else {
				for (p = afs; p->af_name; p++) {
					if (ifr.ifr_addr.sa_family == p->af_af)
						(*p->af_status)(0);
				}
			}
			count++;
			if (ifrp->ifr_addr.sa_family == AF_INET)
				noinet = 0;
			continue;
		}
	}
	free(inbuf);
	if (count == 0) {
		fprintf(stderr, "%s: no such interface\n", name);
		exit(1);
	}
}

#define RIDADDR 0
#define ADDR	1
#define MASK	2
#define DSTADDR	3

/*ARGSUSED*/
void setifaddr(char *addr, int param)
{
	/*
	 * Delay the ioctl to set the interface addr until flags are all set.
	 * The address interpretation may depend on the flags,
	 * and the flags may change when the address is set.
	 */
	setaddr++;
	newaddr = 1;
	if (doalias == 0)
		clearaddr = 1;
	(*afp->af_getaddr)(addr, (doalias >= 0 ? ADDR : RIDADDR));
}

void setifnetmask(char *addr)
{
	(*afp->af_getaddr)(addr, MASK);
}

/*
 * Note: doing an SIOCIGIFFLAGS scribbles on the union portion
 * of the ifreq structure, which may confuse other parts of ifconfig.
 * Make a private copy so we can avoid that.
 */
void setifflags(char *vname, int value)
{
	struct ifreq my_ifr;

	memcpy((char *)&my_ifr, (char *)&ifr, sizeof(struct ifreq));

	if (ioctl(sock, SIOCGIFFLAGS, (caddr_t)&my_ifr) < 0)
		err(1, "SIOCGIFFLAGS");
	strncpy(my_ifr.ifr_name, name, sizeof(my_ifr.ifr_name));
	flags = my_ifr.ifr_flags;

	if (value < 0) {
		value = -value;
		flags &= ~value;
	} else
		flags |= value;
	my_ifr.ifr_flags = flags;
	if (ioctl(sock, SIOCSIFFLAGS, (caddr_t)&my_ifr) < 0)
		err(1, "SIOCSIFFLAGS");
}

void setifmetric(char *val)
{
	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	ifr.ifr_metric = atoi(val);
	if (ioctl(sock, SIOCSIFMETRIC, (caddr_t)&ifr) < 0)
		warn("SIOCSIFMETRIC");
}

void setifmtu(char *val, int d)
{
	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	ifr.ifr_mtu = atoi(val);
	if (ioctl(sock, SIOCSIFMTU, (caddr_t)&ifr) < 0)
		warn("SIOCSIFMTU");
}


#define SIN(x) ((struct sockaddr_in *) &(x))
struct sockaddr_in *sintab[] = {
SIN(ridreq.ifr_addr), SIN(addreq.ifra_addr),
SIN(addreq.ifra_mask), SIN(addreq.ifra_broadaddr)};

void in_getaddr(char *s, int which)
{
	struct sockaddr_in *sin = sintab[which];
/*
	struct hostent *hp;
	struct netent *np;
*/
	sin->sin_len = sizeof(*sin);
	if (which != MASK)
		sin->sin_family = AF_INET;

	if (inet_aton(s, &sin->sin_addr) == 0) {
/*
XXX - we don't have these yet!

		if ((hp = gethostbyname(s)))
			memcpy(&sin->sin_addr, hp->h_addr, hp->h_length);
		else if ((np = getnetbyname(s)))
			sin->sin_addr = inet_makeaddr(np->n_net, INADDR_ANY);
		else
*/
			errx(1, "%s: bad value", s);
	}
}

void in_status(int force)
{
	struct sockaddr_in *sin, sin2;

	getsock(AF_INET);
	if (sock < 0) {
		if (errno == EPROTONOSUPPORT)
			return;
		err(1, "socket");
	}
	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	sin = (struct sockaddr_in *)&ifr.ifr_addr;

	/*
	 * We keep the interface address and reset it before each
	 * ioctl() so we can get ifaliases information (as opposed
	 * to the primary interface netmask/dstaddr/broadaddr, if
	 * the ifr_addr field is zero).
	 */
	memcpy(&sin2, &ifr.ifr_addr, sizeof(sin2));

	printf("\tinet %s ", inet_ntoa(sin->sin_addr));
	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(sock, SIOCGIFNETMASK, (caddr_t)&ifr) < 0) {
		if (errno != EADDRNOTAVAIL)
			warn("SIOCGIFNETMASK");
		memset(&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
	} else
		netmask.sin_addr =
		    ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
	if (flags & IFF_POINTOPOINT) {
		memcpy(&ifr.ifr_addr, &sin2, sizeof(sin2));
		if (ioctl(sock, SIOCGIFDSTADDR, (caddr_t)&ifr) < 0) {
			if (errno == EADDRNOTAVAIL)
			    memset(&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
			else
			    warn("SIOCGIFDSTADDR");
		}
		strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
		sin = (struct sockaddr_in *)&ifr.ifr_dstaddr;
		printf("--> %s ", inet_ntoa(sin->sin_addr));
	}
	printf("netmask 0x%08lx ", ntohl(netmask.sin_addr.s_addr));
	if (flags & IFF_BROADCAST) {
		memcpy(&ifr.ifr_addr, &sin2, sizeof(sin2));
		if (ioctl(sock, SIOCGIFBRDADDR, (caddr_t)&ifr) < 0) {
			if (errno == EADDRNOTAVAIL)
			    memset(&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
			else
			    warn("SIOCGIFBRDADDR");
		}
		strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
		sin = (struct sockaddr_in *)&ifr.ifr_addr;
		if (sin->sin_addr.s_addr != 0)
			printf("broadcast %s", inet_ntoa(sin->sin_addr));
	}
	putchar('\n');
}
/*
	1  IFF_UP       
	2  IFF_BROADCAST
	3  IFF_PROMISC
	4  IFF_RUNNING     = 0x0008,
	5  IFF_MULTICAST   = 0x0010,
	6  IFF_BROADCAST   = 0x0020,
	7  IFF_POINTOPOINT = 0x0040,
	8  IFF_NOARP       = 0x0080, 
	9  IFF_LOOPBACK	= 0x0100,
	10 IFF_DEBUG       = 0x0200,
	11 IFF_LINK0       = 0x0400,
	12 IFF_LINK1       = 0x0800,
	13 IFF_LINK2       = 0x1000
	
*/
#define	IFFBITS \
"\017\1UP\2BROADCAST\3PROMISC\4RUNNING\5MULTICAST\6BROADCAST\7POINTOPOINT\10NOARP\
\11LOOPBACK\12DEBUG\13SIMPLEX\15LINK0\16LINK1\17LINK2"


/*
 * Print a value a la the %b format of the kernel's printf
 */
static void printb(char *s, uint16 v, char *bits)
{
	int i, any = 0;
	char c;

	if (bits && *bits == 8)
		printf("%s=%o", s, v);
	else
		printf("%s=%x", s, v);
	bits++;
	if (bits) {
		putchar('<');
		while ((i = *bits++)) {
			if (v & (1 << (i-1))) {
				if (any)
					putchar(',');
				any = 1;
				for (; (c = *bits) > 32; bits++)
					putchar(c);
			} else
				for (; *bits > 32; bits++)
					;
		}
		putchar('>');
	}
}

void in_getprefix(char *plen, int which)
{
	struct sockaddr_in *sin = sintab[which];
	u_char *cp;
	int len = strtol(plen, (char **)NULL, 10);

	if ((len < 0) || (len > 32))
		errx(1, "%s: bad value", plen);
	sin->sin_len = sizeof(*sin);
	if (which != MASK)
		sin->sin_family = AF_INET;
	if ((len == 0) || (len == 32)) {
		memset(&sin->sin_addr, 0xff, sizeof(struct in_addr));
		return;
	}
	memset((void *)&sin->sin_addr, 0x00, sizeof(sin->sin_addr));
	for (cp = (u_char *)&sin->sin_addr; len > 7; len -= 8)
		*cp++ = 0xff;
	if (len)
		*cp = 0xff << (8 - len);
}

/*
 * Print the status of the interface.  If an address family was
 * specified, show it and it only; otherwise, show them all.
 */
void status(int link)
{
	const struct afswtch *p = afp;

	printf("%s: ", name);
	printb("flags", flags, IFFBITS);
	if (metric)
		printf(" metric %d", metric);
	if (mtu)
		printf(" mtu %d", mtu);
	putchar('\n');

	if (link == 0) {
		if ((p = afp) != NULL) {
			(*p->af_status)(1);
		} else for (p = afs; p->af_name; p++) {
			ifr.ifr_addr.sa_family = p->af_af;
			(*p->af_status)(0);
		}
	}

//	phys_status(0);
}

