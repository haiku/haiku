/***********************************************************************
 *                                                                     *
 * $Id: hpgsglobal.c 338 2006-06-17 08:33:36Z softadm $
 *                                                                     *
 * hpgs - HPGl Script, a hpgl/2 interpreter, which uses a Postscript   *
 *        API for rendering a scene and thus renders to a variety of   *
 *        devices and fileformats.                                     *
 *                                                                     *
 * (C) 2004-2006 ev-i Informationstechnologie GmbH  http://www.ev-i.at *
 *                                                                     *
 * Author: Wolfgang Glas                                               *
 *                                                                     *
 *  hpgs is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU Lesser General Public          *
 * License as published by the Free Software Foundation; either        *
 * version 2.1 of the License, or (at your option) any later version.  *
 *                                                                     *
 * hpgs is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of      *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   *
 * Lesser General Public License for more details.                     *
 *                                                                     *
 * You should have received a copy of the GNU Lesser General Public    *
 * License along with this library; if not, write to the               *
 * Free Software  Foundation, Inc., 59 Temple Place, Suite 330,        *
 * Boston, MA  02111-1307  USA                                         *
 *                                                                     *
 ***********************************************************************
 *                                                                     *
 * Global resources of HPGS.                                           *
 *                                                                     *
 ***********************************************************************/

#include <hpgsdevices.h>
#include <hpgsreader.h>
#include <hpgsfont.h>
#include <hpgsi18n.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#ifdef WIN32
#include <windows.h>

static DWORD tls_handle = -1;
/* Use a separate heap for the error messages. This heap is
   destroyed in hpgs_cleanup, so we risk no lost pointers. */
static HANDLE hHeap = 0;

/*! A counterpart of vsprintf, which allocates the result string
    on the heap an is save against buffer overflows.

    Returns 0, if the system is out of memory.
*/
static char* vsprintf_hHeap (const char *fmt, va_list ap)
{
  /* Guess we need no more than 1024 bytes. */
  int n, size = 1024;
  char *p;
  if ((p = HeapAlloc (hHeap,0,size)) == NULL)
    return p;

  while (1)
    {
      /* Try to print in the allocated space. */
      n = _vsnprintf (p, size, fmt, ap);
      /* If that worked, return the string. */
      if (n > -1 && n < size)
	{ return p; }
      /* Else try again with more space. */
      if (n > -1)    /* glibc 2.1 */
	size = n+1; /* precisely what is needed */
      else           /* glibc 2.0 */
	size *= 2;  /* twice the old size */

      {
        char *np = HeapReAlloc (hHeap,0,p,size);
      
        if (!np) { HeapFree(hHeap,0,p); return 0; }
        p = np;
      }
    }
}

static char* sprintf_hHeap (const char *fmt, ...)
{
  va_list ap;
  char *ret;

  va_start(ap, fmt);
  ret = vsprintf_hHeap(fmt,ap);
  va_end(ap);

  return ret;
}

#else
#include <pthread.h>

static pthread_key_t key;
#endif

static char *hpgs_reader_prefix=0;

/*! This function has to be called before using any other
    function of hpgs. It defines the path, where additional
    files such as the truetype files for HPGL files or plugins
    are stored.

    Truetype fonts are searched in the path <prefix>/share/hpgs

    The path is internally duplicated using \c strdup.
*/
void hpgs_init(const char *path)
{
  int l;

  hpgs_reader_prefix = strdup(path);

  if (!hpgs_reader_prefix) return;

  // strip off trailing directory separators.  
  l = strlen(hpgs_reader_prefix);

  if (l && hpgs_reader_prefix[l-1] == HPGS_PATH_SEPARATOR)
    hpgs_reader_prefix[l-1] = '\0';

#ifdef WIN32
  hHeap = HeapCreate(0,4096,128*1024);
  tls_handle = TlsAlloc();
#else
  pthread_key_create (&key,free);
#endif
#ifdef HPGS_HAVE_GETTEXT
  hpgs_i18n_init();
#endif
  hpgs_font_init();
}

