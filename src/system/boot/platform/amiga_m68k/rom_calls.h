/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		François Revol, revol@free.fr.
 * 
 */
#ifndef _AMICALLS_H
#define _AMICALLS_H


#ifdef __cplusplus
extern "C" {
#endif

#ifndef __ASSEMBLER__
#include <OS.h>
#include <SupportDefs.h>

/*
   General macros for Amiga function calls. Not all the possibilities have
   been created - only the ones which exist in OS 3.1. Third party libraries
   and future versions of AmigaOS will maybe need some new ones...

   LPX - functions that take X arguments.

   Modifiers (variations are possible):
   NR - no return (void),
   A4, A5 - "a4" or "a5" is used as one of the arguments,
   UB - base will be given explicitly by user (see cia.resource).
   FP - one of the parameters has type "pointer to function".

   "bt" arguments are not used - they are provided for backward compatibility
   only.
*/
/* those were taken from fd2pragma, but no copyright seems to be claimed on them */

#define LP0(offs, rt, name, bt, bn)				\
({								\
   ({								\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register rt _##name##_re __asm("d0");			\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : "r" (_##name##_bn)					\
      : "fp0", "fp1", "cc", "memory");				\
      _##name##_re;						\
   });								\
})

#define LP0NR(offs, name, bt, bn)				\
({								\
   {								\
      register int _d0 __asm("d0");				\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : "r" (_##name##_bn)					\
      : "fp0", "fp1", "cc", "memory");				\
   }								\
})

#define LP1(offs, rt, name, t1, v1, r1, bt, bn)			\
({								\
   t1 _##name##_v1 = (v1);					\
   ({								\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register rt _##name##_re __asm("d0");			\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : "r" (_##name##_bn), "rf"(_n1)				\
      : "fp0", "fp1", "cc", "memory");				\
      _##name##_re;						\
   });								\
})

#define LP1NR(offs, name, t1, v1, r1, bt, bn)			\
({								\
   t1 _##name##_v1 = (v1);					\
   {								\
      register int _d0 __asm("d0");				\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : "r" (_##name##_bn), "rf"(_n1)				\
      : "fp0", "fp1", "cc", "memory");				\
   }								\
})

/* Only graphics.library/AttemptLockLayerRom() */
#define LP1A5(offs, rt, name, t1, v1, r1, bt, bn)		\
({								\
   t1 _##name##_v1 = (v1);					\
   ({								\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register rt _##name##_re __asm("d0");			\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      __asm volatile ("exg d7,a5\n\tjsr %%a6@(-"#offs":W)\n\texg d7,a5" \
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : "r" (_##name##_bn), "rf"(_n1)				\
      : "fp0", "fp1", "cc", "memory");				\
      _##name##_re;						\
   });								\
})

/* Only graphics.library/LockLayerRom() and graphics.library/UnlockLayerRom() */
#define LP1NRA5(offs, name, t1, v1, r1, bt, bn)			\
({								\
   t1 _##name##_v1 = (v1);					\
   {								\
      register int _d0 __asm("d0");				\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      __asm volatile ("exg d7,a5\n\tjsr %%a6@(-"#offs":W)\n\texg d7,a5" \
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : "r" (_##name##_bn), "rf"(_n1)				\
      : "fp0", "fp1", "cc", "memory");				\
   }								\
})

/* Only exec.library/Supervisor() */
#define LP1A5FP(offs, rt, name, t1, v1, r1, bt, bn, fpt)	\
({								\
   typedef fpt;							\
   t1 _##name##_v1 = (v1);					\
   ({								\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register rt _##name##_re __asm("d0");			\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      __asm volatile ("exg d7,a5\n\tjsr %%a6@(-"#offs":W)\n\texg d7,a5" \
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : "r" (_##name##_bn), "rf"(_n1)				\
      : "fp0", "fp1", "cc", "memory");				\
      _##name##_re;						\
   });								\
})

#define LP2(offs, rt, name, t1, v1, r1, t2, v2, r2, bt, bn)	\
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   ({								\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register rt _##name##_re __asm("d0");			\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2)		\
      : "fp0", "fp1", "cc", "memory");				\
      _##name##_re;						\
   });								\
})

