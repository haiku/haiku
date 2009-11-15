# -*-Makefile-*-
# This Makefile fragment tries to be general-purpose enough to be
# used by many projects via the gnulib maintainer-makefile module.

## Copyright (C) 2001-2009 Free Software Foundation, Inc.
##
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.

# This is reported not to work with make-3.79.1
# ME := $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))
ME := maint.mk

# Override this in cfg.mk if you use a non-standard build-aux directory.
build_aux ?= $(srcdir)/build-aux

# Do not save the original name or timestamp in the .tar.gz file.
# Use --rsyncable if available.
gzip_rsyncable := \
  $(shell gzip --help 2>/dev/null|grep rsyncable >/dev/null && echo --rsyncable)
GZIP_ENV = '--no-name --best $(gzip_rsyncable)'

# cfg.mk must define the gpg_key_ID used by this package.
GIT = git
VC = $(GIT)
VC-tag = git tag -s -m '$(VERSION)' -u '$(gpg_key_ID)'

VC_LIST = $(build_aux)/vc-list-files -C $(srcdir)

VC_LIST_EXCEPT = \
  $(VC_LIST) | if test -f $(srcdir)/.x-$@; then grep -vEf $(srcdir)/.x-$@; \
	       else grep -Ev "$${VC_LIST_EXCEPT_DEFAULT-ChangeLog}"; fi

ifeq ($(origin prev_version_file), undefined)
  prev_version_file = $(srcdir)/.prev-version
endif

PREV_VERSION := $(shell cat $(prev_version_file) 2>/dev/null)
VERSION_REGEXP = $(subst .,\.,$(VERSION))
PREV_VERSION_REGEXP = $(subst .,\.,$(PREV_VERSION))

ifeq ($(VC),$(GIT))
this-vc-tag = v$(VERSION)
this-vc-tag-regexp = v$(VERSION_REGEXP)
else
tag-package = $(shell echo "$(PACKAGE)" | tr '[:lower:]' '[:upper:]')
tag-this-version = $(subst .,_,$(VERSION))
this-vc-tag = $(tag-package)-$(tag-this-version)
this-vc-tag-regexp = $(this-vc-tag)
endif
my_distdir = $(PACKAGE)-$(VERSION)

# Old releases are stored here.
release_archive_dir ?= ../release

# Prevent programs like 'sort' from considering distinct strings to be equal.
# Doing it here saves us from having to set LC_ALL elsewhere in this file.
export LC_ALL = C

## --------------- ##
## Sanity checks.  ##
## --------------- ##

_cfg_mk := $(shell test -f $(srcdir)/cfg.mk && echo '$(srcdir)/cfg.mk')

# Collect the names of rules starting with `sc_'.
syntax-check-rules := $(sort $(shell sed -n 's/^\(sc_[a-zA-Z0-9_-]*\):.*/\1/p' \
			$(srcdir)/$(ME) $(_cfg_mk)))
.PHONY: $(syntax-check-rules)

local-checks-available = \
  $(syntax-check-rules)
.PHONY: $(local-checks-available)

# Arrange to print the name of each syntax-checking rule just before running it.
$(syntax-check-rules): %: %.m
$(patsubst %, %.m, $(syntax-check-rules)):
	@echo $(patsubst sc_%.m, %, $@)

local-check := $(filter-out $(local-checks-to-skip), $(local-checks-available))

syntax-check: $(local-check)
#	@grep -nE '#  *include <(limits|std(def|arg|bool))\.h>'		\
#	    $$(find -type f -name '*.[chly]') &&			\
#	  { echo '$(ME): found conditional include' 1>&2;		\
#	    exit 1; } || :

#	grep -nE '^#  *include <(string|stdlib)\.h>'			\
#	    $(srcdir)/{lib,src}/*.[chy] &&				\
#	  { echo '$(ME): FIXME' 1>&2;					\
#	    exit 1; } || :
# FIXME: don't allow `#include .strings\.h' anywhere

# By default, _prohibit_regexp does not ignore case.
export ignore_case =
_ignore_case = $$(test -n "$$ignore_case" && echo -i || :)

# There are many rules below that prohibit constructs in this package.
# If the offending construct can be matched with a grep-E-style regexp,
# use this macro.  The shell variables "re" and "msg" must be defined.
define _prohibit_regexp
  dummy=; : so we do not need a semicolon before each use;		\
  test "x$$re" != x || { echo '$(ME): re not defined' 1>&2; exit 1; };	\
  test "x$$msg" != x || { echo '$(ME): msg not defined' 1>&2; exit 1; };\
  grep $(_ignore_case) -nE "$$re" $$($(VC_LIST_EXCEPT)) &&		\
    { echo '$(ME): '"$$msg" 1>&2; exit 1; } || :
