/* socket.c */

/* This is a very simple little shared library that acts as a wrapper
 * for our device/kernel stack!
 */

#include <fcntl.h>
#include <unistd.h>
#include <kernel/OS.h>
#include <iovec.h>
#include <stdio.h>

#include "sys/socket.h"
#include "sys/sockio.h"
#include "netinet/in.h"

#include "net_stack_driver.h"

const char *g_stack_driver_path = "/dev/" NET_STACK_DRIVER_PATH;

bool g_beos_r5_compatibility = false;

struct beosr5_sockaddr_in {
	uint16 sin_family;
	uint16 sin_port;
	uint32 sin_addr;
	char sin_zero[4];
};

static void convert_from_beos_r5_sockaddr(struct sockaddr *to, const struct sockaddr *from);
static void convert_to_beos_r5_sockaddr(struct sockaddr *to, const struct sockaddr *from);
static void	convert_from_beos_r5_sockopt(int *level, int *optnum);

_EXPORT int socket(int family, int type, int protocol)
{
	int sock;
	int rv;
	struct socket_args args;

	sock = open(g_stack_driver_path, O_RDWR);
	if (sock < 0)
		return sock;

	/* work around the old Be header values... *
	 *
	 * This really sucks...
	 *
	 * Basically we've modified the new stack SOCK_ values to be
	 * start at 10, so if it's below that we're being called by
	 * an R5 app.
	 *
	 * NB
	 * Of course this places a lot of restrictions on what we can
	 * do with library replacements and improvements as no other
	 * component of the "team" can be built using the new 
	 * stack.
	 */
	if (type < SOCK_DGRAM) {
		g_beos_r5_compatibility = true;	
		/* we have old be types... convert... */
		if (type == 1)
			type = SOCK_DGRAM;
		else if (type == 2)
			type = SOCK_STREAM;

		if (protocol == 1)
			protocol = IPPROTO_UDP;
		else if (protocol == 2)
			protocol = IPPROTO_TCP;
		else if (protocol == 3)
			protocol = IPPROTO_ICMP;
		
		/* also convert AF_INET */
		if (family == 1)
			family = AF_INET;
	};

	args.family = family;
	args.type = type;
	args.proto = protocol;

	rv = ioctl(sock, NET_STACK_SOCKET, &args, sizeof(args));
	if (rv < 0) {
		close(sock);
		return rv;
	};

	return sock;
}


_EXPORT int bind(int sock, const struct sockaddr *addr, int addrlen)
{
	struct sockaddr temp;
	struct sockaddr_args args;
	
	if (g_beos_r5_compatibility) {
		convert_from_beos_r5_sockaddr(&temp, addr);
		addr = &temp;
		addrlen = sizeof(struct sockaddr_in);
	}
	
	args.addr = (struct sockaddr *) addr;
	args.addrlen = addrlen;
	
	return ioctl(sock, NET_STACK_BIND, &args, sizeof(args));
}


_EXPORT int shutdown(int sock, int how)
{
	struct int_args args;
	
	args.value = how;
	
	return ioctl(sock, NET_STACK_SHUTDOWN, &args, sizeof(args));
}


_EXPORT int connect(int sock, const struct sockaddr *addr, int addrlen)
{
	struct sockaddr temp;
	struct sockaddr_args args;
	
	if (g_beos_r5_compatibility) {
		convert_from_beos_r5_sockaddr(&temp, addr);
		addr = &temp;
		addrlen = sizeof(struct sockaddr_in);
	}

	args.addr = (struct sockaddr *) addr;
	args.addrlen = addrlen;
       	
	return ioctl(sock, NET_STACK_CONNECT, &args, sizeof(args));
}


_EXPORT int listen(int sock, int backlog)
{
	struct int_args args;

	args.value = backlog;
	
	return ioctl(sock, NET_STACK_LISTEN, &args, sizeof(args));
}


_EXPORT int accept(int sock, struct sockaddr *addr, int *addrlen)
{
	struct sockaddr temp;
	struct accept_args args;
	int	rv;
	int	new_sock;
	void *cookie;

	new_sock = open(g_stack_driver_path, O_RDWR);
	if (new_sock < 0)
		return new_sock;
	
	// The network stack driver will need to know to which net_stack_cookie to
	// *bind* with the new accepted socket. He can't know himself find out 
	// the net_stack_cookie of our new_sock file descriptor, the just open() one...
	// So, here, we ask him the net_stack_cookie value for our fd... :-)
	rv = ioctl(new_sock, NET_STACK_GET_COOKIE, &cookie);
	if (rv < 0) {
		close(new_sock);
		return rv;
	}; 

	args.cookie	= cookie; // this way driver can use the right fd/cookie for the new_sock!

	args.addr		= g_beos_r5_compatibility ? &temp : addr;
	args.addrlen	= g_beos_r5_compatibility ? sizeof(temp) : *addrlen;
	
	rv = ioctl(sock, NET_STACK_ACCEPT, &args, sizeof(args));
	if (rv < 0) {
		close(new_sock);
		return rv;
	};

	if (g_beos_r5_compatibility) {
		convert_to_beos_r5_sockaddr(addr, &temp);
		*addrlen = sizeof(struct beosr5_sockaddr_in);
	} else
		*addrlen = args.addrlen;

	return new_sock;
}


