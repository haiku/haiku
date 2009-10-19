dnl $Id: config.php-406+.m4 14574 2005-10-29 16:27:43Z bonefish $

PHP_ARG_WITH(pdflib,whether to include PDFlib support,
[  --with-pdflib[=DIR]     Include PDFlib 4.x support. DIR is the PDFlib
                          base install directory, defaults to /usr/local
                          Set DIR to "shared" to build as dl, or "shared,DIR"
                          to build as dl and still specify DIR.])

  PHP_SUBST(PDFLIB_SHARED_LIBADD)
  PHP_EXTENSION(pdf, $ext_shared)

  case "$PHP_PDFLIB" in
    yes)
      AC_CHECK_LIB(pdf, PDF_open_pdi, [
        AC_DEFINE(HAVE_PDFLIB,1,[ ])
	PHP_ADD_LIBRARY(pdf,, PDFLIB_SHARED_LIBADD)
      ],[
        AC_MSG_ERROR(PDFlib extension requires PDFlib 4.x.)
      ])
        PHP_ADD_LIBRARY(pdf,, PDFLIB_SHARED_LIBADD)
      ;;
    no)
      ;;
    *)
      test -f $PHP_PDFLIB/include/pdflib.h && PDFLIB_INCLUDE="$PHP_PDFLIB/include"
      if test -n "$PDFLIB_INCLUDE" ; then
        PHP_CHECK_LIBRARY(pdf, PDF_open_pdi, [ 
          AC_DEFINE(HAVE_PDFLIB,1,[ ]) 
          PHP_ADD_LIBRARY_WITH_PATH(pdf, $PHP_PDFLIB/lib, PDFLIB_SHARED_LIBADD) 
	  PHP_ADD_INCLUDE($PDFLIB_INCLUDE)
        ],[
          AC_MSG_ERROR(PDFlib extension requires PDFlib 4.x.)
        ],[
	  -L$PHP_PDFLIB/lib
        ])
      else  
	AC_MSG_ERROR([pdflib.h not found under $PHP_PDFLIB/include/]) 

      fi ;;
  esac
