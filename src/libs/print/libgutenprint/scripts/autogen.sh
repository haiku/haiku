#!/bin/sh
# Run this to generate all the initial makefiles, etc.
# Shamelessly copied from Glade

DIE=0

if test -d m4local ; then
  :
else
  echo "Directory \`m4local' does not exist.  Creating it."
  if test -e m4local ; then
    echo "**Error**: A file \`m4local' exists and is not a directory."
    echo "Please remove it."
    DIE=1
  fi
  mkdir m4local
fi

libtoolv=`libtool --version | head -1 | sed 's,.*[      ]\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\).*[a-z]*\([   ]?.*\|\)$,\1,'`
libtool_major=`echo $libtoolv | awk -F. '{print $1}'`
libtool_minor=`echo $libtoolv | awk -F. '{print $2}'`
libtool_point=`echo $libtoolv | awk -F. '{print $3}'`

test "$libtool_major" -le 1 && {
  test "$libtool_minor" -lt 5
} && {
  echo
  echo "**Warning**: You should have \`libtool' 1.5 or newer installed to"
  echo "create a gutenprint distribution.  Earlier versions of libtool do"
  echo "not generate correct code for all platforms."
  echo "Get ftp://ftp.gnu.org/pub/gnu/libtool/libtool-1.5.tar.gz"
  echo "(or a newer version if it is available)"
  DIE=1
}

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`autoconf' installed to compile gutenprint."
  echo "Download the appropriate package for your distribution,"
  echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
  DIE=1
}

test -f $srcdir/configure.ac && sed "s/XXXRELEASE_DATE=XXX/RELEASE_DATE=\"`date '+%d %b %Y'`\"/" $srcdir/m4extra/stp_release.m4.in > $srcdir/m4/stp_release.m4

test -f $srcdir/ChangeLog || echo > $srcdir/ChangeLog

(grep "^AM_PROG_LIBTOOL" $srcdir/configure.ac >/dev/null) && {
  (libtool --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "**Error**: You must have \`libtool' installed to compile gutenprint."
    echo "Get ftp://ftp.gnu.org/pub/gnu/libtool/libtool-1.5.tar.gz"
    echo "(or a newer version if it is available)"
    DIE=1
  }
}

grep "^AM_GNU_GETTEXT" $srcdir/configure.ac >/dev/null && {
  grep "sed.*POTFILES" $srcdir/configure.ac >/dev/null || \
  (gettext --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "**Error**: You must have \`gettext' installed to compile gutenprint."
    echo "Get ftp://ftp.gnu.org/pub/gnu/gettext/gettext-0.16.tar.gz"
    echo "(or a newer version if it is available)"
    DIE=1
  }
}

(pkg-config --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`pkg-config' installed to compile gutenprint."
  echo "Download the appropriate package for your distribution,"
  echo "or get the source tarball at http://www.freedesktop.org/"
  DIE=1
}


#### MRS: The following now only generates a warning, since earlier
####      versions of gettext *do* work, they just don't create the
####      right uninstall code.

gettextv=`gettext --version | head -1 | awk '{print $NF}'`
gettext_major=`echo $gettextv | awk -F. '{print $1}'`
gettext_minor=`echo $gettextv | awk -F. '{print $2}'`
gettext_point=`echo $gettextv | awk -F. '{print $3}'`

test "$gettext_major" -eq 0 && {
  test "$gettext_minor" -lt 16
} && {
  echo
  echo "**Warning**: You must have \`gettext' 0.16 or newer installed to"
  echo "create a gutenprint distribution.  Earlier versions of gettext do"
  echo "not generate the correct 'make uninstall' code."
  echo "Get ftp://ftp.gnu.org/gnu/gettext/gettext-0.16.tar.gz"
  echo "(or a newer version if it is available)"
}

(autopoint --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`autopoint' installed to compile gutenprint."
  echo "Get ftp://ftp.gnu.org/pub/gnu/gettext/gettext-0.11.5.tar.gz"
  echo "(or a newer version if it is available)"
  DIE=1
  NO_AUTOMAKE=yes
}

(automake --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`automake' installed to compile gutenprint."
  echo "Get ftp://ftp.gnu.org/pub/gnu/automake/automake-1.7.tar.gz"
  echo "(or a newer version if it is available)"
  DIE=1
  NO_AUTOMAKE=yes
}