endef

sc_avoid_if_before_free:
	@$(build_aux)/useless-if-before-free				\
		$(useless_free_options)					\
	    $$($(VC_LIST_EXCEPT) | grep -v useless-if-before-free) &&	\
	  { echo '$(ME): found useless "if" before "free" above' 1>&2;	\
	    exit 1; } || :

sc_cast_of_argument_to_free:
	@re='\<free *\( *\(' msg='don'\''t cast free argument'		\
	  $(_prohibit_regexp)

sc_cast_of_x_alloc_return_value:
	@re='\*\) *x(m|c|re)alloc\>'					\
	msg='don'\''t cast x*alloc return value'			\
	  $(_prohibit_regexp)

sc_cast_of_alloca_return_value:
	@re='\*\) *alloca\>' msg='don'\''t cast alloca return value'	\
	  $(_prohibit_regexp)

sc_space_tab:
	@re='[ ]	' msg='found SPACE-TAB sequence; remove the SPACE' \
	  $(_prohibit_regexp)

# Don't use *scanf or the old ato* functions in `real' code.
# They provide no error checking mechanism.
# Instead, use strto* functions.
sc_prohibit_atoi_atof:
	@re='\<([fs]?scanf|ato([filq]|ll)) *\('				\
	msg='do not use *scan''f, ato''f, ato''i, ato''l, ato''ll or ato''q' \
	  $(_prohibit_regexp)

# Use STREQ rather than comparing strcmp == 0, or != 0.
sc_prohibit_strcmp:
	@grep -nE '! *str''cmp *\(|\<str''cmp *\([^)]+\) *=='		\
	    $$($(VC_LIST_EXCEPT))					\
	  | grep -vE ':# *define STREQ\(' &&				\
	  { echo '$(ME): use STREQ in place of the above uses of str''cmp' \
		1>&2; exit 1; } || :

# Using EXIT_SUCCESS as the first argument to error is misleading,
# since when that parameter is 0, error does not exit.  Use `0' instead.
sc_error_exit_success:
	@grep -nE 'error \(EXIT_SUCCESS,'				\
	    $$($(VC_LIST_EXCEPT) | grep -E '\.[chly]$$') &&		\
	  { echo '$(ME): found error (EXIT_SUCCESS' 1>&2; exit 1; } || :

# `FATAL:' should be fully upper-cased in error messages
# `WARNING:' should be fully upper-cased, or fully lower-cased
sc_error_message_warn_fatal:
	@grep -nEA2 '[^rp]error \(' $$($(VC_LIST_EXCEPT))		\
	    | grep -E '"Warning|"Fatal|"fatal' &&			\
	  { echo '$(ME): use FATAL, WARNING or warning'	1>&2;		\
	    exit 1; } || :

# Error messages should not start with a capital letter
sc_error_message_uppercase:
	@grep -nEA2 '[^rp]error \(' $$($(VC_LIST_EXCEPT))		\
	    | grep -E '"[A-Z]'						\
	    | grep -vE '"FATAL|"WARNING|"Java|"C#|PRIuMAX' &&		\
	  { echo '$(ME): found capitalized error message' 1>&2;		\
	    exit 1; } || :

# Error messages should not end with a period
sc_error_message_period:
	@grep -nEA2 '[^rp]error \(' $$($(VC_LIST_EXCEPT))		\
	    | grep -E '[^."]\."' &&					\
	  { echo '$(ME): found error message ending in period' 1>&2;	\
	    exit 1; } || :

sc_file_system:
	@re=file''system ignore_case=1					\
	msg='found use of "file''system"; spell it "file system"'	\
	  $(_prohibit_regexp)

# Don't use cpp tests of this symbol.  All code assumes config.h is included.
sc_prohibit_have_config_h:
	@grep -n '^# *if.*HAVE''_CONFIG_H' $$($(VC_LIST_EXCEPT)) &&	\
	  { echo '$(ME): found use of HAVE''_CONFIG_H; remove'		\
		1>&2; exit 1; } || :

# Nearly all .c files must include <config.h>.  However, we also permit this
# via inclusion of a package-specific header, if cfg.mk specified one.
# config_h_header must be suitable for grep -E.
config_h_header ?= <config\.h>
sc_require_config_h:
	@if $(VC_LIST_EXCEPT) | grep -l '\.c$$' > /dev/null; then	\
	  grep -EL '^# *include $(config_h_header)'			\
		$$($(VC_LIST_EXCEPT) | grep '\.c$$')			\
	      | grep . &&						\
	  { echo '$(ME): the above files do not include <config.h>'	\
		1>&2; exit 1; } || :;					\
	else :;								\
	fi

