dnl Check for DJGPP. we use DJ_GPP as the variable
dnl EXEEXXT
AC_DEFUN(AC_DJGPP,
[AC_CACHE_CHECK(for DJGPP environment, ac_cv_djgpp,
[AC_TRY_COMPILE(,[ return __DJGPP__;],
ac_cv_djgpp=yes, ac_cv_djgpp=no)
rm -f conftest*])
DJ_GPP=
test "$ac_cv_djgpp" = yes && DJ_GPP=yes])
