/*
 * errno.h
 *
 * POSIX style error codes.
 * Be values used where required to keep compatibility.
 */

#ifndef _POSIX_ERRNO_H
#define _POSIX_ERRNO_H

#ifdef __cplusplus
extern "C"
{
#endif  

/* XXX - this really belongs in limits.h, but as we don't have one
 * yet it's here so we can build.
 * XXX - move me!
 */
/* Minimum's */
#ifndef LONG_MIN
#define LONG_MIN          (-2147483647L-1)
#endif

/* XXX - Fix this once TLS works */
extern int errno;

/* We're defining values based on the values that Be used, hence
 * the starting point at + 0x7000
 */
#define POSIX_ERROR_START    LONG_MIN + 0x7000

/* The basic error codes that don't have a B_ equivalent
 *
 * NB when adding codes make sure that the Be Errors.h didn't
 *    define a value or it needs to go in the bottom section and
 *    have an identical value assigned.
 * NB new values just go on the bottom.
 */
enum {
	E2BIG = POSIX_ERROR_START + 1,
	ECHILD,
	EDEADLK,
	EFBIG,
	EMLINK,
	ENFILE,
	ENODEV,
	ENOLCK,
	ENOSYS,
	ENOTTY,              /* +10 */
	ENXIO,
	ESPIPE,
	ESRCH,
	EFPOS,
	ESIGPARM,
	EDOM,
	ERANGE,
	EPROTOTYPE,
	EPROTONOSUPPORT,
	EPFNOSUPPORT,        /* +20 */
	EAFNOSUPPORT,
	EADDRINUSE,
	EADDRNOTAVAIL,
	ENETDOWN,
	ENETUNREACH,
	ENETRESET,
	ECONNABORTED,
	ECONNRESET,
	EISCONN,
	ENOTCONN,            /* +30 */
	ESHUTDOWN,
	ECONNREFUSED,
	EHOSTUNREACH,
	ENOPROTOOPT,
	ENOBUFS,
	EINPROGRESS,
	EALREADY,
	EILSEQ,
	ENOMSG,
	ESTALE,              /* +40 */
	EOVERFLOW,
	EMSGSIZE,
	EOPNOTSUPP,
	ENOTSOCK,
	
	/* These are additional to the Be ones */
	EBADMSG,
	ECANCELLED,
	EDESTADDRREQ,
	EDQUOT,
	EHOSTDOWN,
	EIDRM,               /* +50 */
	EISDIR,
	EMULTIHOP,
	ENODATA,
	ENOLINK,
	ENOSR,
	ENOSTR,
	EPROTO,
	EPROTOOPT,
	ETIME,
	EFTYPE
};

/*
 * Errors with Be equivalents
 */

/* TODO: add comments showing what the equivalent Be errors are... */

/* NB - These have strange values to be the same as the Be values... */

/* General errors */
enum {
	ENOMEM = LONG_MIN,
	EIO,
	EACCES,
	EINVAL = LONG_MIN + 5,
	ETIMEDOUT = LONG_MIN + 9,
	EINTR,
	EWOULDBLOCK,
	EAGAIN = EWOULDBLOCK,
	EBUSY = LONG_MIN + 14,
	EPERM
};

/* Storage kit/File system errors */
enum {
	EBADF = LONG_MIN + 0x6000,
	EEXIST = EBADF + 2,
	ENOENT,
	ENAMETOOLONG,
	ENOTDIR,
	ENOTEMPTY,
	ENOSPC,
	EROFS,
	EMFILE,
	EXDEV,
	ELOOP,
	EPIPE
};

#define ENOEXEC   LONG_MIN + 0x1302

#ifdef __cplusplus
} /* "C" */
#endif 

#endif /* _POSIX_ERRNO_H */
