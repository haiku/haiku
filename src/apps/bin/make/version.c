/* We use <config.h> instead of "config.h" so that a compilation
   using -I. -I$srcdir will use ./config.h rather than $srcdir/config.h
   (which it would do because make.h was found in $srcdir).  */
#include <config.h>

#ifndef MAKE_HOST
# define MAKE_HOST "unknown"
#endif

char *version_string = VERSION;
char *make_host = MAKE_HOST;

/*
  Local variables:
  version-control: never
  End:
 */
