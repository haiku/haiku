/*-
 * Copyright (c) 1983, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)fcntl.h	8.3 (Berkeley) 1/21/94
 */

/**
 * @file fcntl.h
 * @brief File Control functions and definitions
 */
 
#ifndef _FCNTL_H_
#define	_FCNTL_H_

/**
 * @defgroup Fcntl fcntl.h 
 * @brief File Control functions and definitions
 * @note Definitions in this file also apply to open() and 
 *       internally in the kernel
 * @note Relevant kernel functions should be here as well
 * @ingroup OpenBeOS_POSIX
 * @{
 */

#ifndef _KERNEL_
//#include <sys/types.h>
#endif


/**
 * @defgroup File_Status_Flags File Status Flags
 * The flags described in this section are used by fcntl(), open() and also
 * withing the kernel.
 * @note O_ are used by fcntl() and open()
 * @note F flags are used by the kernel
 * @ingroup fcntl
 * @{
 */
/** open for reading only */
#define	O_RDONLY	0x0000
/** open for writing only */
#define	O_WRONLY	0x0001		
/** open for reading and writing */
#define	O_RDWR		0x0002
/** mask for above modes */
#define	O_ACCMODE	0x0003		

#ifndef _POSIX_SOURCE
  /** kernel flag to allow testing of read/write directly
   * @note not #ifdef _KERNEL_ to allow TIOCFLUSH to work */
  #define	FREAD		0x0001
  /** kernel flag to allow testing of read/write directly
   * @note not #ifdef _KERNEL_ to allow TIOCFLUSH to work */
  #define	FWRITE		0x0002
#endif

/** no delay */
#define	O_NONBLOCK	0x0004		
/** set append mode */
#define	O_APPEND	0x0008		
/** open with shared file lock */
#define	O_SHLOCK	0x0010		
/** open with exclusive file lock */
#define	O_EXLOCK	0x0020		
/** signal pgrp when data ready */
#define	O_ASYNC		0x0040		
/** backwards compatibility */
#define	O_FSYNC		O_SYNC		
/** if path is a symlink, don't follow */
#define	O_NOFOLLOW	0x0100		
/** synchronous writes */
#define	O_SYNC		0x0080		
/** create if nonexistant */
#define	O_CREAT		0x0200		
/** truncate to zero length */
#define	O_TRUNC		0x0400		
/** error if already exists */
#define	O_EXCL		0x0800		

#ifdef _KERNEL_
  /* mark during gc() */
  #define	FMARK		0x1000
  /* defer for next gc pass */
  #define	FDEFER		0x2000		
  /* descriptor holds advisory lock */
  #define	FHASLOCK	0x4000		
#endif

/*
 * POSIX 1003.1 specifies a higher granularity for syncronous operations
 * than are currently supported. We may want to look at what needs to be done
 * to support these. At present just map them.
 */
/** @def O_DSYNC synchronous data writes */
#define	O_DSYNC		O_SYNC		
/** @def O_RSYNC synchronous reads */
#define	O_RSYNC		O_SYNC		

/* defined by POSIX 1003.1; BSD default, this bit is not required */
/** @def O_NOCTTY don't assign controlling terminal */
#define	O_NOCTTY	0x8000		

#ifdef _KERNEL_
  /*
   * convert from open() flags to/from fflags; convert O_RD/WR to FREAD/FWRITE.
   * For out-of-range values for the flags, be slightly careful (but lossy).
   */
  /** @def FFLAGS(oflags) Convert from O_ flags to F flags for kernel */
  #define	FFLAGS(oflags)	(((oflags) & ~O_ACCMODE) | (((oflags) + 1) & O_ACCMODE))
  /** @def OFLAGS(fflags) Convert from F flags to O_ flags for open() */
  #define	OFLAGS(fflags)	(((fflags) & ~O_ACCMODE) | (((fflags) - 1) & O_ACCMODE))

  /** @def FMASK bits to save after open */
  #define	FMASK		(FREAD|FWRITE|FAPPEND|FASYNC|FFSYNC|FNONBLOCK)
  /** @def FCNTLFLAGS bits settable by fcntl(F_SETFL, ...) */
  #define	FCNTLFLAGS	(FAPPEND|FASYNC|FFSYNC|FNONBLOCK)
#endif

/*
 * The O_* flags used to have only F* names, which were used in the kernel
 * and by fcntl.  We retain the F* names for the kernel f_flags field
 * and for backward compatibility for fcntl.
 */
#ifndef _POSIX_SOURCE
  /** kernel/compat */
  #define	FAPPEND		O_APPEND	
  /** kernel/compat */
  #define	FASYNC		O_ASYNC		
  /** kernel */
  #define	FFSYNC		O_SYNC		
  /** kernel */
  #define	FNONBLOCK	O_NONBLOCK	
  /** compat */
  #define	FNDELAY		O_NONBLOCK	
  /** compat */
  #define	O_NDELAY	O_NONBLOCK	
#endif

/** @} */

/**
 * @defgroup FCNTL_Flags Flags used for fcntl()
 * @ingroup FCNTL
 * @{
 */
 