#define LP2NR(offs, name, t1, v1, r1, t2, v2, r2, bt, bn)	\
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   {								\
      register int _d0 __asm("d0");				\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2)		\
      : "fp0", "fp1", "cc", "memory");				\
   }								\
})

/* Only cia.resource/AbleICR() and cia.resource/SetICR() */
#define LP2UB(offs, rt, name, t1, v1, r1, t2, v2, r2)		\
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   ({								\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register rt _##name##_re __asm("d0");			\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : "r"(_n1), "rf"(_n2)					\
      : "fp0", "fp1", "cc", "memory");				\
      _##name##_re;						\
   });								\
})

/* Only dos.library/InternalUnLoadSeg() */
#define LP2FP(offs, rt, name, t1, v1, r1, t2, v2, r2, bt, bn, fpt) \
({								\
   typedef fpt;							\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   ({								\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register rt _##name##_re __asm("d0");			\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2)		\
      : "fp0", "fp1", "cc", "memory");				\
      _##name##_re;						\
   });								\
})

#define LP3(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   ({								\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register rt _##name##_re __asm("d0");			\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3)	\
      : "fp0", "fp1", "cc", "memory");				\
      _##name##_re;						\
   });								\
})

#define LP3NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   {								\
      register int _d0 __asm("d0");				\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3)	\
      : "fp0", "fp1", "cc", "memory");				\
   }								\
})

/* Only cia.resource/AddICRVector() */
#define LP3UB(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   ({								\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register rt _##name##_re __asm("d0");			\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : "r"(_n1), "rf"(_n2), "rf"(_n3)				\
      : "fp0", "fp1", "cc", "memory");				\
      _##name##_re;						\
   });								\
})

/* Only cia.resource/RemICRVector() */
#define LP3NRUB(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3)	\
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   {								\
      register int _d0 __asm("d0");				\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : "r"(_n1), "rf"(_n2), "rf"(_n3)				\
      : "fp0", "fp1", "cc", "memory");				\
   }								\
})

/* Only exec.library/SetFunction() */
#define LP3FP(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, bt, bn, fpt) \
({								\
   typedef fpt;							\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   ({								\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register rt _##name##_re __asm("d0");			\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3)	\
      : "fp0", "fp1", "cc", "memory");				\
      _##name##_re;						\
   });								\
})

/* Only graphics.library/SetCollision() */
#define LP3NRFP(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, bt, bn, fpt) \
({								\
   typedef fpt;							\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   {								\
      register int _d0 __asm("d0");				\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3)	\
      : "fp0", "fp1", "cc", "memory");				\
   }								\
})

#define LP4(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   ({								\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register rt _##name##_re __asm("d0");			\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4) \
      : "fp0", "fp1", "cc", "memory");				\
      _##name##_re;						\
   });								\
})

#define LP4NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   {								\
      register int _d0 __asm("d0");				\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4) \
      : "fp0", "fp1", "cc", "memory");				\
   }								\
})

/* Only exec.library/RawDoFmt() */
#define LP4FP(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, bt, bn, fpt) \
({								\
   typedef fpt;							\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   ({								\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register rt _##name##_re __asm("d0");			\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4) \
      : "fp0", "fp1", "cc", "memory");				\
      _##name##_re;						\
   });								\
})

#define LP5(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   ({								\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register rt _##name##_re __asm("d0");			\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5) \
      : "fp0", "fp1", "cc", "memory");				\
      _##name##_re;						\
   });								\
})

#define LP5NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   {								\
      register int _d0 __asm("d0");				\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5) \
      : "fp0", "fp1", "cc", "memory");				\
   }								\
})

/* Only exec.library/MakeLibrary() */
#define LP5FP(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, bt, bn, fpt) \
({								\
   typedef fpt;							\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   ({								\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register rt _##name##_re __asm("d0");			\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5) \
      : "fp0", "fp1", "cc", "memory");				\
      _##name##_re;						\
   });								\
})

