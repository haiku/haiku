#include <libio/libio.h>

#ifndef _LIBC_LIBIO_H
#define _LIBC_LIBIO_H

libc_hidden_proto (__overflow)
libc_hidden_proto (__underflow)
libc_hidden_proto (__uflow)
libc_hidden_proto (__woverflow)
libc_hidden_proto (__wunderflow)
libc_hidden_proto (__wuflow)

#if defined _IO_MTSAFE_IO && _IO_lock_inexpensive
# undef _IO_flockfile
# define _IO_flockfile(_fp) \
  if (((_fp)->_flags & _IO_USER_LOCK) == 0)				      \
     _IO_lock_lock (*(_fp)->_lock)
# undef _IO_funlockfile
# define _IO_funlockfile(_fp) \
  if (((_fp)->_flags & _IO_USER_LOCK) == 0)				      \
    _IO_lock_unlock (*(_fp)->_lock)
#endif

#endif
