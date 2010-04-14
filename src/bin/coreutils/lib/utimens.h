#include <time.h>
int fdutimens (char const *, int, struct timespec const [2]);
int gl_futimens (int, char const *, struct timespec const [2]);
int utimens (char const *, struct timespec const [2]);
int lutimens (char const *, struct timespec const [2]);

#if GNULIB_FDUTIMENSAT
# include <fcntl.h>
# include <sys/stat.h>

int fdutimensat (int dir, char const *name, int fd, struct timespec const [2]);

/* Using this function makes application code slightly more readable.  */
static inline int
lutimensat (int fd, char const *file, struct timespec const times[2])
{
  return utimensat (fd, file, times, AT_SYMLINK_NOFOLLOW);
}
#endif
