#include <unistd.h>
#include <fcntl.h>
#include "sys/select.h"

#include "poll.h"

// -------------------------------
int poll(struct pollfd *fds, nfds_t numfds, int timeout)
{
	fd_set		read_set;
	fd_set		write_set;
	fd_set		exception_set;
	nfds_t		i;
	int			n;
	int			rc;

	FD_ZERO(&read_set);
	FD_ZERO(&write_set);
	FD_ZERO(&exception_set);

	n = -1;
	for(i = 0; i < numfds; i++) {
		if (fds[i].fd < 0)
			continue;

		if (fds[i].events & POLLIN)		FD_SET(fds[i].fd, &read_set);
		if (fds[i].events & POLLOUT)	FD_SET(fds[i].fd, &write_set);
		if (fds[i].events & POLLERR)	FD_SET(fds[i].fd, &exception_set);

		if (fds[i].fd > n)
			n = fds[i].fd;
	};

	if (n == -1)
		// Hey!? Nothing to poll, in fact!!!
		return 0;
	
	if (timeout < 0)
		rc = select(n+1, &read_set, &write_set, &exception_set, NULL);
	else {
		struct timeval	tv;

		tv.tv_sec 	= timeout / 1000;
		tv.tv_usec	= 1000 * (timeout % 1000);
		rc = select(n+1, &read_set, &write_set, &exception_set, &tv);
	};

	if (rc < 0)
		return rc;

	for(i = 0; i < (nfds_t) n; i++) { 
		fds[i].revents = 0;

		if (FD_ISSET(fds[i].fd, &read_set))			fds[i].revents |= POLLIN;
		if (FD_ISSET(fds[i].fd, &write_set))		fds[i].revents |= POLLOUT;
		if (FD_ISSET(fds[i].fd, &exception_set))	fds[i].revents |= POLLERR;
	};

	return rc;
}