/* Only reqtools.library/XXX() */
#define LP5A4(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   ({								\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register rt _##name##_re __asm("d0");			\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      __asm volatile ("exg d7,a4\n\tjsr %%a6@(-"#offs":W)\n\texg d7,a4" \
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5) \
      : "fp0", "fp1", "cc", "memory");				\
      _##name##_re;						\
   });								\
})

#define LP6(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   t6 _##name##_v6 = (v6);					\
   ({								\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register rt _##name##_re __asm("d0");			\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6) \
      : "fp0", "fp1", "cc", "memory");				\
      _##name##_re;						\
   });								\
})

#define LP6NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   t6 _##name##_v6 = (v6);					\
   {								\
      register int _d0 __asm("d0");				\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6) \
      : "fp0", "fp1", "cc", "memory");				\
   }								\
})

#define LP7(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   t6 _##name##_v6 = (v6);					\
   t7 _##name##_v7 = (v7);					\
   ({								\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register rt _##name##_re __asm("d0");			\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7) \
      : "fp0", "fp1", "cc", "memory");				\
      _##name##_re;						\
   });								\
})

#define LP7NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   t6 _##name##_v6 = (v6);					\
   t7 _##name##_v7 = (v7);					\
   {								\
      register int _d0 __asm("d0");				\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7) \
      : "fp0", "fp1", "cc", "memory");				\
   }								\
})

/* Only workbench.library/AddAppIconA() */
#define LP7A4(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   t6 _##name##_v6 = (v6);					\
   t7 _##name##_v7 = (v7);					\
   ({								\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register rt _##name##_re __asm("d0");			\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      __asm volatile ("exg d7,a4\n\tjsr %%a6@(-"#offs":W)\n\texg d7,a4" \
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7) \
      : "fp0", "fp1", "cc", "memory");				\
      _##name##_re;						\
   });								\
})

/* Would you believe that there really are beasts that need more than 7
   arguments? :-) */

/* For example intuition.library/AutoRequest() */
#define LP8(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   t6 _##name##_v6 = (v6);					\
   t7 _##name##_v7 = (v7);					\
   t8 _##name##_v8 = (v8);					\
   ({								\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register rt _##name##_re __asm("d0");			\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      register t8 _n8 __asm(#r8) = _##name##_v8;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7), "rf"(_n8) \
      : "fp0", "fp1", "cc", "memory");				\
      _##name##_re;						\
   });								\
})

/* For example intuition.library/ModifyProp() */
#define LP8NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   t6 _##name##_v6 = (v6);					\
   t7 _##name##_v7 = (v7);					\
   t8 _##name##_v8 = (v8);					\
   {								\
      register int _d0 __asm("d0");				\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      register t8 _n8 __asm(#r8) = _##name##_v8;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7), "rf"(_n8) \
      : "fp0", "fp1", "cc", "memory");				\
   }								\
})

/* For example layers.library/CreateUpfrontHookLayer() */
#define LP9(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   t6 _##name##_v6 = (v6);					\
   t7 _##name##_v7 = (v7);					\
   t8 _##name##_v8 = (v8);					\
   t9 _##name##_v9 = (v9);					\
   ({								\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register rt _##name##_re __asm("d0");			\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      register t8 _n8 __asm(#r8) = _##name##_v8;		\
      register t9 _n9 __asm(#r9) = _##name##_v9;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7), "rf"(_n8), "rf"(_n9) \
      : "fp0", "fp1", "cc", "memory");				\
      _##name##_re;						\
   });								\
})

/* For example intuition.library/NewModifyProp() */
#define LP9NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   t6 _##name##_v6 = (v6);					\
   t7 _##name##_v7 = (v7);					\
   t8 _##name##_v8 = (v8);					\
   t9 _##name##_v9 = (v9);					\
   {								\
      register int _d0 __asm("d0");				\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      register t8 _n8 __asm(#r8) = _##name##_v8;		\
      register t9 _n9 __asm(#r9) = _##name##_v9;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7), "rf"(_n8), "rf"(_n9) \
      : "fp0", "fp1", "cc", "memory");				\
   }								\
})

