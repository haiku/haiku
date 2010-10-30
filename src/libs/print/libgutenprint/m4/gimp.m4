# Configure paths for GIMP
# Manish Singh    98-6-11
# Shamelessly stolen from Owen Taylor

dnl STP_PATH_GIMP([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for GIMP, and define GIMP_CFLAGS and GIMP_LIBS
dnl
AC_DEFUN([STP_PATH_GIMP],
[dnl
dnl Get the cflags and libraries from the gimptool script
dnl
AC_ARG_WITH(gimp-prefix,[  --with-gimp-prefix=PFX  Prefix where GIMP 1.2 is installed (optional)],
            gimptool_prefix="$withval", gimptool_prefix="")
AC_ARG_WITH(gimp-exec-prefix,[  --with-gimp-exec-prefix=PFX Exec prefix where GIMP 1.2 is installed (optional)],
            gimptool_exec_prefix="$withval", gimptool_exec_prefix="")
AC_ARG_ENABLE(gimptest, [  --disable-gimptest      Do not try to compile and run a test GIMP 1.2 program],
		    , enable_gimptest=yes)

  if test x$gimptool_exec_prefix != x ; then
     gimptool_args="$gimptool_args --exec-prefix=$gimptool_exec_prefix"
     if test x${GIMPTOOL+set} != xset ; then
        GIMPTOOL=$gimptool_exec_prefix/bin/gimptool-1.2
     fi
  fi
  if test x$gimptool_prefix != x ; then
     gimptool_args="$gimptool_args --prefix=$gimptool_prefix"
     if test x${GIMPTOOL+set} != xset ; then
        GIMPTOOL=$gimptool_prefix/bin/gimptool-1.2
     fi
  fi

  AC_PATH_PROGS([GIMPTOOL], [gimptool-1.2 gimptool], no)
  min_gimp_version=ifelse([$1], ,1.0.0,$1)
  AC_MSG_CHECKING(for GIMP 1.2 - version >= $min_gimp_version)
  no_gimp=""
  if test "$GIMPTOOL" = "no" ; then
    no_gimp=yes
  else
    GIMP_CFLAGS=`$GIMPTOOL $gimptool_args --cflags`
    GIMP_LIBS=`$GIMPTOOL $gimptool_args --libs`

    GIMP_CFLAGS_NOUI=`$GIMPTOOL $gimptool_args --cflags-noui`
    noui_test=`echo $GIMP_CFLAGS_NOUI | sed 's/^\(Usage\).*/\1/'`
    if test "$noui_test" = "Usage" ; then
       GIMP_CFLAGS_NOUI=$GIMP_CFLAGS
       GIMP_LIBS_NOUI=$GIMP_LIBS
    else
       GIMP_LIBS_NOUI=`$GIMPTOOL $gimptool_args --libs-noui`
    fi

    GIMP_DATA_DIR=`$GIMPTOOL $gimptool_args --gimpdatadir`
    GIMP_PLUGIN_DIR=`$GIMPTOOL $gimptool_args --gimpplugindir`
    nodatadir_test=`echo $GIMP_DATA_DIR | sed 's/^\(Usage\).*/\1/'`
    if test "$nodatadir_test" = "Usage" ; then
       GIMP_DATA_DIR=""
       GIMP_PLUGIN_DIR=""
    fi

    gimptool_major_version=`$GIMPTOOL $gimptool_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    gimptool_minor_version=`$GIMPTOOL $gimptool_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    gimptool_micro_version=`$GIMPTOOL $gimptool_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_gimptest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $GIMP_CFLAGS"
      LIBS="$LIBS $GIMP_LIBS"
dnl
dnl Now check if the installed GIMP 1.2 is sufficiently new. (Also sanity
dnl checks the results of gimptool to some extent
dnl
      rm -f conf.gimptest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>

#include <libgimp/gimp.h>

#ifndef GIMP_CHECK_VERSION
#define GIMP_CHECK_VERSION(major, minor, micro) \
    (GIMP_MAJOR_VERSION == (major) && \
     GIMP_MINOR_VERSION == (minor) && \
     GIMP_MICRO_VERSION >= (micro))
#endif

#if GIMP_CHECK_VERSION(1,2,0)
GimpPlugInInfo
#else
GPlugInInfo
#endif
PLUG_IN_INFO =
{
  NULL,  /* init_proc */
  NULL,  /* quit_proc */
  NULL,  /* query_proc */
  NULL   /* run_proc */
};

int main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.gimptest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = g_strdup("$min_gimp_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_gimp_version");
     exit(1);
   }

    if (($gimptool_major_version == major) &&
        ($gimptool_minor_version == minor) &&
        ($gimptool_micro_version >= micro))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'gimptool --version' returned %d.%d.%d, but the version\n", $gimptool_major_version, $gimptool_minor_version, $gimptool_micro_version);
      printf("*** of GIMP 1.2 required is at least %d.%d.%d. If gimptool is\n", major, minor, micro);
      printf("*** correct, then it is best to upgrade to the required version.\n");
      printf("*** If gimptool was wrong, set the environment variable GIMPTOOL\n");
      printf("*** to point to the correct copy of gimptool, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_gimp=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_gimp" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])
  else
     AC_MSG_RESULT(no)
     if test "$GIMPTOOL" = "no" ; then
       echo "*** The gimptool script installed by GIMP 1.2 could not be"
       echo "*** found.  If GIMP 1.2 was installed in PREFIX, make sure"
       echo "*** PREFIX/bin is in your path, or set the GIMPTOOL"
       echo "*** environment variable to the full path to gimptool."
     else
       if test -f conf.gimptest ; then
        :
       else
          echo "*** Could not run GIMP 1.2 test program, checking why..."
          CFLAGS="$CFLAGS $GIMP_CFLAGS"
          LIBS="$LIBS $GIMP_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include <libgimp/gimp.h>

#ifndef GIMP_CHECK_VERSION
#define GIMP_CHECK_VERSION(major, minor, micro) \
    (GIMP_MAJOR_VERSION == (major) && \
     GIMP_MINOR_VERSION == (minor) && \
     GIMP_MICRO_VERSION >= (micro))
#endif

#if GIMP_CHECK_VERSION(1,2,0)
GimpPlugInInfo
#else
GPlugInInfo
#endif
PLUG_IN_INFO =
{
  NULL,  /* init_proc */
  NULL,  /* quit_proc */
  NULL,  /* query_proc */
  NULL   /* run_proc */
};
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means that"
          echo "*** the run-time linker is not finding GIMP 1.2 or is finding the wrong"
          echo "*** version of GIMP.  If it is not finding GIMP 1.2, you'll need to set"
          echo "*** your LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf"
          echo "*** to point to the installed location.  Also, make sure you have run"
          echo "*** ldconfig if that is required on your system."
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it,"
          echo "*** although you may also be able to get things to work by modifying"
          echo "*** LD_LIBRARY_PATH."],
        [ echo "*** The test program failed to compile or link.  See the file config.log"
          echo "*** for the exact error that occured.  This usually means GIMP 1.2 was"
          echo "*** incorrectly installed or that you have moved GIMP 1.2 since it was"
          echo "*** installed.  In the latter case, you may want to edit the gimptool"
          echo "*** gimptool script: $GIMPTOOL" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     GIMP_CFLAGS=""
     GIMP_LIBS=""
     GIMP_CFLAGS_NOUI=""
     GIMP_LIBS_NOUI=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(GIMP_CFLAGS)
  AC_SUBST(GIMP_LIBS)
  AC_SUBST(GIMP_CFLAGS_NOUI)
  AC_SUBST(GIMP_LIBS_NOUI)
  AC_SUBST(GIMP_DATA_DIR)
  AC_SUBST(GIMP_PLUGIN_DIR)
  rm -f conf.gimptest
])
