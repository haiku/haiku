#!/bin/sh
# We want to support both legacy and current autoconf - this is a bit ugly...
AC_VERSION=`autoconf --version 2>&1 |head -n1 |sed -e "s/.* //;s/\.//;s/[a-z]//"`
if test -z "$AC_VERSION"; then
	echo "Warning: Couldn't determine autoconf version. Assuming a current version."
	AC_VERSION=252
fi
if test "$AC_VERSION" -lt 250; then
	rm -f acinclude.m4
	echo "#undef ssize_t" >acconfig.h
	for i in m4/*.m4; do
		if cat $i |grep -q "jm_"; then
			cat $i >>acinclude.m4
		elif test ! -e `aclocal --print-ac-dir`/`basename $i`; then
			cat $i >>acinclude.m4
		fi
	done
	aclocal
else
	aclocal -I m4
fi
autoheader
automake -a
if test "$AC_VERSION" -lt 250; then
	# Workaround for a bug in ancient versions of autoheader
	sed -e 's,#undef $,/* your autoheader is buggy */,g' config.hin >config.hin.new
	rm config.hin
	mv config.hin.new config.hin
	# Make sure config.hin doesn't get rebuilt after the workaround
	sed -e 's,@AUTOHEADER@,true,' Makefile.in >Makefile.in.new
	rm Makefile.in
	mv Makefile.in.new Makefile.in
fi
autoconf
