#include <unistd.h>
#include <fcntl.h>
#include "sys/select.h"

#include "poll.h"

// -------------------------------
int poll(struct pollfd * p, int nb, int timeout)
{
	fd_set		read_set;
	fd_set		write_set;
	fd_set		exception_set;
	int			i;
	int			n;
	int			rc;

	FD_ZERO(&read_set);
	FD_ZERO(&write_set);
	FD_ZERO(&exception_set);

	n = -1;
	for(i = 0; i < nb; i++) {
		if (p[i].fd < 0)
			continue;

		if (p[i].events & POLLIN)	FD_SET(p[i].fd, &read_set);
		if (p[i].events & POLLOUT)	FD_SET(p[i].fd, &write_set);
		if (p[i].events & POLLERR)	FD_SET(p[i].fd, &exception_set);

		if (p[i].fd > n)
			n = p[i].fd;
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
		rc = select(n, &read_set, &write_set, &exception_set, &tv);
	};

	if (rc < 0)
		return rc;

	for(i = 0; i < n; i++) { 
		p[i].revents = 0;

		if (FD_ISSET(p[i].fd, &read_set))		p[i].revents |= POLLIN;
		if (FD_ISSET(p[i].fd, &write_set))		p[i].revents |= POLLOUT;
		if (FD_ISSET(p[i].fd, &exception_set))	p[i].revents |= POLLERR;
	};

	return rc;
}

