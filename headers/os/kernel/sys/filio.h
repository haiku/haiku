/**
 * @file sys/filio.h
 * @brief ioctl() definitions for file descriptor operations
 */

#ifndef _SYS_FILIO_H
#define _SYS_FILIO_H
/**
 * @defgroup IOCTL_filio sys/filio.h
 * @brief ioctl() definitions for file descriptor operations
 * @ingroup OpenBeOS_POSIX
 * @ingroup IOCTL
 * @{
 */

#include <sys/ioccom.h>

/** @def FIOCLEX set close on exec 
 * @ref FD_CLOEXEC */
#define FIOCLEX      _IO('f', 1)             
/** @def FIONCLEX remove close on exec flag 
 * @ref FD_CLOEXEC*/
#define FIONCLEX     _IO('f', 2)
/** @def FIBMAP get logical block 
 * @note this is not yet implemented on OpenBeOS */             
#define FIBMAP       _IOWR('f', 122, void *) 
/** @def FIOASYNC set/clear async I/O */
#define FIOASYNC     _IOW('f', 123, int)     
/** @def FIOGETOWN get owner */
#define FIOGETOWN    _IOR('f', 123, int)     
/** @def FIOSETOWN set owner */
#define FIOSETOWN    _IOW('f', 123, int)     
/** @def FIONBIO set/clear non-blocking I/O */
#define FIONBIO      _IOW('f', 126, int)     
/** @def FIONREAD get number of bytes available to read */
#define FIONREAD     _IOW('f', 127, int)     

/** @} */

#endif /* _SYS_FILIO_H */