/*! This function return a pointer to the installation prefix
    set through \c hpgs_init. The returned string has any trailing
    directory separator of the string passed to \c hpgs_init stripped
    off.
*/
HPGS_API const char *hpgs_get_prefix()
{
  if (hpgs_reader_prefix)
    return hpgs_reader_prefix;

#ifdef WIN32
  return "C:\\Programme\\EV-i";
#else
#ifndef HPGS_DEFAULT_PREFIX
  return "/usr/local";
#else
  return HPGS_DEFAULT_PREFIX;
#endif
#endif
}

/*! This function has to be called atfer the last usage of hpgs.
    It cleans up internally allocated resources.
*/
void hpgs_cleanup()
{
  hpgs_font_cleanup();

  hpgs_cleanup_plugin_devices();

  if (hpgs_reader_prefix)
    {
      free(hpgs_reader_prefix);
      hpgs_reader_prefix = 0;
    }
#ifdef WIN32
  TlsFree(tls_handle);
  HeapDestroy(hHeap);
  hHeap=0;
  tls_handle=-1;
#else
  hpgs_clear_error();
  pthread_key_delete (key);
#endif
}

/*! Get the value of the command line argument argv[0],
    if the argument starts with \c opt.  

    This subroutine is useful when parsing command lines
    like in the following loop:

\verbatim 
int main(int argc, const char *argv[])
{
  while (argc>1)
    {
      int narg = 1;
      const char *value;
      
      if (hpgs_get_arg_value("-r",argv,&value,&narg))
	{
          ...parse value of option -r...
	}
      else if (hpgs_get_arg_value("-p",argv,&value,&narg))
	{
	  ...parse value of option -p...
	}
      else
	{
	  if (argv[0][0] == '-')
	    {
	      fprintf(stderr,"Error: Unknown option %s given.\n",argv[0]);
	      return usage();
	    }
          break;
	}

      argv+=narg;
      argc-=narg;
    }

\endverbatim

    Returns \c HPGS_TRUE, if argv[0] starts with opt.
    If \c *narg == 2, argv[0] is exactly \c opt and
    \c *value points to \c argv[1].
    If \c *narg == 1, argv[0] startwith \c opt and
    \c *value points to \c argv[0]+strlen(opt).

    Returns \c HPGS_FALSE, if argv[0] does not start with \c opt.
*/
hpgs_bool hpgs_get_arg_value(const char *opt, const char *argv[],
                             const char **value, int *narg)
{
  int l = strlen(opt);
  if (strncmp(opt,argv[0],l)!=0)
    return 0;

  if (l == strlen(argv[0]))
    {
      *value = argv[1];
      *narg = 2;
    }
  else
    {
      *value = argv[0] + l;
      *narg = 1;
    }

  return 1;
}

/*! This subroutine encapsulates the standard C library function \c realloc.
    This is needed in order to aviod hassels when \c realloc returns
    a null pointer and the original pointer is still needed for subsequent
    deallocations.

    \c itemsz is the size of the individual items in the array.
    \c pptr is a pointer to the pointer holding te address of the array.
            On success, \c *pptr points to the newly allocated array, otherwise
            \c *pptr is untouched.
    \c psz is a pointer to the allocated size of the array .
            On success, *psz hold the new size \c nsz of the array,
            otherwise \c *psz is untouched.

    Returns 0 upon success or -1 if the system is out of memory.
*/
int hpgs_array_safe_resize (size_t itemsz, void **pptr, size_t *psz, size_t nsz)
{
  void *np = realloc(*pptr,nsz*itemsz);
  if (!np) return -1;
  *pptr=np;
  *psz=nsz;
  return 0;
}

