/* poll.h
 * 
 * poll is an alternative way to deal with system notification
 * of events on sockets
 */

#ifndef __POLL_H
#define __POLL_H

#ifdef __cplusplus
extern "C" {
#endif

#define POLLIN   0x0001
#define POLLPRI  0x0002   /* not used */
#define POLLOUT  0x0004
#define POLLERR  0x0008

struct pollfd {
	int fd;
	int events;	/* events to look for */
	int revents;	/* events that occured */
};

extern int poll(struct pollfd * p, int nb, int timeout);

#ifdef __cplusplus
}
#endif

#endif /* __POLL_H */

