#include <unistd.h>
#include <image.h>
#include <OS.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <errno.h>

#include "net_stack_driver.h"


#ifndef BONE_VERSION

static int fd_set_count(fd_set * bits, int nbits)
{
	int count;
	int fd;
	
	count = 0;
	for (fd = 0; fd < nbits; fd++)
		if (FD_ISSET(fd, bits))
			count++;

	return count;
}


_EXPORT int select(int nbits, struct fd_set * rbits, 
                      struct fd_set * wbits, 
                      struct fd_set * ebits, 
                      struct timeval * tv)
{
	int fd;
	int n;
	struct stack_driver_args args;
	status_t status;
	area_id area;
	struct r5_selectsync * rss;
	
#define ROUND_TO_PAGE_SIZE(x) (((x) + (B_PAGE_SIZE) - 1) & ~((B_PAGE_SIZE) - 1))

	// We share a area with the fd_set on it with the network stack code.
	// The r5_notify_select_event in the net_stack_driver code will fill it and wakeup us
	// when a event raise.

	area = create_area("r5_selectsync", (void **) &rss,
		B_ANY_ADDRESS, ROUND_TO_PAGE_SIZE(sizeof(*rss)), B_NO_LOCK, 
		B_READ_AREA | B_WRITE_AREA);
	if (area < B_OK) {
		errno = area;
		return -1;
	};
	
	// The area is multi-thread protected by a semaphore locker 
	rss->lock = create_sem(1, "r5_selectsync_lock");
	if (rss->lock < B_OK) {
		errno = rss->lock;
		rss->wakeup = 0;
		n = -1;
		goto error;
	};

	// Create the wakeup call semaphore
	rss->wakeup = create_sem(0, "r5_selectsync_wakeup");
	if (rss->wakeup < B_OK) {
		errno = rss->wakeup;
		n = -1;
		goto error;
	};
	
	FD_ZERO(&rss->rbits);
	FD_ZERO(&rss->wbits);
	FD_ZERO(&rss->ebits);
	
	/* Beware, ugly hacky hack: we pass the area_id of our shared r5_selectsync,
	   that net_stack_driver should udate directly
	*/
	args.u.select.sync = (struct selectsync *) area;

	/* call, indirectly, net_stack_select() for each event to monitor
	 * as we are not the vfs, we can't call this device hook ourself, but 
	 * our NET_STACK_SELECT opcode would do it for us...
	 */
	n = 0;
	for (fd = 0; fd < nbits; fd++) {
		if (rbits && FD_ISSET(fd, rbits)) {
			args.u.select.ref = (fd << 8) | 1;
			if (ioctl(fd, NET_STACK_SELECT, &args, sizeof(args)) >= 0)
				n++;
    	};
		if (wbits && FD_ISSET(fd, wbits)) {
			args.u.select.ref = (fd << 8) | 2;
			if (ioctl(fd, NET_STACK_SELECT, &args, sizeof(args)) >= 0)
				n++;
    	};
		if (ebits && FD_ISSET(fd, ebits)) {
			args.u.select.ref = (fd << 8) | 3;
			if (ioctl(fd, NET_STACK_SELECT, &args, sizeof(args)) >= 0)
				n++;
    	};
	}
	
	if (n < 1) {
		// TODO: maybe we could hack a workaround for select() on file/dev here:
		// If no fd handle the NET_STACK_SELECT, assume they're files, so always
		// readable and writable?
		errno = B_BAD_VALUE;
		n = -1;
		goto error;
	};

	// Okay, now wait for our wakeup call.
	// Maybe his's already there, even... (when the select event is already raised)
	if (tv) {
		bigtime_t timeout;

		timeout = tv->tv_sec * 1000000LL + tv->tv_usec;
 		status = acquire_sem_etc(rss->wakeup, 1, B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, timeout);
	} else
		status = acquire_sem(rss->wakeup);  

	// we don't need it anymore, and we say "select() party is over" that way, too :-)
	delete_sem(rss->lock);
	delete_sem(rss->wakeup);
	rss->lock = rss->wakeup = -1;
	
	// unregister socket event notification
 	for(fd = 0; fd < nbits; fd++) {
		if (rbits && FD_ISSET(fd, rbits)) {
			args.u.select.ref = (fd << 8) | 1;
       		ioctl(fd, NET_STACK_DESELECT, &args, sizeof(args));
    	};
		if (wbits && FD_ISSET(fd, wbits)) {
			args.u.select.ref = (fd << 8) | 2;
			ioctl(fd, NET_STACK_DESELECT, &args, sizeof(args));
    	};
		if (ebits && FD_ISSET(fd, ebits)) {
			args.u.select.ref = (fd << 8) | 3;
			ioctl(fd, NET_STACK_DESELECT, &args, sizeof(args));
		};
	};

	// Okay, what the final result?
	switch (status) {
	case B_OK:
	case B_WOULD_BLOCK:
	case B_TIMED_OUT:	// logicly, 'n' should stay to 0 in this case...
		n = 0;
		// IMPORTANT: memcpy uses the old R5 fd_bits size (sizeof(unsigned) * 8)!
		if (rbits) {
			memcpy(rbits, &rss->rbits, sizeof(unsigned) * 8);
			n += fd_set_count(rbits, nbits);
		};
		if (wbits) {
			memcpy(wbits, &rss->wbits, sizeof(unsigned) * 8);
			n += fd_set_count(wbits, nbits);
		};
		if (ebits) {
			memcpy(ebits, &rss->ebits, sizeof(unsigned) * 8);
			n += fd_set_count(ebits, nbits);
		};
		break;
	
	default:
		errno = status;
		n = -1;
		break;
	};	

error:
	if (rss->lock >= 0)
		delete_sem(rss->lock);
	if (rss->wakeup >= 0)
		delete_sem(rss->wakeup);
		
	delete_area(area);
	return n;
}

#endif
