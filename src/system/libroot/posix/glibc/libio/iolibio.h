#include <libio.h>

/* These emulate stdio functionality, but with a different name
   (_IO_ungetc instead of ungetc), and using _IO_FILE instead of FILE. */

#ifdef __cplusplus
extern "C" {
#endif

extern int _IO_fclose (_IO_FILE*);
extern int _IO_new_fclose (_IO_FILE*);
extern int _IO_old_fclose (_IO_FILE*);
extern _IO_FILE *_IO_fdopen (int, const char*) __THROW;
libc_hidden_proto (_IO_fdopen)
extern _IO_FILE *_IO_old_fdopen (int, const char*) __THROW;
extern _IO_FILE *_IO_new_fdopen (int, const char*) __THROW;
extern int _IO_fflush (_IO_FILE*);
libc_hidden_proto (_IO_fflush)
extern int _IO_fgetpos (_IO_FILE*, _IO_fpos_t*);
extern int _IO_fgetpos64 (_IO_FILE*, _IO_fpos64_t*);
extern char* _IO_fgets (char*, int, _IO_FILE*);
extern _IO_FILE *_IO_fopen (const char*, const char*);
extern _IO_FILE *_IO_old_fopen (const char*, const char*);
extern _IO_FILE *_IO_new_fopen (const char*, const char*);
extern _IO_FILE *_IO_fopen64 (const char*, const char*);
extern _IO_FILE *__fopen_internal (const char*, const char*, int);
extern _IO_FILE *__fopen_maybe_mmap (_IO_FILE *) __THROW;
extern int _IO_fprintf (_IO_FILE*, const char*, ...);
extern int _IO_fputs (const char*, _IO_FILE*);
libc_hidden_proto (_IO_fputs)
extern int _IO_fsetpos (_IO_FILE*, const _IO_fpos_t *);
extern int _IO_fsetpos64 (_IO_FILE*, const _IO_fpos64_t *);
extern long int _IO_ftell (_IO_FILE*);
libc_hidden_proto (_IO_ftell)
extern _IO_size_t _IO_fread (void*, _IO_size_t, _IO_size_t, _IO_FILE*);
libc_hidden_proto (_IO_fread)
extern _IO_size_t _IO_fwrite (const void*, _IO_size_t, _IO_size_t, _IO_FILE*);
libc_hidden_proto (_IO_fwrite)
extern char* _IO_gets (char*);
extern void _IO_perror (const char*) __THROW;
extern int _IO_printf (const char*, ...);
extern int _IO_puts (const char*);
extern int _IO_scanf (const char*, ...);
extern void _IO_setbuffer (_IO_FILE *, char*, _IO_size_t) __THROW;
libc_hidden_proto (_IO_setbuffer)
extern int _IO_setvbuf (_IO_FILE*, char*, int, _IO_size_t) __THROW;
libc_hidden_proto (_IO_setvbuf)
extern int _IO_sscanf (const char*, const char*, ...) __THROW;
extern int _IO_sprintf (char *, const char*, ...) __THROW;
extern int _IO_ungetc (int, _IO_FILE*) __THROW;
extern int _IO_vsscanf (const char *, const char *, _IO_va_list) __THROW;
extern int _IO_vsprintf (char*, const char*, _IO_va_list) __THROW;
libc_hidden_proto (_IO_vsprintf)
extern int _IO_vswprintf (wchar_t*, _IO_size_t, const wchar_t*, _IO_va_list)
       __THROW;

struct obstack;
extern int _IO_obstack_vprintf (struct obstack *, const char *, _IO_va_list)
       __THROW;
extern int _IO_obstack_printf (struct obstack *, const char *, ...) __THROW;
#ifndef _IO_pos_BAD
#define _IO_pos_BAD ((_IO_off64_t)(-1))
#endif
#define _IO_clearerr(FP) ((FP)->_flags &= ~(_IO_ERR_SEEN|_IO_EOF_SEEN))
#define _IO_fseek(__fp, __offset, __whence) \
  (_IO_seekoff_unlocked (__fp, __offset, __whence, _IOS_INPUT|_IOS_OUTPUT) \
   == _IO_pos_BAD ? EOF : 0)
#define _IO_rewind(FILE) \
  (void) _IO_seekoff_unlocked (FILE, 0, 0, _IOS_INPUT|_IOS_OUTPUT)
#define _IO_vprintf(FORMAT, ARGS) \
  _IO_vfprintf (_IO_stdout, FORMAT, ARGS)
#define _IO_freopen(FILENAME, MODE, FP) \
  (_IO_file_close_it (FP), \
   _IO_file_fopen (FP, FILENAME, MODE, 1))
#define _IO_old_freopen(FILENAME, MODE, FP) \
  (_IO_old_file_close_it (FP), _IO_old_file_fopen(FP, FILENAME, MODE))
#define _IO_freopen64(FILENAME, MODE, FP) \
  (_IO_file_close_it (FP), \
   _IO_file_fopen (FP, FILENAME, MODE, 0))
#define _IO_fileno(FP) ((FP)->_fileno)
extern _IO_FILE* _IO_popen (const char*, const char*) __THROW;
extern _IO_FILE* _IO_new_popen (const char*, const char*) __THROW;
extern _IO_FILE* _IO_old_popen (const char*, const char*) __THROW;
extern int __new_pclose (_IO_FILE *) __THROW;
extern int __old_pclose (_IO_FILE *) __THROW;
#define _IO_pclose _IO_fclose
#define _IO_setbuf(_FP, _BUF) _IO_setbuffer (_FP, _BUF, _IO_BUFSIZ)
#define _IO_setlinebuf(_FP) _IO_setvbuf (_FP, NULL, 1, 0)

_IO_FILE *__new_freopen (const char *, const char *, _IO_FILE *) __THROW;
_IO_FILE *__old_freopen (const char *, const char *, _IO_FILE *) __THROW;

#ifdef __cplusplus
}
#endif