# You must include <config.h> before including any other header file.
# This can possibly be via a package-specific header, if given by cfg.mk.
sc_require_config_h_first:
	@if $(VC_LIST_EXCEPT) | grep -l '\.c$$' > /dev/null; then	\
	  fail=0;							\
	  for i in $$($(VC_LIST_EXCEPT) | grep '\.c$$'); do		\
	    grep '^# *include\>' $$i | sed 1q				\
		| grep -E '^# *include $(config_h_header)' > /dev/null	\
	      || { echo $$i; fail=1; };					\
	  done;								\
	  test $$fail = 1 &&						\
	    { echo '$(ME): the above files include some other header'	\
		'before <config.h>' 1>&2; exit 1; } || :;		\
	else :;								\
	fi

sc_prohibit_HAVE_MBRTOWC:
	@re='\bHAVE_MBRTOWC\b' msg="do not use $$re; it is always defined" \
	  $(_prohibit_regexp)

# To use this "command" macro, you must first define two shell variables:
# h: the header, enclosed in <> or ""
# re: a regular expression that matches IFF something provided by $h is used.
define _header_without_use
  dummy=; : so we do not need a semicolon before each use;		\
  h_esc=`echo "$$h"|sed 's/\./\\\\./g'`;				\
  if $(VC_LIST_EXCEPT) | grep -l '\.c$$' > /dev/null; then		\
    files=$$(grep -l '^# *include '"$$h_esc"				\
	     $$($(VC_LIST_EXCEPT) | grep '\.c$$')) &&			\
    grep -LE "$$re" $$files | grep . &&					\
      { echo "$(ME): the above files include $$h but don't use it"	\
	1>&2; exit 1; } || :;						\
  else :;								\
  fi
endef

# Prohibit the inclusion of assert.h without an actual use of assert.
sc_prohibit_assert_without_use:
	@h='<assert.h>' re='\<assert *\(' $(_header_without_use)

# Prohibit the inclusion of getopt.h without an actual use.
sc_prohibit_getopt_without_use:
	@h='<getopt.h>' re='\<getopt(_long)? *\(' $(_header_without_use)

# Don't include quotearg.h unless you use one of its functions.
sc_prohibit_quotearg_without_use:
	@h='"quotearg.h"' re='\<quotearg(_[^ ]+)? *\(' $(_header_without_use)

# Don't include quote.h unless you use one of its functions.
sc_prohibit_quote_without_use:
	@h='"quote.h"' re='\<quote(_n)? *\(' $(_header_without_use)

# Don't include this header unless you use one of its functions.
sc_prohibit_long_options_without_use:
	@h='"long-options.h"' re='\<parse_long_options *\(' \
	  $(_header_without_use)

# Don't include this header unless you use one of its functions.
sc_prohibit_inttostr_without_use:
	@h='"inttostr.h"' re='\<(off|[iu]max|uint)tostr *\(' \
	  $(_header_without_use)

# Don't include this header unless you use one of its functions.
sc_prohibit_error_without_use:
	@h='"error.h"' \
	re='\<error(_at_line|_print_progname|_one_per_line|_message_count)? *\('\
	  $(_header_without_use)

sc_prohibit_safe_read_without_use:
	@h='"safe-read.h"' re='(\<SAFE_READ_ERROR\>|\<safe_read *\()' \
	  $(_header_without_use)

sc_prohibit_argmatch_without_use:
	@h='"argmatch.h"' \
	re='(\<(ARRAY_CARDINALITY|X?ARGMATCH(|_TO_ARGUMENT|_VERIFY))\>|\<argmatch(_exit_fn|_(in)?valid) *\()' \
	  $(_header_without_use)

sc_prohibit_root_dev_ino_without_use:
	@h='"root-dev-ino.h"' \
	re='(\<ROOT_DEV_INO_(CHECK|WARN)\>|\<get_root_dev_ino *\()' \
	  $(_header_without_use)

# Prohibit the inclusion of c-ctype.h without an actual use.
ctype_re = isalnum|isalpha|isascii|isblank|iscntrl|isdigit|isgraph|islower\
|isprint|ispunct|isspace|isupper|isxdigit|tolower|toupper
sc_prohibit_c_ctype_without_use:
	@h='[<"]c-ctype.h[">]' re='\<c_($(ctype_re)) *\(' $(_header_without_use)

