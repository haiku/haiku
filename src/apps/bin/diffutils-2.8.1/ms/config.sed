# Additional editing of Makefiles and of config.status

# Copyright (C) 2001, 2002 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# Written by Eli Zaretskii.


/(echo[ 	]*':t/ a\
# DJGPP specific Makefile changes.\
  /^aliaspath *	*=/s,:,";",g;t t\
  /TEXINPUTS=/s,:,";",g;t t\
  /PATH=/s,:,";",g;t t\
  s,\\.deps,_deps,g;t t\
  s,\\.new\\.,_new.,g;t t\
  s,\\.old\\.,_old.,g;t t\
  s,\\.tab\\.,_tab.,g;t t\
  s,Makefile\\.in\\.in,Makefile.in-in,g;t t\
  s,Makefile\\.am\\.in,Makefile.am-in,g;t t\
  /^install-info-am:/,/^$/ {\
    /@list=.\\\$(INFO_DEPS)\[^ \]/s,DEPS),& diff.i,\
    s,\\(\\\$\\\$d/\\\$\\\$file-\\[0-9\\]\\[0-9\\]\\)\\(\[^ \]\\),\\1 \\$\\$d/\\$\\$file[0-9] \\$\\$d/\\$\\$file[0-9][0-9]\\2,\
    s,\\( \\\$\\\$file-\\[0-9\\]\\[0-9\\]\\)\\(\[^ \]\\),\\1 \\$\\$file[0-9] \\$\\$file[0-9][0-9]\\2,\
  }\
  /^uninstall-info-am:/,/^$/ {\
    /@list=.\\\$(INFO_DEPS)\[^ \]/s,DEPS),& diff.i,\
    s,\\(file-\\[0-9\\]\\[0-9\\]\\)\\(\[^ \]\\),\\1 \\$\\$file[0-9] \\$\\$file[0-9][0-9]\\2,\
  }

# Makefile.in.in is renamed to Makefile.in-in.
/^ac_config_files=/,/_ACEOF/ {
  s|po/Makefile\.in|&:po/Makefile.in-in|
}
/CONFIG_FILES=/ s|po/Makefile\.in|&:po/Makefile.in-in|2

# We always use _deps instead of .deps, because the latter is an
# invalid name on 8+3 MS-DOS filesystem.  This makes the generated
# Makefiles good for every DJGPP installation, not only the one
# where the package was configured (which could happen to be a
# Windows box, where leading dots in file names are allowed).
s,\.deps,_deps,g

# The following two items are changes needed for configuring
# and compiling across partitions.
# The given srcdir value is always translated from the
# "x:" syntax into "/dev/x" syntax while we run configure.
/^[ 	]*-srcdir=\*.*$/ a\
    ac_optarg=`echo "$ac_optarg" | sed "s,^\\([A-Za-z]\\):,/dev/\\1,"`
/set X `ls -Lt \$srcdir/ i\
   if `echo $srcdir | grep "^/dev/" - > /dev/null`; then\
     srcdir=`echo "$srcdir" | sed -e "s%^/dev/%%" -e "s%/%:/%"`\
   fi

# Autoconf 2.52e generated configure scripts write absolute paths into
# Makefiles, making them useless for DJGPP installations other than the
# one for which the package has been configured.
/MISSING=/,/^$/ {
  /^fi$/ a\
am_missing_run=`echo "$am_missing_run" | sed 's%/dev.*/diffutil.*[-.]2.*[7-9].*[0-9]%${top_srcdir}%'`
}
/^install_sh=/a\
install_sh=`echo "$install_sh" | sed 's%/dev.*/diffutil.*[-.]2.*[7-9].*[0-9]%${top_srcdir}%'`


# The following makes sure we are not going to remove a directory
# which is the cwd on its drive (DOS doesn't allow to remove such
# a directory).  The trick is to chdir to the root directory on
# temp directory's drive.
/^ *trap 'exit_status=\$\?; rm -rf/s%rm -rf%cd $tmp; cd /; &%

# AC_CONFIG_LINKS fails if the source and destination are on
# different file systems and symlinks don't work.
/^    ln \$srcdir/s%||%|| cp -pf $srcdir/$ac_source $ac_dest ||%