# if no automake, don't bother testing for aclocal
test -n "$NO_AUTOMAKE" || (aclocal --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: Missing \`aclocal'.  The version of \`automake'"
  echo "installed doesn't appear recent enough."
  echo "Get ftp://ftp.gnu.org/pub/gnu/automake/automake-1.7.tar.gz"
  echo "(or a newer version if it is available)"
  DIE=1
}

# Check first for existence and then for proper version of Jade >= 1.2.1

jade_err=0

# Exists?
jade_exists=`type -p $jade`
test -z "$jade_exists" && jade_err=1

# Proper rev?
test "$jade_err" -eq 0 && {
#  echo "Checking for proper revision of jade..."
  jade_version=`jade -v < /dev/null 2>&1 | grep -i "jade version" | awk -F\" '{print $2}'`

  jade_version_major=`echo $jade_version | awk -F. '{print $1}'`
  jade_version_minor=`echo $jade_version | awk -F. '{print $2}'`
  jade_version_point=`echo $jade_version | awk -F. '{print $3}'`

  test "$jade_version_major" -ge 1 || jade_err=1

  test "$jade_version_minor" -lt 2 || {
      test "$jade_version_minor" -eq 2 -a "$jade_version_point" -lt 1
    } && jade_err=1

  test "$jade_err" -eq 1 && {
    echo " "
    echo "***Warning***: You must have \"Jade\" version 1.2.1 or"
    echo "newer installed to build the Gutenprint user's guide."
    echo "Get ftp://ftp.jclark.com/pub/jade/jade-1.2.1.tar.gz"
    echo "(or a newer version if available)"
    echo " "
  }
}

# Check for existence of dvips

dvipsloc=`type -p dvips`

test -z "$dvipsloc" && {
  echo " "
  echo "***Warning***: You must have \"dvips\" installed to"
  echo "build the Gutenprint user's guide."
  echo " "
}

# Check for existence of jadetex

jadetexloc=`type -p jadetex`

test -z "$jadetexloc" && {
  echo " "
  echo "***Warning***: You must have \"jadetex\" version 3.5 or"
  echo "newer installed to build the Gutenprint user's guide."
  echo "Get ftp://prdownloads.sourceforge.net/jadetex/jadetex-3.5.tar.gz"
  echo "(or a newer version if available)"
  echo " "
}

# Check for OpenJade >= 1.3

openjade_err=0

openjadeloc=`type -p openjade`

# Exists?
test -z "$openjadeloc" && openjade_err=1

# Proper rev?
test "$openjade_err" -eq 0 && {
#  echo "Checking for proper revision of openjade..."
  openjade_version=`openjade -v < /dev/null 2>&1 | sed 's/"//g' | grep -i "openjade version" $tmp_file | awk -F ' ' '{print $4}'`
  openjade_version_major=`echo $openjade_version | awk -F. '{print $1}'`
  openjade_version_minor=`echo $openjade_version | awk -F. '{print $2}'`
  openjade_version_minor=`echo $openjade_version_minor | awk -F- '{print $1}'`

  test "$openjade_version_major" -ge 1 || openjade_err=1
  test "$openjade_version_minor" -ge 3 || openjade_err=1

  test "$openjade_err" -eq 1 && {
    echo " "
    echo "***Warning***: You must have \"OpenJade\" version 1.3 or"
    echo "newer installed to build the Gutenprint user's guide."
    echo "Get http://download.sourceforge.net/openjade/openjade-1.3.tar.gz"
    echo "(or a newer version if available)"
    echo " "
  }
}

# Check for ps2pdf

ps2pdfloc=`type -p ps2pdf`

test -z "ps2pdfloc" && {
  echo " "
  echo "***Warning***: You must have \"ps2pdf\" installed to"
  echo "build the Gutenprint user's guide."
  echo "\"ps2pdf\" comes from the GNU Ghostscript software package."
  echo "Get ftp://ftp.gnu.org/gnu/ghostscript/ghostscript-6.5.1.tar.gz"
  echo "(or a newer version if available)"
  echo " "
}

# Check first for existence and then for proper version of sgmltools-lite >=3.0.2

sgmltools_err=0

# Exists?
sgmltoolsloc=`type -p sgmltools`
test -z "$sgmltoolsloc" && sgmltools_err=1

# Proper rev?
test "$sgmltools_err" -eq 0 && {
#  echo "Checking for proper revision of sgmltools..."
  sgmltools_version=`sgmltools --version | awk '{print $3}'`

  sgmltools_version_major=`echo $sgmltools_version | awk -F. '{print $1}'`
  sgmltools_version_minor=`echo $sgmltools_version | awk -F. '{print $2}'`
  sgmltools_version_point=`echo $sgmltools_version | awk -F. '{print $3}'`

  test "$sgmltools_version_major" -ge 3 || sgmltools_err=1
  test "$sgmltools_version_minor" -gt 0 ||
    (test "$sgmltools_version_minor" -eq 0 -a "$sgmltools_version_point" -ge 2) ||
    sgmltools_err=1

  test "$sgmltools_err" -eq 1 && {
    echo " "
    echo "***Warning***: You must have \"sgmltools-lite\" version 3.0.2"
    echo "or newer installed to build the Gutenprint user's guide."
    echo "Get http://prdownloads.sourceforge.net/projects/sgmltools-lite/sgmltools-lite-3.0.2.tar.gz"
    echo "(or a newer version if available)"
    echo " "
  }
}

