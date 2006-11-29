

#define BUILDING_HAIKU_ERROR_MAPPER 1
#include <BeOSBuildCompatibility.h>

#include <errno.h>
#include <string.h>

#include <map>

using namespace std;

static map<int, int> sToHaikuErrorMap;
static map<int, int> sToHostErrorMap;
static bool sErrorMapsInitialized = false;

// init_error_map
static void
init_error_map()
{
	if (sErrorMapsInitialized)
		return;

	#define ADD_ERROR(error) \
		sToHaikuErrorMap[error] = HAIKU_##error; \
		sToHostErrorMap[HAIKU_##error] = error;

	ADD_ERROR(E2BIG);
	ADD_ERROR(ECHILD);
	ADD_ERROR(EDEADLK);
	ADD_ERROR(EFBIG);
	ADD_ERROR(EMLINK);
	ADD_ERROR(ENFILE);
	ADD_ERROR(ENODEV);
	ADD_ERROR(ENOLCK);
	ADD_ERROR(ENOSYS);
	ADD_ERROR(ENOTTY);
	ADD_ERROR(ENXIO);
	ADD_ERROR(ESPIPE);
	ADD_ERROR(ESRCH);
	#ifdef EFPOS
		ADD_ERROR(EFPOS);
	#endif
	#ifdef ESIGPARM
		ADD_ERROR(ESIGPARM);
	#endif
	ADD_ERROR(EDOM);
	ADD_ERROR(ERANGE);
	ADD_ERROR(EPROTOTYPE);
	ADD_ERROR(EPROTONOSUPPORT);
	ADD_ERROR(EPFNOSUPPORT);
	ADD_ERROR(EAFNOSUPPORT);
	ADD_ERROR(EADDRINUSE);
	ADD_ERROR(EADDRNOTAVAIL);
	ADD_ERROR(ENETDOWN);
	ADD_ERROR(ENETUNREACH);
	ADD_ERROR(ENETRESET);
	ADD_ERROR(ECONNABORTED);
	ADD_ERROR(ECONNRESET);
	ADD_ERROR(EISCONN);
	ADD_ERROR(ENOTCONN);
	ADD_ERROR(ESHUTDOWN);
	ADD_ERROR(ECONNREFUSED);
	ADD_ERROR(EHOSTUNREACH);
	ADD_ERROR(ENOPROTOOPT);
	ADD_ERROR(ENOBUFS);
	ADD_ERROR(EINPROGRESS);
	ADD_ERROR(EALREADY);
	ADD_ERROR(EILSEQ);
	ADD_ERROR(ENOMSG);
	ADD_ERROR(ESTALE);
	ADD_ERROR(EOVERFLOW);
	ADD_ERROR(EMSGSIZE);
	ADD_ERROR(EOPNOTSUPP);
	ADD_ERROR(ENOTSOCK);
	ADD_ERROR(EHOSTDOWN);
	ADD_ERROR(EBADMSG);
	ADD_ERROR(ECANCELED);
	ADD_ERROR(EDESTADDRREQ);
	ADD_ERROR(EDQUOT);
	ADD_ERROR(EIDRM);
	ADD_ERROR(EMULTIHOP);
	#ifdef ENODATA
		ADD_ERROR(ENODATA);
	#endif
	ADD_ERROR(ENOLINK);
	#ifdef ENOSR
		ADD_ERROR(ENOSR);
	#endif
	#ifdef ENOSTR
		ADD_ERROR(ENOSTR);
	#endif
	ADD_ERROR(ENOTSUP);
	ADD_ERROR(EPROTO);
	#ifdef ETIME
		ADD_ERROR(ETIME);
	#endif
	ADD_ERROR(ETXTBSY);
	ADD_ERROR(ENOMEM);
	ADD_ERROR(EACCES);
	ADD_ERROR(EINTR);
	ADD_ERROR(EIO);
	ADD_ERROR(EBUSY);
	ADD_ERROR(EFAULT);
	ADD_ERROR(ETIMEDOUT);
	ADD_ERROR(EAGAIN);
	ADD_ERROR(EWOULDBLOCK);
	ADD_ERROR(EBADF);
	ADD_ERROR(EEXIST);
	ADD_ERROR(EINVAL);
	ADD_ERROR(ENAMETOOLONG);
	ADD_ERROR(ENOENT);
	ADD_ERROR(EPERM);
	ADD_ERROR(ENOTDIR);
	ADD_ERROR(EISDIR);
	ADD_ERROR(ENOTEMPTY);
	ADD_ERROR(ENOSPC);
	ADD_ERROR(EROFS);
	ADD_ERROR(EMFILE);
	ADD_ERROR(EXDEV);
	ADD_ERROR(ELOOP);
	ADD_ERROR(ENOEXEC);
	ADD_ERROR(EPIPE);

	sErrorMapsInitialized = true;
}

// to_host_error
static int
to_host_error(int error)
{
	init_error_map();

	map<int, int>::iterator it = sToHostErrorMap.find(error);
	return (it != sToHostErrorMap.end() ? it->second : error);
}

// to_haiku_error
static int
to_haiku_error(int error)
{
	init_error_map();

	map<int, int>::iterator it = sToHaikuErrorMap.find(error);
	if (it != sToHaikuErrorMap.end())
		return it->second;

	return (error > 0 ? -error : error);
}

// _haiku_build_strerror
char *
_haiku_build_strerror(int errnum)
{
	return strerror(to_host_error(errnum));
}

// _haiku_build_errno
int *
_haiku_build_errno()
{
	static int localErrno = 0;
	localErrno = to_haiku_error(errno);
	return &localErrno;
}

