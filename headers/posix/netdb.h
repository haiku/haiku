/* netdb.h */

#ifndef NETDB_H
#define NETDB_H

#include <errno.h>

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

#define HOST_NOT_FOUND 1
#define TRY_AGAIN 2
#define NO_RECOVERY 3
#define NO_DATA 4

#ifndef h_errno
extern int h_errno;
extern int *_h_errnop(void);
#define h_errno (*_h_errnop())
#endif /* h_errno */

struct hostent {
	char *h_name;
	char **h_aliases;
	int h_addrtype;
	int h_length;
	char **h_addr_list;
};
#define h_addr h_addr_list[0]

struct servent {
	char *s_name;
	char **s_aliases;
	int s_port;
	char *s_proto;
};	

/*
 * Assumption here is that a network number
 * fits in an in_addr_t -- probably a poor one.
 */
struct  netent {
        char            *n_name;        /* official name of net */
        char            **n_aliases;    /* alias list */
        int             n_addrtype;     /* net address type */
        in_addr_t       n_net;          /* network # */
};

struct  protoent {
        char    *p_name;        /* official protocol name */
        char    **p_aliases;    /* alias list */
        int     p_proto;        /* protocol # */
};

struct addrinfo {
	int ai_flags;               /* input flags */
	int ai_family;              /* protocol family for socket */
	int ai_socktype;            /* socket type */
	int ai_protocol;            /* protocol for socket */
	socklen_t ai_addrlen;       /* length of socket-address */
	struct sockaddr *ai_addr;   /* socket-address for socket */
	char *ai_canonname;         /* canonical name for service location (iff req) */
	struct addrinfo *ai_next;   /* pointer to next in list */
};


struct hostent *gethostbyname(const char *hostname);
struct hostent *gethostbyaddr(const char *hostname, int len, int type);
struct servent *getservbyname(const char *name, const char *proto);
void herror(const char *);
unsigned long inet_addr(const char *a_addr);
char *inet_ntoa(struct in_addr addr);

int gethostname(char *hostname, size_t hostlen);

/* BE specific, because of lack of UNIX passwd functions */
int getusername(char *username, size_t userlen);
int getpassword(char *password, size_t passlen);


/* These are new! */
struct netent   *getnetbyaddr (in_addr_t, int);
struct netent   *getnetbyname (const char *);
struct netent   *getnetent (void);
struct protoent *getprotoent (void);
struct protoent *getprotobyname (const char *);
struct protoent *getprotobynumber (int);
struct hostent  *gethostbyname2 (const char *, int);
struct servent  *getservbyport (int, const char *);

int              getaddrinfo (const char *, const char *,
                              const struct addrinfo *, 
                              struct addrinfo **);
void             freeaddrinfo (struct addrinfo *);
int              getnameinfo (const struct sockaddr *, socklen_t,
                              char *, size_t, char *, size_t,
                              int);

void sethostent (int);
void setnetent (int);
void setprotoent (int);
void setservent (int);    

void endhostent (void);
void endnetent (void);
void endprotoent (void);
void endservent (void);  

#define _PATH_HEQUIV    "/etc/hosts.equiv"
#define _PATH_HOSTS     "/etc/hosts"
#define _PATH_NETWORKS  "/etc/networks"
#define _PATH_PROTOCOLS "/etc/protocols"
#define _PATH_SERVICES  "/etc/services"

/*
 * Error return codes from gethostbyname() and gethostbyaddr()
 * (left in extern int h_errno).
 */

#define NETDB_INTERNAL  -1      /* see errno */
#define NETDB_SUCCESS   0       /* no problem */
#define HOST_NOT_FOUND  1       /* Authoritative Answer Host not found */
#define TRY_AGAIN       2       /* Non-Authoritive Host not found, or SERVERFAIL */
#define NO_RECOVERY     3       /* Non recoverable errors, FORMERR, REFUSED, NOTIMP */
#define NO_DATA         4       /* Valid name, no data record of requested type */
#define NO_ADDRESS      NO_DATA /* no address, look for MX record */

/* Values for getaddrinfo() and getnameinfo() */
#define AI_PASSIVE      1       /* socket address is intended for bind() */
#define AI_CANONNAME    2       /* request for canonical name */
#define AI_NUMERICHOST  4       /* don't ever try nameservice */
#define AI_EXT          8       /* enable non-portable extensions */
/* valid flags for addrinfo */
#define AI_MASK         (AI_PASSIVE | AI_CANONNAME | AI_NUMERICHOST)

#define NI_NUMERICHOST  1       /* return the host address, not the name */
#define NI_NUMERICSERV  2       /* return the service address, not the name */
#define NI_NOFQDN       4       /* return a short name if in the local domain */
#define NI_NAMEREQD     8       /* fail if either host or service name is unknown */
#define NI_DGRAM        16      /* look up datagram service instead of stream */
#define NI_WITHSCOPEID  32      /* KAME hack: attach scopeid to host portion */

#define NI_MAXHOST      MAXHOSTNAMELEN  /* max host name returned by getnameinfo */
#define NI_MAXSERV      32      /* max serv. name length returned by getnameinfo */

/*
 * Scope delimit character (KAME hack)
 */
#define SCOPE_DELIMITER '%'

#define EAI_BADFLAGS    -1      /* invalid value for ai_flags */
#define EAI_NONAME      -2      /* name or service is not known */
#define EAI_AGAIN       -3      /* temporary failure in name resolution */
#define EAI_FAIL        -4      /* non-recoverable failure in name resolution */
#define EAI_NODATA      -5      /* no address associated with name */
#define EAI_FAMILY      -6      /* ai_family not supported */
#define EAI_SOCKTYPE    -7      /* ai_socktype not supported */
#define EAI_SERVICE     -8      /* service not supported for ai_socktype */
#define EAI_ADDRFAMILY  -9      /* address family for name not supported */
#define EAI_MEMORY      -10     /* memory allocation failure */
#define EAI_SYSTEM      -11     /* system error (code indicated in errno) */
#define EAI_BADHINTS    -12     /* invalid value for hints */
#define EAI_PROTOCOL    -13     /* resolved protocol is unknown */


/*
 * Flags for getrrsetbyname()
 */
#define RRSET_VALIDATED         1

/*
 * Return codes for getrrsetbyname()
 */
#define ERRSET_SUCCESS          0

#endif /* NETDB_H */
