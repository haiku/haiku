## Process this file with automake to create Makefile.in
AUTOMAKE_OPTIONS=no-dependencies

info_TEXINFOS = grep.texi

man_MANS = grep.1 fgrep.1 egrep.1

EXTRA_DIST = grep.1 egrep.man fgrep.man

CLEANFILES = egrep.1 fgrep.1

fgrep.1: fgrep.man
	inst=`echo "grep" | sed '$(transform)'`.1; \
sed -e "s%@grep@%$$inst%g" $(srcdir)/fgrep.man > $@

egrep.1: egrep.man
	inst=`echo "grep" | sed '$(transform)'`.1; \
sed -e "s%@grep@%$$inst%g" $(srcdir)/egrep.man > $@