/* Kriton Kyrimis <kyrimis@cti.gr> says CyberGraphics needs the following */
#define LP10(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, t10, v10, r10, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   t6 _##name##_v6 = (v6);					\
   t7 _##name##_v7 = (v7);					\
   t8 _##name##_v8 = (v8);					\
   t9 _##name##_v9 = (v9);					\
   t10 _##name##_v10 = (v10);					\
   ({								\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register rt _##name##_re __asm("d0");			\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      register t8 _n8 __asm(#r8) = _##name##_v8;		\
      register t9 _n9 __asm(#r9) = _##name##_v9;		\
      register t10 _n10 __asm(#r10) = _##name##_v10;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7), "rf"(_n8), "rf"(_n9), "rf"(_n10) \
      : "fp0", "fp1", "cc", "memory");				\
      _##name##_re;						\
   });								\
})

/* Only graphics.library/BltMaskBitMapRastPort() */
#define LP10NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, t10, v10, r10, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   t6 _##name##_v6 = (v6);					\
   t7 _##name##_v7 = (v7);					\
   t8 _##name##_v8 = (v8);					\
   t9 _##name##_v9 = (v9);					\
   t10 _##name##_v10 = (v10);					\
   {								\
      register int _d0 __asm("d0");				\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      register t8 _n8 __asm(#r8) = _##name##_v8;		\
      register t9 _n9 __asm(#r9) = _##name##_v9;		\
      register t10 _n10 __asm(#r10) = _##name##_v10;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7), "rf"(_n8), "rf"(_n9), "rf"(_n10) \
      : "fp0", "fp1", "cc", "memory");				\
   }								\
})