_EXPORT ssize_t recvfrom(int sock, void *buffer, size_t buflen, int flags,
             struct sockaddr *addr, size_t *addrlen)
{
	struct sockaddr temp;
	struct msghdr mh;
	struct iovec iov;
	int rv;

	/* XXX - would this be better done as scatter gather? */	
	mh.msg_name = g_beos_r5_compatibility ?  (caddr_t)&temp : (caddr_t)addr;
	mh.msg_namelen = g_beos_r5_compatibility ? sizeof(temp) : addrlen ? *addrlen : 0;
	mh.msg_flags = flags;
	mh.msg_control = NULL;
	mh.msg_controllen = 0;
	iov.iov_base = buffer;
	iov.iov_len = buflen;
	mh.msg_iov = &iov;
	mh.msg_iovlen = 1;

	rv = ioctl(sock, NET_STACK_RECVFROM, &mh, sizeof(mh));
	if (rv < 0)
		return rv;
		
	if (g_beos_r5_compatibility && addr)
		convert_to_beos_r5_sockaddr(addr, &temp);
	
	if (addrlen) {
		if (g_beos_r5_compatibility)
			*addrlen = sizeof(struct beosr5_sockaddr_in);
		else
			*addrlen = mh.msg_namelen;
	}
		
	return rv;
}

_EXPORT ssize_t sendto(int sock, const void *buffer, size_t buflen, int flags,
           const struct sockaddr *addr, size_t addrlen)
{
	struct sockaddr temp;
	struct msghdr mh;
	struct iovec iov;

	if (g_beos_r5_compatibility) {
		convert_from_beos_r5_sockaddr(&temp, addr);
		addr = &temp;
		addrlen = sizeof(struct sockaddr_in);
	}

	/* XXX - would this be better done as scatter gather? */	
	mh.msg_name = (caddr_t)addr;
	mh.msg_namelen = addrlen;
	mh.msg_flags = flags;
	mh.msg_control = NULL;
	mh.msg_controllen = 0;
	iov.iov_base = (caddr_t)buffer;
	iov.iov_len = buflen;
	mh.msg_iov = &iov;
	mh.msg_iovlen = 1;

	return ioctl(sock, NET_STACK_SENDTO, &mh, sizeof(mh));
}


/* These need to be adjusted to take account of the MSG_PEEK
 * flag, but old R5 doesn't use it...
 */
_EXPORT ssize_t recv(int sock, void *data, size_t datalen, int flags)
{
	struct data_xfer_args args;
	int	rv;
	
	args.data = data;
	args.datalen = datalen;
	args.flags = flags;
	args.addr = NULL;
	args.addrlen = 0;
	
	rv = ioctl(sock, NET_STACK_RECV, &args, sizeof(args));
	if (rv < 0)
		return rv;
		
	return args.datalen;
}


_EXPORT ssize_t send(int sock, const void *data, size_t datalen, int flags)
{
	struct data_xfer_args args;
	int	rv;
	
	args.data = (void *) data;
	args.datalen = datalen;
	args.flags = flags;
	args.addr = NULL;
	args.addrlen = 0;
	
	rv = ioctl(sock, NET_STACK_SEND, &args, sizeof(args));
	if (rv < 0)
		return rv;
		
	return args.datalen;
}


_EXPORT int getsockopt(int sock, int level, int option, void *optval, size_t *optlen)
{
	struct sockopt_args args;
	int rv;

	if (g_beos_r5_compatibility && option == 5) { // BeOS R5 SO_FIONREAD
		status_t rv;
		int temp;
		rv = ioctl(sock, FIONREAD, &temp);
		*(int*) optval = temp;
		*optlen = sizeof(temp); 
		return rv;
	};

	if (g_beos_r5_compatibility)
		convert_from_beos_r5_sockopt(&level, &option);

	args.level = level;
	args.option = option;
	args.optval = optval;
	args.optlen = *optlen;
	
	rv = ioctl(sock, NET_STACK_GETSOCKOPT, &args, sizeof(args));
	if (rv ==0)
		*optlen = args.optlen;
	return rv;
}

