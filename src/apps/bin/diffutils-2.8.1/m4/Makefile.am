## Process this file with automake to produce Makefile.in -*-Makefile-*-

##m4-files-begin
EXTRA_DIST = \
  README jm-glibc-io.m4 Makefile.am.in \
c-bs-a.m4 \
c-stack.m4 \
check-decl.m4 \
codeset.m4 \
dos.m4 \
error.m4 \
fnmatch.m4 \
gettext.m4 \
glibc21.m4 \
gnu-source.m4 \
iconv.m4 \
inttypes.m4 \
jm-glibc-io.m4 \
lcmessage.m4 \
lib-ld.m4 \
lib-link.m4 \
lib-prefix.m4 \
longlong.m4 \
malloc.m4 \
mbrtowc.m4 \
mbstate_t.m4 \
prereq.m4 \
progtest.m4 \
realloc.m4 \
regex.m4 \
setmode.m4 \
st_mtim.m4 \
stdbool.m4 \
strftime.m4 \
vararrays.m4 \
xstrtoumax.m4
##m4-files-end

Makefile.am: Makefile.am.in
	rm -f $@ $@t
	sed -n '1,/^##m4-files-begin/p' $< > $@t
	(((echo EXTRA_DIST =; \
	   echo "  README jm-glibc-io.m4 Makefile.am.in" \
	  ) | tr '\012' @); \
	  (echo *.m4|tr ' ' @) ) \
	  |sed 's/@$$/%/;s/@/ \\@/g' |tr @% '\012\012' \
	  >> $@t
	sed -n '/^##m4-files-end/,$$p' $< >> $@t
	chmod a-w $@t
	mv $@t $@
