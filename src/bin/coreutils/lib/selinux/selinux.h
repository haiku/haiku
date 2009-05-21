#ifndef SELINUX_SELINUX_H
# define SELINUX_SELINUX_H

# include <sys/types.h>
# include <errno.h>

typedef unsigned short security_class_t;
# define security_context_t char*
# define is_selinux_enabled() 0

static inline int getcon (security_context_t *con _UNUSED_PARAMETER_)
  { errno = ENOTSUP; return -1; }
static inline void freecon (security_context_t con _UNUSED_PARAMETER_) {}


static inline int getfscreatecon (security_context_t *con _UNUSED_PARAMETER_)
  { errno = ENOTSUP; return -1; }
static inline int setfscreatecon (security_context_t con _UNUSED_PARAMETER_)
  { errno = ENOTSUP; return -1; }
static inline int matchpathcon (char const *s _UNUSED_PARAMETER_,
				mode_t m _UNUSED_PARAMETER_,
				security_context_t *con _UNUSED_PARAMETER_)
  { errno = ENOTSUP; return -1; }
static inline int getfilecon (char const *s _UNUSED_PARAMETER_,
			      security_context_t *con _UNUSED_PARAMETER_)
  { errno = ENOTSUP; return -1; }
static inline int lgetfilecon (char const *s _UNUSED_PARAMETER_,
			       security_context_t *con _UNUSED_PARAMETER_)
  { errno = ENOTSUP; return -1; }
static inline int setfilecon (char const *s _UNUSED_PARAMETER_,
			      security_context_t con _UNUSED_PARAMETER_)
  { errno = ENOTSUP; return -1; }
static inline int lsetfilecon (char const *s _UNUSED_PARAMETER_,
			       security_context_t con _UNUSED_PARAMETER_)
  { errno = ENOTSUP; return -1; }
static inline int fsetfilecon (int fd _UNUSED_PARAMETER_,
			       security_context_t con _UNUSED_PARAMETER_)
  { errno = ENOTSUP; return -1; }

static inline int security_check_context
    (security_context_t con _UNUSED_PARAMETER_)
  { errno = ENOTSUP; return -1; }
static inline int security_check_context_raw
    (security_context_t con _UNUSED_PARAMETER_)
  { errno = ENOTSUP; return -1; }
static inline int setexeccon (security_context_t con _UNUSED_PARAMETER_)
  { errno = ENOTSUP; return -1; }
static inline int security_compute_create
    (security_context_t scon _UNUSED_PARAMETER_,
     security_context_t tcon _UNUSED_PARAMETER_,
     security_class_t tclass _UNUSED_PARAMETER_,
     security_context_t *newcon _UNUSED_PARAMETER_)
  { errno = ENOTSUP; return -1; }
static inline int matchpathcon_init_prefix
    (char const *path _UNUSED_PARAMETER_,
     char const *prefix _UNUSED_PARAMETER_)
  { errno = ENOTSUP; return -1; }
#endif
