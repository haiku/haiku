/* socket.c */

/* This is a very simple little shared library that acts as a wrapper
 * for our device/kernel stack!
 */

#include <fcntl.h>
#include <unistd.h>
#include <kernel/OS.h>
#include <iovec.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/sockio.h>
#include <netinet/in.h>

#include "net_stack_driver.h"

// Forward declaration of sysctl.h, because it's currently declared under headers/private/kernel/sysctl.h
int sysctl(int *, uint, void *, size_t *, void *, size_t);

static char *	stack_driver_path(void);


_EXPORT int socket(int family, int type, int protocol)
{
	int sock;
	int rv;
	struct stack_driver_args args;
	
	sock = open(stack_driver_path(), O_RDWR);
	if (sock < 0)
		return sock;
	
	args.u.socket.family = family;
	args.u.socket.type = type;
	args.u.socket.proto = protocol;
	
	rv = ioctl(sock, NET_STACK_SOCKET, &args, sizeof(args));
	if (rv < 0) {
		close(sock);
		return rv;
	}
	
	return sock;
}


_EXPORT int bind(int sock, const struct sockaddr *addr, int addrlen)
{
	struct stack_driver_args args;
	
	args.u.sockaddr.addr = (struct sockaddr *) addr;
	args.u.sockaddr.addrlen = addrlen;
	
	return ioctl(sock, NET_STACK_BIND, &args, sizeof(args));
}


_EXPORT int shutdown(int sock, int how)
{
	struct stack_driver_args args;
	
	args.u.integer = how;
	
	return ioctl(sock, NET_STACK_SHUTDOWN, &args, sizeof(args));
}


_EXPORT int connect(int sock, const struct sockaddr *addr, int addrlen)
{
	struct stack_driver_args args;
	
	args.u.sockaddr.addr = (struct sockaddr *) addr;
	args.u.sockaddr.addrlen = addrlen;
	
	return ioctl(sock, NET_STACK_CONNECT, &args, sizeof(args));
}


_EXPORT int listen(int sock, int backlog)
{
	struct stack_driver_args args;

	args.u.integer = backlog;
	
	return ioctl(sock, NET_STACK_LISTEN, &args, sizeof(args));
}


_EXPORT int accept(int sock, struct sockaddr *addr, int *addrlen)
{
	struct stack_driver_args args;
	int rv;
	int new_sock;
	void *cookie;
	
	new_sock = open(stack_driver_path(), O_RDWR);
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
	}
	
	args.u.accept.cookie     = cookie; // this way driver can use the right fd/cookie for the new_sock! 		
	
	args.u.accept.addr = addr;
	args.u.accept.addrlen = addrlen ? *addrlen : 0;
	
	rv = ioctl(sock, NET_STACK_ACCEPT, &args, sizeof(args));
	if (rv < 0) { 
		close(new_sock); 
		return rv; 
	}
	
	*addrlen = args.u.accept.addrlen;
	
	return new_sock;
}


_EXPORT ssize_t recvfrom(int sock, void *buffer, size_t buflen, int flags,
             struct sockaddr *addr, int *addrlen)
{
	struct msghdr mh;
	struct iovec iov;
	int rv;
	
	/* XXX - would this be better done as scatter gather? */	
	mh.msg_name = (caddr_t)addr;
	mh.msg_namelen = addrlen ? *addrlen : 0;
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
	
	if (addrlen)
		*addrlen = mh.msg_namelen;
	
	return rv;
}

_EXPORT ssize_t sendto(int sock, const void *buffer, size_t buflen, int flags,
           const struct sockaddr *addr, int addrlen)
{
	struct msghdr mh;
	struct iovec iov;
	
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
	struct stack_driver_args args;
	int	rv;
	
	args.u.transfer.data = data;
	args.u.transfer.datalen = datalen;
	args.u.transfer.flags = flags;
	args.u.transfer.addr = NULL;
	args.u.transfer.addrlen = 0;
	
	rv = ioctl(sock, NET_STACK_RECV, &args, sizeof(args));
	if (rv < 0)
		return rv;
		
	return args.u.transfer.datalen;
}