_empty =
_sp = $(_empty) $(_empty)
# The following list was generated by running:
# man signal.h|col -b|perl -ne '/bsd_signal.*;/.../sigwaitinfo.*;/ and print' \
#   | perl -lne '/^\s+(?:int|void).*?(\w+).*/ and print $1' | fmt
_sig_functions = \
  bsd_signal kill killpg pthread_kill pthread_sigmask raise sigaction \
  sigaddset sigaltstack sigdelset sigemptyset sigfillset sighold sigignore \
  siginterrupt sigismember signal sigpause sigpending sigprocmask sigqueue \
  sigrelse sigset sigsuspend sigtimedwait sigwait sigwaitinfo
_sig_function_re = $(subst $(_sp),|,$(strip $(_sig_functions)))
# The following were extracted from "man signal.h" manually.
_sig_types_and_consts =							\
  MINSIGSTKSZ SA_NOCLDSTOP SA_NOCLDWAIT SA_NODEFER SA_ONSTACK		\
  SA_RESETHAND SA_RESTART SA_SIGINFO SIGEV_NONE SIGEV_SIGNAL		\
  SIGEV_THREAD SIGSTKSZ SIG_BLOCK SIG_SETMASK SIG_UNBLOCK SS_DISABLE	\
  SS_ONSTACK mcontext_t pid_t sig_atomic_t sigevent siginfo_t sigset_t	\
  sigstack sigval stack_t ucontext_t
# generated via this:
# perl -lne '/^#ifdef (SIG\w+)/ and print $1' lib/sig2str.c|sort -u|fmt -70
_sig_names =								\
  SIGABRT SIGALRM SIGALRM1 SIGBUS SIGCANCEL SIGCHLD SIGCLD SIGCONT	\
  SIGDANGER SIGDIL SIGEMT SIGFPE SIGFREEZE SIGGRANT SIGHUP SIGILL	\
  SIGINFO SIGINT SIGIO SIGIOT SIGKAP SIGKILL SIGKILLTHR SIGLOST SIGLWP	\
  SIGMIGRATE SIGMSG SIGPHONE SIGPIPE SIGPOLL SIGPRE SIGPROF SIGPWR	\
  SIGQUIT SIGRETRACT SIGSAK SIGSEGV SIGSOUND SIGSTKFLT SIGSTOP SIGSYS	\
  SIGTERM SIGTHAW SIGTRAP SIGTSTP SIGTTIN SIGTTOU SIGURG SIGUSR1	\
  SIGUSR2 SIGVIRT SIGVTALRM SIGWAITING SIGWINCH SIGWIND SIGWINDOW	\
  SIGXCPU SIGXFSZ
_sig_syms_re = $(subst $(_sp),|,$(strip $(_sig_names) $(_sig_types_and_consts)))

# Prohibit the inclusion of signal.h without an actual use.
sc_prohibit_signal_without_use:
	@h='<signal.h>'							\
	re='\<($(_sig_function_re)) *\(|\<($(_sig_syms_re))\>'		\
	  $(_header_without_use)

sc_obsolete_symbols:
	@re='\<(HAVE''_FCNTL_H|O''_NDELAY)\>'				\
	msg='do not use HAVE''_FCNTL_H or O'_NDELAY			\
	  $(_prohibit_regexp)

# FIXME: warn about definitions of EXIT_FAILURE, EXIT_SUCCESS, STREQ

# Each nonempty ChangeLog line must start with a year number, or a TAB.
sc_changelog:
	@if $(VC_LIST_EXCEPT) | grep -l '^ChangeLog$$' >/dev/null; then	\
	  grep -n '^[^12	]'					\
	    $$($(VC_LIST_EXCEPT) | grep '^ChangeLog$$') &&		\
	  { echo '$(ME): found unexpected prefix in a ChangeLog' 1>&2;	\
	    exit 1; } || :;						\
	fi

# Ensure that each .c file containing a "main" function also
# calls set_program_name.
sc_program_name:
	@if $(VC_LIST_EXCEPT) | grep -l '\.c$$' > /dev/null; then	\
	  files=$$(grep -l '^main *(' $$($(VC_LIST_EXCEPT) | grep '\.c$$')); \
	  grep -LE 'set_program_name *\(m?argv\[0\]\);' $$files		\
	      | grep . &&						\
	  { echo '$(ME): the above files do not call set_program_name'	\
		1>&2; exit 1; } || :;					\
	else :;								\
	fi

# Require that the final line of each test-lib.sh-using test be this one:
# Exit $fail
# Note: this test requires GNU grep's --label= option.
Exit_witness_file ?= tests/test-lib.sh
Exit_base := $(notdir $(Exit_witness_file))
sc_require_test_exit_idiom:
	@if test -f $(srcdir)/$(Exit_witness_file); then		\
	  die=0;							\
	  for i in $$(grep -l -F 'srcdir/$(Exit_base)'			\
		$$($(VC_LIST) tests)); do				\
	    tail -n1 $$i | grep '^Exit .' > /dev/null			\
	      && : || { die=1; echo $$i; }				\
	  done;								\
	  test $$die = 1 &&						\
	    { echo 1>&2 '$(ME): the final line in each of the above is not:'; \
	      echo 1>&2 'Exit something';				\
	      exit 1; } || :;						\
	fi