_EXPORT int setsockopt(int sock, int level, int option, const void *optval, size_t optlen)
{
	struct sockopt_args args;
	
	/* BeOS R5 uses 
	 * setsockopt(sock, SOL_SOCKET, SO_NONBLOCK, &i, sizeof(i));
	 * Try to capture that here and the do the correct thing.
	 */
	if (g_beos_r5_compatibility && option == 3) {
		int temp = *(int*) optval;
		return ioctl(sock, FIONBIO, &temp);
	}

	if (g_beos_r5_compatibility)
		convert_from_beos_r5_sockopt(&level, &option);

	args.level = level;
	args.option = option;
	args.optval = (void *) optval;
	args.optlen = optlen;
	
	return ioctl(sock, NET_STACK_SETSOCKOPT, &args, sizeof(args));
}

_EXPORT int getpeername(int sock, struct sockaddr *addr, int *addrlen)
{
	struct sockaddr temp;
	struct sockaddr_args args;
	int rv;
	
	args.addr = g_beos_r5_compatibility ? &temp : addr;
	args.addrlen = g_beos_r5_compatibility ? sizeof(temp) : *addrlen;
	
	rv = ioctl(sock, NET_STACK_GETPEERNAME, &args, sizeof(args));
	if (rv < 0)
		return rv;
		
	if (g_beos_r5_compatibility) {
		convert_to_beos_r5_sockaddr(addr, &temp);
		*addrlen = sizeof(struct beosr5_sockaddr_in);
	} else
		*addrlen = args.addrlen;

	return rv;
}

_EXPORT int getsockname(int sock, struct sockaddr *addr, int *addrlen)
{
	struct sockaddr temp;
	struct sockaddr_args args;
	int rv;
	
	args.addr = g_beos_r5_compatibility ? &temp : addr;
	args.addrlen = g_beos_r5_compatibility ? sizeof(temp) : *addrlen;
	
	rv = ioctl(sock, NET_STACK_GETSOCKNAME, &args, sizeof(args));
	if (rv < 0)
		return rv;
		
	if (g_beos_r5_compatibility) {
		convert_to_beos_r5_sockaddr(addr, &temp);
		*addrlen = sizeof(struct beosr5_sockaddr_in);
	} else
		*addrlen = args.addrlen;

	return rv;
}


_EXPORT int sysctl (int *name, uint namelen, void *oldp, size_t *oldlenp, 
            void *newp, size_t newlen)
{
	int s;
	struct sysctl_args sa;
	int rv;
	
	s = socket(PF_ROUTE, SOCK_RAW, 0);
	if (s < 0)
		return s;
		
	sa.name = name;
	sa.namelen = namelen;
	sa.oldp = oldp;
	sa.oldlenp = oldlenp;
	sa.newp = newp;
	sa.newlen = newlen;
	
	rv = ioctl(s, NET_STACK_SYSCTL, &sa, sizeof(sa));

	close(s);
	
	return rv;
}
	
/* 
 * Private routines
 * ----------------
 */

static void convert_from_beos_r5_sockaddr(struct sockaddr *_to, const struct sockaddr *_from)
{
	const struct beosr5_sockaddr_in *from = (struct beosr5_sockaddr_in *)_from;
	struct sockaddr_in *to = (struct sockaddr_in *)_to;
	memset(to, 0, sizeof(*to));
	to->sin_len = sizeof(*to);
	if (from->sin_family == 1)
		to->sin_family = AF_INET;
	else
		to->sin_family = from->sin_family;
	to->sin_port = from->sin_port;
	to->sin_addr.s_addr = from->sin_addr;
}

static void convert_to_beos_r5_sockaddr(struct sockaddr *_to, const struct sockaddr *_from)
{
	const struct sockaddr_in *from = (struct sockaddr_in *)_from;
	struct beosr5_sockaddr_in *to = (struct beosr5_sockaddr_in *)_to;
	memset(to, 0, sizeof(*to));
	if (from->sin_family == AF_INET)
		to->sin_family = 1;
	else
		to->sin_family = from->sin_family;
	to->sin_port = from->sin_port;
	to->sin_addr = from->sin_addr.s_addr;
}

static void	convert_from_beos_r5_sockopt(int *level, int *optnum)
{
	if (*level == 1)
		*level = SOL_SOCKET;
		
	switch (*optnum) {
		case 1: 
			*optnum = SO_DEBUG; 
			break;
		case 2: 
			*optnum = SO_REUSEADDR; 
			break;
/*
		case 3: 
			*optnum = SO_NONBLOCK; 
			break;
*/
		case 4: 
			*optnum = SO_REUSEPORT; 
			break;
/*
		case 5: 
			*optnum = SO_FIONREAD; 
			break;
*/
	};
}