/*! A counterpart of vsprintf, which allocates the result string
    on the heap an is save against buffer overflows.

    Returns 0, if the system is out of memory.
*/
char* hpgs_vsprintf_malloc (const char *fmt, va_list ap)
{
  /* Guess we need no more than 1024 bytes. */
  int n, size = 1024;
  char *p;
  if ((p = malloc (size)) == NULL)
    return p;

  while (1)
    {
      /* Try to print in the allocated space. */
      va_list aq;
      va_copy(aq, ap);      
#ifdef WIN32
      n = _vsnprintf (p, size, fmt, aq);
#else
      n = vsnprintf (p, size, fmt, aq);
#endif
      va_end(aq);
      /* If that worked, return the string. */
      if (n > -1 && n < size)
	{ return p; }
      /* Else try again with more space. */
      if (n > -1)    /* glibc 2.1 */
	size = n+1; /* precisely what is needed */
      else           /* glibc 2.0 */
	size *= 2;  /* twice the old size */

      {
        char *np = realloc (p, size);
      
        if (!np) { free(p); return 0; }
        p = np;
      }
    }
}

/*! A counterpart of sprintf, which allocates the result string
    on the heap an is save against buffer overflows.

    Returns 0, if the system is out of memory.
*/
char* hpgs_sprintf_malloc (const char *fmt, ...)
{
  va_list ap;
  char *ret;

  va_start(ap, fmt);
  ret = hpgs_vsprintf_malloc(fmt,ap);
  va_end(ap);

  return ret;
}

/*! Constructs a filename on the heap for a file located in the
    installation path passed to \c hpgs_init.

    Returns 0, if the system is out of memory.
*/
char *hpgs_share_filename(const char *rel_filename)
{
  return hpgs_sprintf_malloc("%s%cshare%chpgs%c%s",
			     hpgs_get_prefix(),
			     HPGS_PATH_SEPARATOR,
			     HPGS_PATH_SEPARATOR,
			     HPGS_PATH_SEPARATOR,
                             rel_filename);
}

/*! Set the error message of the current thread to the given string.

    Always returns -1.
*/
int hpgs_set_error(const char *fmt, ...)
{
  va_list ap;
  int ret;

  va_start(ap, fmt);
  ret = hpgs_set_verror(fmt,ap);
  va_end(ap);

  return ret;
}

/*! Prepends the error message of the current thread with the given string.
    this is useful, if you want to push context information
    to the error message.

    Always returns -1.
*/
int hpgs_error_ctxt(const char *fmt, ...)
{
  va_list ap;
  int ret;

  va_start(ap, fmt);
  ret = hpgs_verror_ctxt(fmt,ap);
  va_end(ap);

  return ret;
}

/*! Set the error message of the current thread to the given string.

    Always returns -1.
*/
int hpgs_set_verror(const char *fmt, va_list ap)
{
  char *x;
#ifdef WIN32
  x = (char *)TlsGetValue(tls_handle);
  if (x) HeapFree(hHeap,0,x);
  x = vsprintf_hHeap(fmt,ap);
  TlsSetValue(tls_handle,x);
#else
  x = (char *)pthread_getspecific (key);
  if (x) free(x);

  va_list aq;
  va_copy(aq, ap);      
  x = hpgs_vsprintf_malloc(fmt,aq);
  va_end(aq);

  pthread_setspecific (key,x);
#endif
  return -1;
}

/*!
   Clears the error message of the current thread.
*/
void hpgs_clear_error()
{
  char *x;
#ifdef WIN32
  x = (char *)TlsGetValue(tls_handle);
  if (x) HeapFree(hHeap,0,x);
  TlsSetValue(tls_handle,0);
#else
  x = (char *)pthread_getspecific (key);
  if (x) free(x);
  pthread_setspecific (key,0);
#endif
}

/*!
   Checks, whther the error message of the current thread has been set.
*/
HPGS_API hpgs_bool hpgs_have_error()
{
  char *x;
#ifdef WIN32
  x = (char *)TlsGetValue(tls_handle);
#else
  x = (char *)pthread_getspecific (key);
#endif
  return x != 0;
}