sc_the_the:
	@re='\<the ''the\>'						\
	ignore_case=1 msg='found use of "the ''the";'			\
	  $(_prohibit_regexp)

sc_trailing_blank:
	@re='[	 ]$$'							\
	msg='found trailing blank(s)'					\
	  $(_prohibit_regexp)

# Match lines like the following, but where there is only one space
# between the options and the description:
#   -D, --all-repeated[=delimit-method]  print all duplicate lines\n
longopt_re = --[a-z][0-9A-Za-z-]*(\[?=[0-9A-Za-z-]*\]?)?
sc_two_space_separator_in_usage:
	@grep -nE '^   *(-[A-Za-z],)? $(longopt_re) [^ ].*\\$$'		\
	    $$($(VC_LIST_EXCEPT)) &&					\
	  { echo "$(ME): help2man requires at least two spaces between"; \
	    echo "$(ME): an option and its description";		\
		1>&2; exit 1; } || :

# Look for diagnostics that aren't marked for translation.
# This won't find any for which error's format string is on a separate line.
sc_unmarked_diagnostics:
	@grep -nE							\
	    '\<error \([^"]*"[^"]*[a-z]{3}' $$($(VC_LIST_EXCEPT))	\
	  | grep -v '_''(' &&						\
	  { echo '$(ME): found unmarked diagnostic(s)' 1>&2;		\
	    exit 1; } || :

# Avoid useless parentheses like those in this example:
# #if defined (SYMBOL) || defined (SYM2)
sc_useless_cpp_parens:
	@grep -n '^# *if .*defined *(' $$($(VC_LIST_EXCEPT)) &&		\
	  { echo '$(ME): found useless parentheses in cpp directive'	\
		1>&2; exit 1; } || :

# Require the latest GPL.
sc_GPL_version:
	@re='either ''version [^3]' msg='GPL vN, N!=3'			\
	  $(_prohibit_regexp)

cvs_keywords = \
  Author|Date|Header|Id|Name|Locker|Log|RCSfile|Revision|Source|State

sc_prohibit_cvs_keyword:
	@re='\$$($(cvs_keywords))\$$'					\
	    msg='do not use CVS keyword expansion'			\
	  $(_prohibit_regexp)

# Make sure we don't use st_blocks.  Use ST_NBLOCKS instead.
# This is a bit of a kludge, since it prevents use of the string
# even in comments, but for now it does the job with no false positives.
sc_prohibit_stat_st_blocks:
	@re='[.>]st_blocks' msg='do not use st_blocks; use ST_NBLOCKS'	\
	  $(_prohibit_regexp)

# Make sure we don't define any S_IS* macros in src/*.c files.
# They're already defined via gnulib's sys/stat.h replacement.
sc_prohibit_S_IS_definition:
	@re='^ *# *define  *S_IS'					\
	msg='do not define S_IS* macros; include <sys/stat.h>'		\
	  $(_prohibit_regexp)

# Each program that uses proper_name_utf8 must link with
# one of the ICONV libraries.
sc_proper_name_utf8_requires_ICONV:
	@progs=$$(grep -l 'proper_name_utf8 ''("' $$($(VC_LIST_EXCEPT)));\
	if test "x$$progs" != x; then					\
	  fail=0;							\
	  for p in $$progs; do						\
	    dir=$$(dirname "$$p");					\
	    base=$$(basename "$$p" .c);					\
	    grep "$${base}_LDADD.*ICONV)" $$dir/Makefile.am > /dev/null	\
	      || { fail=1; echo 1>&2 "$(ME): $$p uses proper_name_utf8"; }; \
	  done;								\
	  test $$fail = 1 &&						\
	    { echo 1>&2 '$(ME): the above do not link with any ICONV library'; \
	      exit 1; } || :;						\
	fi

# Warn about "c0nst struct Foo const foo[]",
# but not about "char const *const foo" or "#define const const".
sc_redundant_const:
	@re='\bconst\b[[:space:][:alnum:]]{2,}\bconst\b'		\
	msg='redundant "const" in declarations'				\
	  $(_prohibit_regexp)

sc_const_long_option:
	@grep '^ *static.*struct option ' $$($(VC_LIST_EXCEPT))		\
	  | grep -Ev 'const struct option|struct option const' && {	\
	      echo 1>&2 '$(ME): add "const" to the above declarations'; \
	      exit 1; } || :

