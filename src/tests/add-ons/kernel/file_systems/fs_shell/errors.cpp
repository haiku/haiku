/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <string.h>

#include "errors.h"

const char *
fs_strerror(int error)
{
	return strerror(to_platform_error(error));
}


#if (defined(__BEOS__) || defined(__HAIKU__))

int
from_platform_error(int error)
{
	return error;
}


int
to_platform_error(int error)
{
	return error;
}


#else

struct error_entry {
	int		fs_error;
	int		platform_error;
};

static error_entry sErrors[] = {
	{ FS_OK, 0 },

	// POSIX errors
	{ FS_E2BIG, E2BIG },
	{ FS_ECHILD, ECHILD },
	{ FS_EDEADLK, EDEADLK },
	{ FS_EFBIG, EFBIG },
	{ FS_EMLINK, EMLINK },
	{ FS_ENFILE, ENFILE },
	{ FS_ENODEV, ENODEV },
	{ FS_ENOLCK, ENOLCK },
	{ FS_ENOSYS, ENOSYS },
	{ FS_ENOTTY, ENOTTY },
	{ FS_ENXIO, ENXIO },
	{ FS_ESPIPE, ESPIPE },
	{ FS_ESRCH, ESRCH },
	#ifdef EFPOS
		{ FS_EFPOS, EFPOS },
	#endif
	#ifdef ESIGPARM
		{ FS_ESIGPARM, ESIGPARM },
	#endif
	{ FS_EDOM, EDOM },
	{ FS_ERANGE, ERANGE },
	{ FS_EPROTOTYPE, EPROTOTYPE },
	{ FS_EPROTONOSUPPORT, EPROTONOSUPPORT },
	{ FS_EPFNOSUPPORT, EPFNOSUPPORT },
	{ FS_EAFNOSUPPORT, EAFNOSUPPORT },
	{ FS_EADDRINUSE, EADDRINUSE },
	{ FS_EADDRNOTAVAIL, EADDRNOTAVAIL },
	{ FS_ENETDOWN, ENETDOWN },
	{ FS_ENETUNREACH, ENETUNREACH },
	{ FS_ENETRESET, ENETRESET },
	{ FS_ECONNABORTED, ECONNABORTED },
	{ FS_ECONNRESET, ECONNRESET },
	{ FS_EISCONN, EISCONN },
	{ FS_ENOTCONN, ENOTCONN },
	{ FS_ESHUTDOWN, ESHUTDOWN },
	{ FS_ECONNREFUSED, ECONNREFUSED },
	{ FS_EHOSTUNREACH, EHOSTUNREACH },
	{ FS_ENOPROTOOPT, ENOPROTOOPT },
	{ FS_ENOBUFS, ENOBUFS },
	{ FS_EINPROGRESS, EINPROGRESS },
	{ FS_EALREADY, EALREADY },
	{ FS_EILSEQ, EILSEQ },
	{ FS_ENOMSG, ENOMSG },
	{ FS_ESTALE, ESTALE },
	{ FS_EOVERFLOW, EOVERFLOW },
	{ FS_EMSGSIZE, EMSGSIZE },
	{ FS_EOPNOTSUPP, EOPNOTSUPP },
	{ FS_ENOTSOCK, ENOTSOCK },
	{ FS_EHOSTDOWN, EHOSTDOWN },
	{ FS_EBADMSG, EBADMSG },
	{ FS_ECANCELED, ECANCELED },
	{ FS_EDESTADDRREQ, EDESTADDRREQ },
	{ FS_EDQUOT, EDQUOT },
	{ FS_EIDRM, EIDRM },
	{ FS_EMULTIHOP, EMULTIHOP },
	#ifdef ENODATA
		{ FS_ENODATA, ENODATA },
	#endif
	{ FS_ENOLINK, ENOLINK },
	#ifdef ENOSR
		{ FS_ENOSR, ENOSR },
	#endif
	#ifdef ENOSTR
		{ FS_ENOSTR, ENOSTR },
	#endif
	{ FS_ENOTSUP, ENOTSUP },
	{ FS_EPROTO, EPROTO },
	#ifdef ETIME
		{ FS_ETIME, ETIME },
	#endif
	{ FS_ETXTBSY, ETXTBSY },

	{ FS_ENOMEM, ENOMEM },
	{ FS_EACCES, EACCES },
	{ FS_EINTR, EINTR },
	{ FS_EIO, EIO },
	{ FS_EBUSY, EBUSY },
	{ FS_EFAULT, EFAULT },
	{ FS_ETIMEDOUT, ETIMEDOUT },
	{ FS_EAGAIN, EAGAIN },
	{ FS_EWOULDBLOCK, EWOULDBLOCK },
	{ FS_EBADF, EBADF },
	{ FS_EEXIST, EEXIST },
	{ FS_EINVAL, EINVAL },
	{ FS_ENAMETOOLONG, ENAMETOOLONG },
	{ FS_ENOENT, ENOENT },
	{ FS_EPERM, EPERM },
	{ FS_ENOTDIR, ENOTDIR },
	{ FS_EISDIR, EISDIR },
	{ FS_ENOTEMPTY, ENOTEMPTY },
	{ FS_ENOSPC, ENOSPC },
	{ FS_EROFS, EROFS },
	{ FS_EMFILE, EMFILE },
	{ FS_EXDEV, EXDEV },
	{ FS_ELOOP, ELOOP },
	{ FS_ENOEXEC, ENOEXEC },
	{ FS_EPIPE, EPIPE },
};
const int sErrorCount = sizeof(sErrors) / sizeof(error_entry);

int
from_platform_error(int error)
{
	if (error == 0)
		return FS_OK;

	for (int i = 0; i < sErrorCount; i++) {
		if (sErrors[i].platform_error == error)
			return sErrors[i].fs_error;
	}
			
	return (error > 0 ? -error : error);
}


int
to_platform_error(int error)
{
	if (error == FS_OK)
		return 0;

	for (int i = 0; i < sErrorCount; i++) {
		if (sErrors[i].fs_error == error)
			return sErrors[i].platform_error;
	}
			
	return error;
}


#endif // ! __BEOS__