/*! Prepends the error message of the current thread with the given string.
    this is useful, if you want to push context information
    to the error message.

    Always returns -1.
*/
int hpgs_verror_ctxt(const char *fmt, va_list ap)
{
  char *x;
#ifdef WIN32
  x = (char *)TlsGetValue(tls_handle);

  if (!fmt) return -1;

  char * new_p = vsprintf_hHeap(fmt,ap);

  if (!new_p) return -1;

  if (x)
    {
      char *tmp = sprintf_hHeap("%s: %s",new_p,x);
      HeapFree(hHeap,0,new_p);

      if (!tmp) return -1;

      HeapFree(hHeap,0,x);
      new_p = tmp;
    }

  TlsSetValue(tls_handle,new_p);

#else
  x = (char *)pthread_getspecific (key);

  char * new_p = hpgs_vsprintf_malloc(fmt,ap);

  if (!new_p) return -1;

  if (x)
    {
      char *tmp = hpgs_sprintf_malloc("%s: %s",new_p,x);
      free(new_p);

      if (!tmp) return -1;

      free(x);
      new_p = tmp; 
    }

  pthread_setspecific (key,new_p);
#endif
  return -1;
}

/*!
   Get the error message of the current thread.
*/
const char *hpgs_get_error()
{
  const char *ret;
#ifdef WIN32
  ret = (char *)TlsGetValue(tls_handle);
#else
  ret = (char *)pthread_getspecific (key);
#endif

  if (!ret) ret = hpgs_i18n("(no error)");
  return ret;
}

static void default_hpgs_logger(const char *fmt, va_list ap)
{
  va_list aq;
  va_copy(aq, ap);      
  vfprintf(stderr,fmt,aq);
  va_end(ap);
}

static hpgs_logger_func_t static_logger = default_hpgs_logger;

/*!
  Set the logging function for logging informational messages.
  If \c func is NULL, the default logging function, which log to
  \c stderr is restored.

  Warning: This function is not thread-safe and should be called
           just after \c hpgs_init() at the beginning of the program.
*/
void hpgs_set_logger(hpgs_logger_func_t func)
{
  if (func)
    static_logger = func;
  else
    static_logger = default_hpgs_logger;
}

/*!
  Logs an informational message using the function set with
  \c hpgs_set_logger(). If no function is set with \c hpgs_set_logger(),
  the message is printed to \c stderr.
*/
void hpgs_log(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  static_logger(fmt,ap);
  va_end(ap);
}

/*!
  Logs an informational message using the function set with
  \c hpgs_set_logger(). If no function is set with \c hpgs_set_logger(),
  the message is printed to \c stderr.
*/
void hpgs_vlog(const char *fmt, va_list ap)
{
  va_list aq;
  va_copy(aq, ap);      
  static_logger(fmt,aq);
  va_end(aq);
}

/*!
   Returns the character code at the given position in an utf-8 string.
   The pointer \c (*p) is advanced to the next character after the current
   position.
*/
int hpgs_next_utf8(const char **p)
{
  int ret;

  if (((unsigned char)**p & 0x80) == 0)
    ret = **p;
  else
    {
      // Hit in the middle of a multibyte sequence
      // -> return EOS.
      if (((unsigned char)**p & 0xC0) == 0x80)
        ret = 0;
      else
        {
          // true multibyte sequence
          int mask = 0x20;
          int need = 2;
          ret = 0x1f;

          while (**p & mask)
            { ret >>= 1; mask >>=1; ++need; }

          ret &= **p;

          while (--need)
            {
              ++(*p);
              if (((**p) & 0xc0) != 0x80) { return 0; }
              ret = (ret << 6) | ((**p) & 0x3f);
            }
        }
    }

  ++(*p);
  return ret;
}

/*!
   Returns the number of unicode characters contained in the first \c n bytes
   of the given utf-8 string. If the null character is encountered, the
   interpretation is stopped before the n'th byte.

   if \c n == -1, the string is ultimatively search up to the first occurrence
   of the null character.
 
   The pointer \c (*p) is advanced to the next character after the current
   position.
*/
HPGS_API int hpgs_utf8_strlen(const char *p, int n)
{
  const char **pp = &p;

  int ret = 0;

  while ((n<0 || ((*pp) - p) < n) && hpgs_next_utf8(pp) != 0)
    ++ret;
         
  return ret;
}