NEWS_hash =								\
  $$(sed -n '/^\*.* $(PREV_VERSION_REGEXP) ([0-9-]*)/,$$p'		\
       $(srcdir)/NEWS							\
     | grep -v '^Copyright .*Free Software'				\
     | md5sum -								\
     | sed 's/ .*//')

# Ensure that we don't accidentally insert an entry into an old NEWS block.
sc_immutable_NEWS:
	@if test -f $(srcdir)/NEWS; then				\
	  test "$(NEWS_hash)" = '$(old_NEWS_hash)' && : ||		\
	    { echo '$(ME): you have modified old NEWS' 1>&2; exit 1; };	\
	fi

# Update the hash stored above.  Do this after each release and
# for any corrections to old entries.
update-NEWS-hash: NEWS
	perl -pi -e 's/^(old_NEWS_hash[ \t]+:?=[ \t]+).*/$${1}'"$(NEWS_hash)/" \
	  $(srcdir)/cfg.mk

# Ensure that we use only the standard $(VAR) notation,
# not @...@ in Makefile.am, now that we can rely on automake
# to emit a definition for each substituted variable.
# We use perl rather than "grep -nE ..." to exempt a single
# use of an @...@-delimited variable name in src/Makefile.am.
sc_makefile_check:
	@perl -ne '/\@[A-Z_0-9]+\@/ && !/^cu_install_program =/'	\
	  -e 'and (print "$$ARGV:$$.: $$_"), $$m=1; END {exit !$$m}'	\
	    $$($(VC_LIST_EXCEPT) | grep -E '(^|/)Makefile\.am$$')	\
	  && { echo '$(ME): use $$(...), not @...@' 1>&2; exit 1; } || :

news-date-check: NEWS
	today=`date +%Y-%m-%d`;						\
	if head $(srcdir)/NEWS | grep '^\*.* $(VERSION_REGEXP) ('$$today')' \
	    >/dev/null; then						\
	  :;								\
	else								\
	  echo "version or today's date is not in NEWS" 1>&2;		\
	  exit 1;							\
	fi

sc_makefile_TAB_only_indentation:
	@grep -nE '^	[ ]{8}'						\
	    $$($(VC_LIST_EXCEPT) | grep -E 'akefile|\.mk$$')		\
	  && { echo '$(ME): found TAB-8-space indentation' 1>&2;	\
	       exit 1; } || :

sc_m4_quote_check:
	@grep -nE '(AC_DEFINE(_UNQUOTED)?|AC_DEFUN)\([^[]'		\
	    $$($(VC_LIST_EXCEPT) | grep -E '(^configure\.ac|\.m4)$$')	\
	  && { echo '$(ME): quote the first arg to AC_DEF*' 1>&2;	\
	       exit 1; } || :

fix_po_file_diag = \
'you have changed the set of files with translatable diagnostics;\n\
apply the above patch\n'

