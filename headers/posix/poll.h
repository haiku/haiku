#ifndef _POLL_H
#define _POLL_H
/* 
** Distributed under the terms of the OpenBeOS License.
*/

typedef unsigned long nfds_t;

struct pollfd {
	int fd;
	int events;		/* events to look for */
	int revents;	/* events that occured */
};

/* events & revents */
#define	POLLIN		0x0001		/* any readable data available */
#define	POLLPRI		0x0002		/* high priority readable data */
#define	POLLOUT		0x0004		/* file descriptor is writeable */
#define	POLLRDNORM	0x0040		/* normal data available */
#define	POLLRDBAND	0x0080		/* priority readable data */
#define	POLLWRNORM	POLLOUT
#define	POLLWRBAND	0x0100		/* priority data can be written */

/* revents only */
#define	POLLERR		0x0008		/* errors pending */
#define	POLLHUP		0x0010		/* disconnected */
#define	POLLNVAL	0x0020		/* invalid file descriptor */


extern
#ifdef __cplusplus
"C"
#endif
int poll(struct pollfd *fds, nfds_t numfds, int timeout);

#endif /* _POLL_H */