_EXPORT ssize_t send(int sock, const void *data, size_t datalen, int flags)
{
	struct stack_driver_args args;
	int	rv;
	
	args.u.transfer.data = (void *) data;
	args.u.transfer.datalen = datalen;
	args.u.transfer.flags = flags;
	args.u.transfer.addr = NULL;
	args.u.transfer.addrlen = 0;
	
	rv = ioctl(sock, NET_STACK_SEND, &args, sizeof(args));
	if (rv < 0)
		return rv;
		
	return args.u.transfer.datalen;
}


_EXPORT int getsockopt(int sock, int level, int option, void *optval, size_t *optlen)
{
	struct stack_driver_args args;
	int rv;
	
	args.u.sockopt.level = level;
	args.u.sockopt.option = option;
	args.u.sockopt.optval = optval;
	args.u.sockopt.optlen = *optlen;
	
	rv = ioctl(sock, NET_STACK_GETSOCKOPT, &args, sizeof(args));
	if (rv ==0)
		*optlen = args.u.sockopt.optlen;
	
	return rv;
}


_EXPORT int setsockopt(int sock, int level, int option, const void *optval, size_t optlen)
{
	struct stack_driver_args args;
	
	args.u.sockopt.level = level;
	args.u.sockopt.option = option;
	args.u.sockopt.optval = (void *) optval;
	args.u.sockopt.optlen = optlen;
	
	return ioctl(sock, NET_STACK_SETSOCKOPT, &args, sizeof(args));
}


_EXPORT int getpeername(int sock, struct sockaddr *addr, int *addrlen)
{
	struct stack_driver_args args;
	int rv;
	
	args.u.sockaddr.addr = addr;
	args.u.sockaddr.addrlen = *addrlen;
	
	rv = ioctl(sock, NET_STACK_GETPEERNAME, &args, sizeof(args));
	if (rv < 0)
		return rv;
	
	*addrlen = args.u.sockaddr.addrlen;
	
	return rv;
}

_EXPORT int getsockname(int sock, struct sockaddr *addr, int *addrlen)
{
	struct stack_driver_args args;
	int rv;
	
	args.u.sockaddr.addr = addr;
	args.u.sockaddr.addrlen = *addrlen;
	
	rv = ioctl(sock, NET_STACK_GETSOCKNAME, &args, sizeof(args));
	if (rv < 0)
		return rv;
	
	*addrlen = args.u.sockaddr.addrlen;
	
	return rv;
}


_EXPORT int socketpair(int family, int type, int protocol, int socket_vector[2])
{
	struct stack_driver_args args;
	int rv;
	void *cookie;

	socket_vector[0] = socket(family, type, protocol);
	if (socket_vector[0] < 0) {
		return socket_vector[0];
	}

	socket_vector[1] = socket(family, type, protocol);
	if (socket_vector[1] < 0) {
		close(socket_vector[0]);
		return socket_vector[1];
	}

	// The network stack driver will need to know to which net_stack_cookie to 
	// *bind* with the new accepted socket. He can't know himself find out 
	// the net_stack_cookie of our new_sock file descriptor, the just open() one... 
	// So, here, we ask him the net_stack_cookie value for our fd... :-) 
	rv = ioctl(socket_vector[1], NET_STACK_GET_COOKIE, &cookie); 
	if (rv < 0) {
		close(socket_vector[0]);
		close(socket_vector[1]); 
		return rv;
	}
	
	args.u.socketpair.cookie     = cookie; // this way driver can use the right fd/cookie! 	

	rv = ioctl(socket_vector[0], NET_STACK_SOCKETPAIR, &args, sizeof(args));
	if (rv < 0) {
		close(socket_vector[0]);
		close(socket_vector[1]);
		return rv;
	}
	return rv;
}


// #pragma mark -


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
	
// #pragma mark -
	
/* 
 * Private routines
 * ----------------
 */

static char * stack_driver_path(void) {
  char * path;

  // user-defined stack driver path?
  path = getenv("NET_STACK_DRIVER_PATH");
  if (!path)
    path = NET_STACK_DRIVER_PATH;
  printf("net-stack-driver-path = %s\n", path);
  return path;
}
