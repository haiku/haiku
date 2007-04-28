#ifndef _FSSH_ERRNO_H
#define _FSSH_ERRNO_H

#ifdef __cplusplus
extern "C"
{
#endif  

#include "fssh_errors.h"

#define FSSH_ENOERR          0
#define FSSH_EOK 			FSSH_ENOERR  /* some code assumes EOK exists */

extern int *_fssh_errnop(void);
#define fssh_errno (*(_fssh_errnop()))

extern int	fssh_get_errno(void);
extern void	fssh_set_errno(int error);

extern int	fssh_to_host_error(int error);

#ifdef __cplusplus
} /* "C" */
#endif 

#endif /* _FSSH_ERRNO_H */
