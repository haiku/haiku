#ifndef _CORE_PRIVATE__H
#define _CORE_PRIVATE__H


struct iovec;

enum    uio_rw { UIO_READ, UIO_WRITE };

/* Segment flag values. */
enum uio_seg {
	UIO_USERSPACE,		/* from user data space */
	UIO_SYSSPACE		/* from system space */
};

struct uio {
	struct  iovec *uio_iov;		/* pointer to array of iovecs */
	int     uio_iovcnt;			/* number of iovecs in array */
	off_t   uio_offset;			/* offset into file this uio corresponds to */
	size_t  uio_resid;			/* residual i/o count */
	enum    uio_seg uio_segflg;	/* see above */
	enum    uio_rw uio_rw;		/* see above */
//	struct  proc *uio_procp;	/* process if UIO_USERSPACE */
};

struct ifnet;


int uiomove(char *cp, int n, struct uio *uio);

void in_if_detach(struct ifnet *ifp);
	// this removes all IP related references for this interface (route, address)

extern struct pool_ctl *mbpool;
extern struct pool_ctl *clpool;

extern int max_hdr;		/* largest link+protocol header */
extern int max_linkhdr;		/* largest link level header */
extern int max_protohdr;		/* largest protocol header */

#endif
