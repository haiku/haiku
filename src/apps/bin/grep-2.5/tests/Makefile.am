## Process this file with automake to create Makefile.in

AWK=@AWK@

TESTS = warning.sh khadafy.sh spencer1.sh bre.sh ere.sh \
        status.sh empty.sh options.sh backref.sh file.sh
EXTRA_DIST = $(TESTS) \
             khadafy.lines khadafy.regexp \
             spencer1.awk spencer1.tests \
             bre.awk ere.awk bre.tests ere.tests
CLEANFILES = spencer1.script bre.script ere.script khadafy.out patfile
TESTS_ENVIRONMENT = \
	AWK=$(AWK) \
	GREP=$(top_builddir)/src/grep \
	GREP_OPTIONS='' \
	LC_ALL=C

# the *.sh when checkout does not have the right permission
# make sure everyting is ok.
dist-hook:
	cd $(distdir); for i in $(TESTS); do chmod +x $$i; done
