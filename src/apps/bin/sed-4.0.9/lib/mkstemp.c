#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STRINGS_H
# include <strings.h>
#else
# include <string.h>
#endif /* HAVE_STRINGS_H */

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_SYS_FILE_H
# include <sys/file.h>
#endif /* HAVE_SYS_FILE_H */

#ifdef HAVE_IO_H
# include <io.h>
#endif /* HAVE_IO_H */

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#include <limits.h>
#include <errno.h>

/* Generate a unique temporary file name from template.  The last six characters of
   template must be XXXXXX and these are replaced with a string that makes the
   filename unique. */

int
mkstemp (template)
     char *template;
{
  int i, j, n, fd;
  char *data = template + strlen(template) - 6;

  if (data < template) {
    errno = EINVAL;
    return -1;
  }

  for (n = 0; n <= 5; n++)
    if (data[n] != 'X') {
      errno = EINVAL;
      return -1;
    }

  for (i = 0; i < INT_MAX; i++) {
    j = i ^ 827714841;             /* Base 36 DOSSUX :-) */
    for (n = 5; n >= 0; n--) {
      data[n] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" [j % 36];
      j /= 36;
    }

    fd = open (template, O_CREAT|O_EXCL|O_RDWR, 0600);
    if (fd != -1)
      return fd;
  }
    
  errno = EEXIST;
  return -1;
}