# Check for convert

convertloc=`type -p convert`
test -z "$convertloc" && {
  echo " "
  echo "***Warning***: You must have \"convert\" installed to"
  echo "build the Gutenprint user's guide."
  echo "\"convert\" comes from the ImageMagick software package."
  echo "Go to http://imagemagick.sourceforge.net/http and get"
  echo "the file ImageMagick-5.3.1.tar.gz"
  echo "(or a newer version if available)"
  echo " "
}

# Check for docbook version 4
# Note workaround for Fedora installation
# Include path for Fedora Docbook. A bit circuitous, but Fedora appends 
# a bunch of extra stuff to the name of the directory-- including the 
# version of Fedora (eg -fc9).  We don't want to test for every version 
# of Fedora and modern bourne shells won't expand the glob (*); 
# therefore, we do a `find` first and then test to see if there are any 
# results.

if test -d /usr/share/sgml/docbook ;  then
  fedora_docbook=`find /usr/share/sgml/docbook -type d -name 'sgml-dtd-4.*' -print`
fi

{
  test -d "/usr/share/sgml/docbook_4" || test -d "/usr/share/sgml/docbook/dtd/4.0" || test -d "/usr/share/sgml/docbook_4.1" || test -n "$fedora_docbook"
} || {
  echo " "
  echo "***Warning***: You must have "Docbook v4" installed to"
  echo "build the Gutenprint user's guide."
  echo " "
}

if test "$DIE" -eq 1; then
  exit 1
fi

if test -z "$*"; then
  echo "**Warning**: I am going to run \`configure' with no arguments."
  echo "If you wish to pass any to it, please specify them on the"
  echo \`$0\'" command line."
  echo
fi

case $CC in
xlc )
  am_opt=--include-deps;;
esac

for coin in `find $srcdir -name configure.ac -print`
do
  dr=`dirname $coin`
  if test -f $dr/NO-AUTO-GEN; then
    echo skipping $dr -- flagged as no auto-gen
  else
    echo processing $dr
    macrodirs=`sed -n -e 's,^dnl AM_ACLOCAL_INCLUDE(\(.*\)),\1,gp' < $coin`
    ( cd $dr
      aclocalinclude="$ACLOCAL_FLAGS"
      for k in $macrodirs; do
  	if test -d $k; then
          aclocalinclude="$aclocalinclude -I $k"
  	##else
	##  echo "**Warning**: No such directory \`$k'.  Ignored."
        fi
      done
      if grep "^AM_GNU_GETTEXT" configure.ac >/dev/null; then
	if grep "sed.*POTFILES" configure.ac >/dev/null; then
	  : do nothing -- we still have an old unmodified configure.ac
	else
	  echo "Creating $dr/aclocal.m4 ..."
	  rm -f aclocal.m4
	  test -r aclocal.m4 || touch aclocal.m4
	  # We've removed po/ChangeLog from the repository.  Version
	  # 0.10.40 of gettext appends an entry to the ChangeLog every time
	  # anyone runs autogen.sh.  Since developers do that a lot, and
	  # then proceed to commit their entire sandbox, we wind up with
	  # an ever-growing po/ChangeLog that generates CVS conflicts on
	  # a routine basis.  There's no good reason for this.
	  echo 'This ChangeLog is redundant. Please see the main ChangeLog for i18n changes.' > po/ChangeLog
	  echo >> po/ChangeLog
	  echo 'This file is present only to keep po/Makefile.in.in happy.' >> po/ChangeLog
	  echo "Running autopoint...  Ignore non-fatal messages."
	    autopoint --force
	  echo "Making $dr/aclocal.m4 writable ..."
	  test -r $dr/aclocal.m4 && chmod u+w $dr/aclocal.m4
        fi
      fi
      if grep "^AM_PROG_LIBTOOL" configure.ac >/dev/null; then
	echo "Running libtoolize..."
	libtoolize --force --copy
      fi
      echo "Running aclocal $aclocalinclude ..."
      aclocal $aclocalinclude
      if grep "^AM_CONFIG_HEADER" configure.ac >/dev/null; then
	echo "Running autoheader..."
	autoheader
      fi
      echo "Running automake --gnu $am_opt ..."
      automake --add-missing --force-missing --copy --gnu $am_opt
      echo "Running autoconf ..."
      autoconf
    )
  fi
done

conf_flags="--enable-maintainer-mode" #--enable-iso-c

if test x$NOCONFIGURE = x; then
  echo Running $srcdir/configure $conf_flags "$@" ...
  $srcdir/configure $conf_flags "$@" \
  && echo Now type \`make\' to compile $PKG_NAME || exit 1
else
  echo Skipping configure process.
fi