/* Only graphics.library/BltBitMap() */
#define LP11(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, t10, v10, r10, t11, v11, r11, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   t6 _##name##_v6 = (v6);					\
   t7 _##name##_v7 = (v7);					\
   t8 _##name##_v8 = (v8);					\
   t9 _##name##_v9 = (v9);					\
   t10 _##name##_v10 = (v10);					\
   t11 _##name##_v11 = (v11);					\
   ({								\
      register int _d1 __asm("d1");				\
      register int _a0 __asm("a0");				\
      register int _a1 __asm("a1");				\
      register rt _##name##_re __asm("d0");			\
      register void *const _##name##_bn __asm("a6") = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      register t8 _n8 __asm(#r8) = _##name##_v8;		\
      register t9 _n9 __asm(#r9) = _##name##_v9;		\
      register t10 _n10 __asm(#r10) = _##name##_v10;		\
      register t11 _n11 __asm(#r11) = _##name##_v11;		\
      __asm volatile ("jsr %%a6@(-"#offs":W)"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7), "rf"(_n8), "rf"(_n9), "rf"(_n10), "rf"(_n11) \
      : "fp0", "fp1", "cc", "memory");				\
      _##name##_re;						\
   });								\
})

typedef void *APTR;

#endif /* __ASSEMBLER__ */

//	#pragma mark -


#ifndef __ASSEMBLER__

// <exec/types.h>


// <exec/nodes.h>


// <exec/lists.h>


// <exec/interrupts.h>


// <exec/library.h>

// cf.
// http://ftp.netbsd.org/pub/NetBSD/NetBSD-release-4-0/src/sys/arch/amiga/stand/bootblock/boot/amigatypes.h

struct Library {
	uint8	dummy1[10];
	uint16	Version, Revision;
	uint8	dummy2[34-24];
};

// <exec/execbase.h>

struct MemHead {
	struct MemHead	*next;
	uint8	dummy1[9-4];
	uint8	Pri;
	uint8	dummy2[14-10];
	uint16	Attribs;
	uint32	First, Lower, Upper, Free;
};

struct ExecBase {
	struct Library	LibNode;
	uint8	dummy1[296-34];
	uint16	AttnFlags;
	uint8	dummy2[300-298];
	void	*ResModules;
	uint8	dummy3[322-304];
	struct MemHead	*MemList;
	uint8	dummy4[568-326];
	uint32	EClockFreq;
	uint8	dummy5[632-334];
} _PACKED;

#endif /* __ASSEMBLER__ */


#define AFF_68010	(0x01)
#define AFF_68020	(0x02)
#define AFF_68030	(0x04)
#define AFF_68040	(0x08)
#define AFF_68881	(0x10)
#define AFF_68882	(0x20)
#define AFF_FPU40	(0x40)


#ifndef __ASSEMBLER__

// <exec/ports.h>



// <exec/io.h>

/*
struct IORequest {
	struct Message	io_Message;
	struct Device	*io_Device;
	struct Unit		*io_Unit;
	uint16			io_Command;
	uint8			io_Flags;
	int8			io_Error;
};

struct IOStdReq {
	struct Message	io_Message;
	struct Device	*io_Device;
	struct Unit		*io_Unit;
	uint16			io_Command;
	uint8			io_Flags;
	int8			io_Error;
	uint32			io_Actual;
	uint32			io_Length;
	void			*io_Data;
	uint32			io_Offset;
};
*/

#endif /* __ASSEMBLER__ */

// io_Flags
#define IOB_QUICK	0
#define IOF_QUICK	0x01


#define CMD_INVALID	0
#define CMD_RESET	1
#define CMD_READ	2
#define CMD_WRITE	3
#define CMD_UPDATE	4
#define CMD_CLEAR	5
#define CMD_STOP	6
#define CMD_START	7
#define CMD_FLUSH	8
#define CMD_NONSTD	9


#ifndef __ASSEMBLER__

// <exec/devices.h>


#endif /* __ASSEMBLER__ */


// <exec/errors.h>

#define IOERR_OPENFAIL		(-1)
#define IOERR_ABORTED		(-2)
#define IOERR_NOCMD			(-3)
#define IOERR_BADLENGTH		(-4)
#define IOERR_BADADDRESS	(-5)
#define IOERR_UNITBUSY		(-6)
#define IOERR_SELFTEST		(-7)


#define EXEC_BASE_NAME SysBase

#define _LVOFindResident	(-0x60)
#define _LVOAllocAbs		(-0xcc)
#define _LVOOldOpenLibrary	(-0x198)
#define _LVOCloseLibrary	(-0x19e)
#define _LVODoIO			(-0x1c8)
#define _LVOOpenLibrary		(-0x228)


#ifndef __ASSEMBLER__

extern ExecBase *EXEC_BASE_NAME;

#define AllocAbs(par1, last) \
	LP2(0xcc, APTR, AllocAbs, unsigned long, par1, d0, APTR, last, a1, \
	, EXEC_BASE_NAME)

#define CloseLibrary(last) \
	LP1NR(0x19e, CloseLibrary, struct Library *, last, a1, \
	, EXEC_BASE_NAME)

#define DoIO(last) \
	LP1(0x1c8, BYTE, DoIO, struct IORequest *, last, a1, \
	, EXEC_BASE_NAME)

#define OpenLibrary(par1, last) \
	LP2(0x228, struct Library *, OpenLibrary, uint8 *, par1, a1, \
	unsigned long, last, d0, \
	, EXEC_BASE_NAME)




extern "C" status_t exec_error(int32 err);

#endif /* __ASSEMBLER__ */


//	#pragma mark -


// <intuition/intuition.h>


#define ALERT_TYPE		0x80000000
#define RECOVERY_ALERT	0x00000000
#define DEADEND_ALERT	0x80000000

#define INTUITION_BASE_NAME IntuitionBase

#define _LVODisplayAlert	(-0x5a)

#ifndef __ASSEMBLER__

extern Library *INTUITION_BASE_NAME;

#define DisplayAlert(par1, par2, last) \
	LP3(0x5a, bool, DisplayAlert, unsigned long, par1, d0, void *, \
	par2, a0, unsigned long, last, d1, \
	, INTUITION_BASE_NAME)

#endif /* __ASSEMBLER__ */


#ifdef __cplusplus
}
#endif

#endif /* _AMICALLS_H */
