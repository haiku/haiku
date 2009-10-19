dnl $Id: config.m4 14574 2005-10-29 16:27:43Z bonefish $

PHP_ARG_WITH(pdflib,whether to include PDFlib support,
[  --with-pdflib[=DIR]     Include PDFlib 4.x support. DIR is the PDFlib
                          base install directory, defaults to /usr/local
                          Set DIR to "shared" to build as dl, or "shared,DIR"
                          to build as dl and still specify DIR.])

  case "$PHP_PDFLIB" in
    yes)
      PHP_EXTENSION(pdf, $ext_shared)
      AC_DEFINE(HAVE_PDFLIB,1,[ ])
      AC_SUBST(PDFLIB_SHARED_LIBADD)
      PHP_ADD_LIBRARY(pdf, PDFLIB_SHARED_LIBADD)
      ;;
    no)
      ;;
    *)
      dnl build from a installed/binary pdflib distribution
      if test -f "$PHP_PDFLIB/include/pdflib.h" ; then
        PHP_EXTENSION(pdf, $ext_shared)
        AC_DEFINE(HAVE_PDFLIB,1,[ ]) 
        AC_SUBST(PDFLIB_SHARED_LIBADD)
        PHP_ADD_LIBRARY_WITH_PATH(pdf, $withval/lib, PDFLIB_SHARED_LIBADD)
        PHP_ADD_INCLUDE($PHP_PDFLIB/include)
      else  
	AC_MSG_ERROR([pdflib.h not found under $PHP_PDFLIB/include/]) 
      fi ;;
  esac
