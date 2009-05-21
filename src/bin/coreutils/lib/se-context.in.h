#ifndef SELINUX_CONTEXT_H
# define SELINUX_CONTEXT_H

# include <errno.h>

typedef int context_t;
static inline context_t context_new (char const *s _UNUSED_PARAMETER_)
  { errno = ENOTSUP; return 0; }
static inline char *context_str (context_t con _UNUSED_PARAMETER_)
  { errno = ENOTSUP; return (void *) 0; }
static inline void context_free (context_t c _UNUSED_PARAMETER_) {}

static inline int context_user_set (context_t sc _UNUSED_PARAMETER_,
				    char const *s _UNUSED_PARAMETER_)
  { errno = ENOTSUP; return -1; }
static inline int context_role_set (context_t sc _UNUSED_PARAMETER_,
				    char const *s _UNUSED_PARAMETER_)
  { errno = ENOTSUP; return -1; }
static inline int context_range_set (context_t sc _UNUSED_PARAMETER_,
				     char const *s _UNUSED_PARAMETER_)
  { errno = ENOTSUP; return -1; }
static inline int context_type_set (context_t sc _UNUSED_PARAMETER_,
				    char const *s _UNUSED_PARAMETER_)
  { errno = ENOTSUP; return -1; }

#endif