# Verify that all source files using _() are listed in po/POTFILES.in.
po_file = po/POTFILES.in
sc_po_check:
	@if test -f $(po_file); then					\
	  grep -E -v '^(#|$$)' $(po_file)				\
	    | grep -v '^src/false\.c$$' | sort > $@-1;			\
	  files=;							\
	  for file in $$($(VC_LIST_EXCEPT)) lib/*.[ch]; do		\
	    test -r $$file || continue;					\
	    case $$file in						\
	      *.m4|*.mk) continue ;;					\
	      *.?|*.??) ;;						\
	      *) continue;;						\
	    esac;							\
	    case $$file in						\
	    *.[ch])							\
	      base=`expr " $$file" : ' \(.*\)\..'`;			\
	      { test -f $$base.l || test -f $$base.y; } && continue;;	\
	    esac;							\
	    files="$$files $$file";					\
	  done;								\
	  grep -E -l '\b(N?_|gettext *)\([^)"]*("|$$)' $$files		\
	    | sort -u > $@-2;						\
	  diff -u -L $(po_file) -L $(po_file) $@-1 $@-2			\
	    || { printf '$(ME): '$(fix_po_file_diag) 1>&2; exit 1; };	\
	  rm -f $@-1 $@-2;						\
	fi

# Sometimes it is useful to change the PATH environment variable
# in Makefiles.  When doing so, it's better not to use the Unix-centric
# path separator of `:', but rather the automake-provided `$(PATH_SEPARATOR)'.
msg = '$(ME): Do not use `:'\'' above; use $$(PATH_SEPARATOR) instead'
sc_makefile_path_separator_check:
	@grep -nE 'PATH[=].*:'						\
	    $$($(VC_LIST_EXCEPT) | grep -E 'akefile|\.mk$$')		\
	  && { echo $(msg) 1>&2; exit 1; } || :

# Check that `make alpha' will not fail at the end of the process.
writable-files:
	if test -d $(release_archive_dir); then :; else			\
	  for file in $(distdir).tar.gz					\
		      $(release_archive_dir)/$(distdir).tar.gz; do	\
	    test -e $$file || continue;					\
	    test -w $$file						\
	      || { echo ERROR: $$file is not writable; fail=1; };	\
	  done;								\
	  test "$$fail" && exit 1 || : ;				\
	fi

v_etc_file = lib/version-etc.c
sample-test = tests/sample-test
texi = doc/$(PACKAGE).texi
# Make sure that the copyright date in $(v_etc_file) is up to date.
# Do the same for the $(sample-test) and the main doc/.texi file.
sc_copyright_check:
	@if test -f $(v_etc_file); then					\
	  grep 'enum { COPYRIGHT_YEAR = '$$(date +%Y)' };' $(v_etc_file) \
	    >/dev/null							\
	  || { echo 'out of date copyright in $(v_etc_file); update it' 1>&2; \
	       exit 1; };						\
	fi
	@if test -f $(sample-test); then				\
	  grep '# Copyright (C) '$$(date +%Y)' Free' $(sample-test)	\
	    >/dev/null							\
	  || { echo 'out of date copyright in $(sample-test); update it' 1>&2; \
	       exit 1; };						\
	fi
	@if test -f $(texi); then					\
	  grep 'Copyright @copyright{} .*'$$(date +%Y)' Free' $(texi)	\
	    >/dev/null							\
	  || { echo 'out of date copyright in $(texi); update it' 1>&2;	\
	       exit 1; };						\
	fi

vc-diff-check:
	(unset CDPATH; cd $(srcdir) && $(VC) diff) > vc-diffs || :
	if test -s vc-diffs; then				\
	  cat vc-diffs;						\
	  echo "Some files are locally modified:" 1>&2;		\
	  exit 1;						\
	else							\
	  rm vc-diffs;						\
	fi

# Use this to make sure we don't run these programs when building
# from a virgin tgz file, below.
null_AM_MAKEFLAGS = \
  ACLOCAL=false \
  AUTOCONF=false \
  AUTOMAKE=false \
  AUTOHEADER=false \
  MAKEINFO=false

built_programs = $$(cd src && MAKEFLAGS= $(MAKE) -s built_programs.list)

rel-files = $(DIST_ARCHIVES)

gnulib_dir ?= $(srcdir)/gnulib
gnulib-version = $$(cd $(gnulib_dir) && git describe)
bootstrap-tools ?= autoconf,automake,gnulib

announcement: NEWS ChangeLog $(rel-files)
	@$(build_aux)/announce-gen					\
	    --release-type=$(RELEASE_TYPE)				\
	    --package=$(PACKAGE)					\
	    --prev=$(PREV_VERSION)					\
	    --curr=$(VERSION)						\
	    --gpg-key-id=$(gpg_key_ID)					\
	    --news=NEWS							\
	    --bootstrap-tools=$(bootstrap-tools)			\
	    --gnulib-version=$(gnulib-version)				\
	    --no-print-checksums					\
	    $(addprefix --url-dir=, $(url_dir_list))

## ---------------- ##
## Updating files.  ##
## ---------------- ##

ftp-gnu = ftp://ftp.gnu.org/gnu
www-gnu = http://www.gnu.org

emit_upload_commands:
	@echo =====================================
	@echo =====================================
	@echo "$(build_aux)/gnupload $(GNUPLOADFLAGS) \\"
	@echo "    --to $(gnu_rel_host):$(PACKAGE) \\"
	@echo "  $(rel-files)"
	@echo '# send the /tmp/announcement e-mail'
	@echo =====================================
	@echo =====================================

noteworthy = * Noteworthy changes in release ?.? (????-??-??) [?]
define emit-commit-log
  printf '%s\n' 'post-release administrivia' '' \
    '* NEWS: Add header line for next release.' \
    '* .prev-version: Record previous version.' \
    '* cfg.mk (old_NEWS_hash): Auto-update.'
endef

.PHONY: no-submodule-changes
no-submodule-changes:
	if test -d $(srcdir)/.git; then					\
	  diff=$$(cd $(srcdir) && git submodule -q foreach		\
		  git diff-index --name-only HEAD)			\
	    || exit 1;							\
	  case $$diff in '') ;;						\
	    *) echo '$(ME): submodule files are locally modified:';	\
		echo "$$diff"; exit 1;; esac;				\
	else								\
	  : ;								\
	fi

.PHONY: alpha beta major
ALL_RECURSIVE_TARGETS += alpha beta major
alpha beta major: $(local-check) writable-files no-submodule-changes
	test $@ = major						\
	  && { echo $(VERSION) | grep -E '^[0-9]+(\.[0-9]+)+$$'	\
	       || { echo "invalid version string: $(VERSION)" 1>&2; exit 1;};}\
	  || :
	$(MAKE) vc-diff-check
	$(MAKE) news-date-check
	$(MAKE) distcheck
	$(MAKE) dist XZ_OPT=-9ev
	$(MAKE) -s announcement RELEASE_TYPE=$@ > /tmp/announce-$(my_distdir)
	if test -d $(release_archive_dir); then			\
	  ln $(rel-files) $(release_archive_dir);		\
	  chmod a-w $(rel-files);				\
	fi
	$(MAKE) -s emit_upload_commands RELEASE_TYPE=$@
	echo $(VERSION) > $(prev_version_file)
	$(MAKE) update-NEWS-hash
	perl -pi -e '$$. == 3 and print "$(noteworthy)\n\n\n"' NEWS
	$(emit-commit-log) > .ci-msg
	$(VC) commit -F .ci-msg -a

.PHONY: web-manual
web-manual:
	@test -z "$(manual_title)" \
	  && { echo define manual_title in cfg.mk 1>&2; exit 1; } || :
	@cd '$(srcdir)/doc'; \
	  $(SHELL) ../build-aux/gendocs.sh -o '$(abs_builddir)/doc/manual' \
	     --email $(PACKAGE_BUGREPORT) $(PACKAGE) \
	    "$(PACKAGE_NAME) - $(manual_title)"
	@echo " *** Upload the doc/manual directory to web-cvs."

# Code Coverage

init-coverage:
	$(MAKE) $(AM_MAKEFLAGS) clean
	lcov --directory . --zerocounters

COVERAGE_CCOPTS ?= "-g --coverage"
COVERAGE_OUT ?= doc/coverage

build-coverage:
	$(MAKE) $(AM_MAKEFLAGS) CFLAGS=$(COVERAGE_CCOPTS) CXXFLAGS=$(COVERAGE_CCOPTS)
	$(MAKE) $(AM_MAKEFLAGS) CFLAGS=$(COVERAGE_CCOPTS) CXXFLAGS=$(COVERAGE_CCOPTS) check
	mkdir -p $(COVERAGE_OUT)
	lcov --directory . --output-file $(COVERAGE_OUT)/$(PACKAGE).info \
		--capture

gen-coverage:
	genhtml --output-directory $(COVERAGE_OUT) \
		$(COVERAGE_OUT)/$(PACKAGE).info \
		--highlight --frames --legend \
		--title "$(PACKAGE_NAME)"

coverage: init-coverage build-coverage gen-coverage

# Update gettext files.
PACKAGE ?= $(shell basename $(PWD))
PO_DOMAIN ?= $(PACKAGE)
POURL = http://translationproject.org/latest/$(PO_DOMAIN)/
PODIR ?= po
refresh-po:
	rm -f $(PODIR)/*.po && \
	echo "$(ME): getting translations into po (please ignore the robots.txt ERROR 404)..." && \
	wget --no-verbose --directory-prefix $(PODIR) --no-directories --recursive --level 1 --accept .po --accept .po.1 $(POURL) && \
	echo 'en@boldquot' > $(PODIR)/LINGUAS && \
	echo 'en@quot' >> $(PODIR)/LINGUAS && \
	ls $(PODIR)/*.po | sed 's/\.po//' | sed 's,$(PODIR)/,,' | sort >> $(PODIR)/LINGUAS

INDENT_SOURCES ?= $(C_SOURCES)
.PHONY: indent
indent:
	indent $(INDENT_SOURCES)

# If you want to set UPDATE_COPYRIGHT_* environment variables,
# put the assignments in this variable.
update-copyright-env ?=

# Run this rule once per year (usually early in January)
# to update all FSF copyright year lists in your project.
# If you have an additional project-specific rule,
# add it in cfg.mk along with a line 'update-copyright: prereq'.
# By default, exclude all variants of COPYING; you can also
# add exemptions (such as ChangeLog..* for rotated change logs)
# in the file .x-update-copyright.
.PHONY: update-copyright
update-copyright:
	grep -l -w Copyright                                             \
	  $$(export VC_LIST_EXCEPT_DEFAULT=COPYING && $(VC_LIST_EXCEPT)) \
	  | $(update-copyright-env) xargs $(build_aux)/$@
