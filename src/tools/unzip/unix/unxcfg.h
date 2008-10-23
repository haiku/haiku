// That's exactly the BeOS configuration.
#if (!defined(__BEOS__) && !defined(__HAIKU__))
#  include <sys/types.h>          /* [cjh]:  This is pretty much a generic  */
#  include <sys/stat.h>           /* POSIX 1003.1 system; see beos/ for     */
#  include <fcntl.h>              /* extra code to deal with our extra file */
#  include <sys/param.h>          /* attributes. */
#  include <unistd.h>
#  include <utime.h>
#  define DIRENT
#  include <time.h>
#  ifndef DATE_FORMAT
#    define DATE_FORMAT DF_MDY  /* GRR:  customize with locale.h somehow? */
#  endif
#  define lenEOL        1
#  define PutNativeEOL  *q++ = native(LF);
#  define SCREENSIZE(ttrows, ttcols)  screensize(ttrows, ttcols)
#  define SCREENWIDTH 80
#  define USE_EF_UT_TIME
#  define SET_DIR_ATTRIB
#  if (!defined(NOTIMESTAMP) && !defined(TIMESTAMP))
#    define TIMESTAMP
#  endif
#  define RESTORE_UIDGID
#  define NO_STRNICMP             /* not in the x86 headers at least */
#  define INT_SPRINTF
#  define SYMLINKS
#  define MAIN main_stub          /* now that we're using a wrapper... */
#  ifndef EOK
#    define EOK B_OK
#  endif

#  define HAIKU_USE_KERN_OPEN 1
extern int _kern_open(int fd, const char *path, int openMode, int perms);

#endif	// !__BEOS__
