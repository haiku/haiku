#! /bin/sh

# edit this to taste; note that you can also override via the environment:
case "$CC" in
  "") CC=cc
esac

if test -f config.h; then :; else
  echo "Creating basic config.h..."
  cat >config.h <<'END_OF_CONFIG_H'
/* A bootstrap version of config.h, for systems which can't
   auto-configure due to a lack of a working sed.  If you are on
   a sufficiently odd machine you may need to hand-tweak this file.

   Regardless, once you get a working version of sed you really should
   re-build starting with a run of "configure", as the bootstrap
   version is almost certainly more crippled than it needs to be on
   your machine.
*/

#define PACKAGE "sed"
#define VERSION "4.0.9-boot"
#define SED_FEATURE_VERSION "4.0"
#define BOOTSTRAP 1

/* Define if your compiler/headers don't support const. */
#undef const

/* Undefine if headers have conflicting definition.  */
#define mbstate_t int

/* Toggle if you encounter errors in lib/mkstemp.c.  */
#define HAVE_UNISTD_H
#define HAVE_FCNTL_H
#undef HAVE_SYS_FILE_H
#undef HAVE_IO_H

/* Undefine if <stdio.h> or <sys/types.h> has conflicting definition.  */
#define size_t unsigned
#define ssize_t int

/* If your antique compiler doesn't grok ``void *'', then #define VOID char */
#undef VOID


/* All other config.h.in options intentionally omitted.  Report as a
   bug if you need extra "#define"s in here. */
END_OF_CONFIG_H
fi

# tell the user what we're doing from here on...
set -x -e

# the ``|| exit 1''s are for fail-stop; set -e doesn't work on some systems

rm -f lib/*.o sed/*.o sed/sed
cd lib || exit 1
rm -f regex.h
cp regex_.h regex.h
${CC} -DHAVE_CONFIG_H -I.. -I. -c alloca.c
${CC} -DHAVE_CONFIG_H -I.. -I. -c getline.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c getopt.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c getopt1.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c memchr.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c memcmp.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c memmove.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c mkstemp.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c strverscmp.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c obstack.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c regex.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c strerror.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c utils.c || exit 1

cd ../sed || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -c sed.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -c fmt.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -c compile.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -c execute.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -c regexp.c || exit 1

${CC} -o sed *.o ../lib/*.o || exit 1