/** 
 * @def F_DUPFD Return a new file descriptor that has
 * @li the lowest number available (>= fd used)
 * @li same object references as original fd
 * @li same offsets as original (file only)
 * @li same file status
 * @li close-on-exec flag is set so it will remain open across execv()
 *
 * @ref FD_CLOEXEC
 */
#define	F_DUPFD		0		/* duplicate file descriptor */
/** @def F_GETFD Get the close-on-exec flag for the fd 
 * @note the argument is ignored
 * @ref FD_CLOEXEC */
#define	F_GETFD		1
/** @def F_SETFD Set the close-on-exec flag for the fd 
 * @ref FD_CLOEXEC */
#define	F_SETFD		2
/** @def F_GETFL Get fd status flags */
#define	F_GETFL		3
/** @def F_SETFL Set the fd status flags */
#define	F_SETFL		4
/** @def F_GETOWN Get the process ID receiving SIGIO/SIGURG signals */
#define	F_GETOWN	5
/** @def F_SETOWN Set the process ID receiving SIGIO/SIGURG signals */
#define F_SETOWN	6
/** @def F_GETLK Get the first lock that blocks the requested lock requested
 *               int the flock structure passed in. 
 * @li If the call succeeds then the structure is returned unaltered except
 *     the lock type is set to F_UNLCK
 * @li If the call fails the details of the blocking lock will be inserted
 *     into the strcture, overwriting the details submitted.
 */
#define	F_GETLK		7
/** @def F_SETLK Set/Clear a file segment lock based on the flock structure
 *               passed in.
 * @note If a shared/exclusive lock cannot be set, EAGAIN will be returned
 */ 
#define	F_SETLK		8
/** @def F_SETLKW Same as F_SETLK but will block if a shared/exclusive
 *                lock is requested and is not available.
 */
#define	F_SETLKW	9

/* file descriptor flags (F_GETFD, F_SETFD) */
/** @def FD_CLOEXEC Flag set in status is fd is set to remain open
  *                 across exec 
  * @ref F_DUPFD
  * @ref F_GETFD
  * @ref F_SETFD
  */
#define	FD_CLOEXEC	1		/* close-on-exec flag */
/** @} */

/**
 * @defgroup FCNTL_FileLocks Locking types for fcntl()
 * @ingroup FCNTL
 * @{
 */

/** shared or read lock */
#define	F_RDLCK		1		
/** unlock */
#define	F_UNLCK		2		
/** exclusive or write lock */
#define	F_WRLCK		3		

#ifdef _KERNEL_
  /** Wait until lock is granted */
  #define	F_WAIT		0x010		
  /** Use flock(2) semantics for lock */
  #define	F_FLOCK		0x020	 	
  /** Use POSIX semantics for lock */
  #define	F_POSIX		0x040	 	
#endif

/** @} */

/**
 * Advisory file segment locking data type -
 * information passed to system by user
 * @note This structure can be used to lock all or part of a file
 * @note Not yet implemented in OpenBeOS
 */
struct flock {
	/** starting offset for lock */
	off_t	l_start;
	/** length of lock required (0 means lock to end of file) */
	off_t	l_len;
	/** process id of process owning lock */
	pid_t	l_pid;
	/** type of lock 
	 * @link FCNTL_FileLocks */
	short	l_type;
	/** type of start poition given */
	short	l_whence;
};


/**
 * @defgroup FCNTL_flock Locking types for flock()
 * @ingroup FCNTL
 * @{
 */
/** shared lock */ 
#define	LOCK_SH		0x01        
/** exclusive lock */
#define	LOCK_EX		0x02		
/** don't block when locking */
#define	LOCK_NB		0x04		
/** unlock */
#define	LOCK_UN		0x08
/** @} */


#ifndef _KERNEL_MODE
  //#include <sys/cdefs.h>
  #include <ktypes.h>
  
  /** @fn int open(const char *path, int oflags, ...);
   * Used to open or create a file for reading/writing
   * @note oflags passed should be OR'd together, e.g.
   * @code
   *    int fd = open("file.txt", O_RDWR | O_APPEND | O_CREAT);
   * @endcode
   * @note if the flag O_CREAT is supplied and the file given by path doesn't
   *       exist it will be created.
   *
   * @ref File_Status_Flags
   */
  int	open (const char *, int, ...);

  /** @fn int creat(const char *path, mode_t mode)
   * Creates a file.
   * @note Obsoleted by open()
   * @code
   * Same as doing open(path, O_CREAT | O_TRUNC | O_WRONLY, mode);
   * @endcode
   * @ref open()
   */
  int	creat (const char *, mode_t);

  /** @fn int fcntl(int fd, int cmd, ...)
   * Provides control over the properties of a descriptor that is
   * already open. The 3rd paramater is technically a void *, but may
   * be interpretted as an int by some commands or cast by others.
   * @ref FCNTL_Flags should be used as the cmd parameter
   */
  int	fcntl (int, int, ...);

  /** @fn int flock(int fd, int operation) 
   * Applies or removes an advisory lock from descriptor fd.
   * @ref FCNTL_flock codes should be used as the operation parameter
   */
  int	flock (int, int);
#endif

/** @} */

#endif /* !_SYS_FCNTL_H_ */

