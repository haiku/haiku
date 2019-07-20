/**
 * @file sys/ioccom.h
 * @brief Definitions & maros common to ioctl
 */
 
#ifndef _SYS_IOCCOM_H
#define _SYS_IOCCOM_H

#include <features.h>

#ifdef _DEFAULT_SOURCE


/**
 * @defgroup IOCTL_common sys/ioccom.h
 * @brief Definitions & maros common to ioctl()
 * @ingroup OpenBeOS_POSIX
 * @ingroup IOCTL
 * Ioctl values passed as the command (2nd) variable have the 
 * command encoded in the lower word and the size of any parameters
 * in the upper word (in or out).
 * The high 3 bits are used to encode whether it's in or out.
 * Due to this you can't just give an ioctl value, you need to encode 
 * it using macros described in
 * @ref IOCTL_macros
 * @{
 */

/** @defgroup IOCTL_parm ioctl() parameter definitions
 * @ingroup IOCTL_common
 * @{
 */
/** @def IOC_VOID */
#define IOC_VOID         (ulong)0x20000000
/** @def IOC_OUT ioctl expects data (output) */
#define IOC_OUT          (ulong)0x40000000
/** @def IOC_IN ioctl passes a value in */
#define IOC_IN           (ulong)0x80000000
/** @def IOC_INOUT ioctl passes data in and out */
#define IOC_INOUT        (IOC_IN|IOC_OUT)
/** @def IOC_DIRMASK */
#define IOC_DIRMASK      (ulong)0xe0000000
/** @} */

/**
 * @defgroup IOCTL_macros IOCTL macros
 * These should be used to define the values passed in as cmd to
 * ioctl()
 * @ingroup IOCTL_common
 * @{
 */
/** @def IOCPARM_MASK mask used to for following macros */
#define IOCPARM_MASK     0x1fff
/** @def IOCPARM_LEN(x) length of the data passed as param */
#define IOCPARM_LEN(x)   (((x) >> 16) & IOCPARM_MASK)
/** @def IOCBASECMD(x) the base command encoded in the ioctl value */
#define IOCBASECMD(x)    ((x) & ~(IOCPARM_MASK << 16))
/** @def IOCGROUP(x) which group of ioctl() commands does this belong to? */
#define IOCGROUP(x)      (((x) >> 8) & 0xff)
/** @def IOCPARM_MAX Maximum size of parameter that can be passed (20 bytes) */ 
#define IOCPARM_MAX      20

/**
 * @defgroup IOCTL_createmacros macro's to create ioctl() values
 * @brief these macro's should be used to create new ioctl() values
 * @ingroup IOCTL_common
 * @{
 */
/** @def _IOC(inout, group, num , len) create a new ioctl */
#define _IOC(inout, group, num, len) \
	(inout | ((len & IOCPARM_MASK)<<16) | ((group) << 8) | (num))
/** @def _IO(g,n) create a new void ioctl for group g, number n */
#define _IO(g,n)          _IOC(IOC_VOID, (g), (n), 0)
/** @def _IOR(g,n,t) create a ioctl() that reads a value of type t*/
#define _IOR(g,n,t)       _IOC(IOC_OUT,  (g), (n), sizeof(t))
/** @def _IOW(g,n,t) ioctl() that writes value of type t, group g, number n */
#define _IOW(g,n,t)       _IOC(IOC_IN ,  (g), (n), sizeof(t))
/** @def _IOWR(g,n,t) ioctl() that reads/writes value of type t 
 * @note this isn't _IORW as this causes name conflicts on some systems */
#define _IOWR(g,n,t)      _IOC(IOC_INOUT,  (g), (n), sizeof(t))
/** @} */

/** @} */


#endif


#endif /* _SYS_IOCCOM_H */
