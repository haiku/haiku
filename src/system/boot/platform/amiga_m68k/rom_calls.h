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
      void *const _##name##_bn = (bn);	\
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : [libbase] "a" (_##name##_bn)					\
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
      void *const _##name##_bn = (bn);	\
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : [libbase] "a" (_##name##_bn)					\
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : [libbase] "a" (_##name##_bn), "rf"(_n1)				\
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : [libbase] "a" (_##name##_bn), "rf"(_n1)				\
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      __asm volatile ("exg d7,a5\n\tmove.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6\n\texg d7,a5" \
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : [libbase] "a" (_##name##_bn), "rf"(_n1)				\
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      __asm volatile ("exg d7,a5\n\tmove.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6\n\texg d7,a5" \
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : [libbase] "a" (_##name##_bn), "rf"(_n1)				\
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      __asm volatile ("exg d7,a5\n\tmove.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6\n\texg d7,a5" \
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : [libbase] "a" (_##name##_bn), "rf"(_n1)				\
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2)		\
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2)		\
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
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2)		\
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3)	\
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3)	\
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
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
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
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3)	\
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3)	\
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4) \
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4) \
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4) \
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5) \
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5) \
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5) \
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      __asm volatile ("exg d7,a4\n\tmove.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6\n\texg d7,a4" \
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5) \
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6) \
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6) \
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7) \
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7) \
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      __asm volatile ("exg d7,a4\n\tmove.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6\n\texg d7,a4" \
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7) \
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      register t8 _n8 __asm(#r8) = _##name##_v8;		\
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7), "rf"(_n8) \
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      register t8 _n8 __asm(#r8) = _##name##_v8;		\
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7), "rf"(_n8) \
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      register t8 _n8 __asm(#r8) = _##name##_v8;		\
      register t9 _n9 __asm(#r9) = _##name##_v9;		\
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7), "rf"(_n8), "rf"(_n9) \
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
      void *const _##name##_bn = (bn);	\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      register t8 _n8 __asm(#r8) = _##name##_v8;		\
      register t9 _n9 __asm(#r9) = _##name##_v9;		\
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7), "rf"(_n8), "rf"(_n9) \
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
      void *const _##name##_bn = (bn);	\
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
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7), "rf"(_n8), "rf"(_n9), "rf"(_n10) \
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
      void *const _##name##_bn = (bn);	\
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
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_d0), "=r" (_d1), "=r" (_a0), "=r" (_a1)		\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7), "rf"(_n8), "rf"(_n9), "rf"(_n10) \
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
      void *const _##name##_bn = (bn);	\
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
      __asm volatile ("move.l %%a6,%%sp@-\n\tmove.l %[libbase],%%a6\n\tjsr %%a6@(-"#offs":W)\n\tmove.l %%sp@+,%%a6"			\
      : "=r" (_##name##_re), "=r" (_d1), "=r" (_a0), "=r" (_a1)	\
      : [libbase] "a" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7), "rf"(_n8), "rf"(_n9), "rf"(_n10), "rf"(_n11) \
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

// our VFS also has a struct Node...
struct ListNode {
	struct ANode	*ln_Succ;
	struct ANode	*ln_Pred;
	uint8	ln_Type;
	uint8	ln_Pri;
	const char *ln_Name;
};

struct List {
	struct ListNode	*lh_Head;
	struct ListNode	*lh_Tail;
	struct ListNode	*lh_TailPred;
	uint8	lh_Type;
	uint8	lh_pad;
};

// <exec/lists.h>


// <exec/interrupts.h>


// <exec/library.h>

// cf.
// http://ftp.netbsd.org/pub/NetBSD/NetBSD-release-4-0/src/sys/arch/amiga/stand/bootblock/boot/amigatypes.h

struct Library {
	uint8	dummy1[10];
	uint16	Version, Revision;
	uint8	dummy2[34-24];
} _PACKED;

// <exec/execbase.h>

struct MemHead {
	struct MemHead	*next;
	uint8	dummy1[9-4];
	uint8	Pri;
	uint8	dummy2[14-10];
	uint16	Attribs;
	uint32	First, Lower, Upper, Free;
} _PACKED;

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

struct Message {
	struct ListNode	mn_Node;
	struct MsgPort	*mn_ReplyPort;
	uint16	mn_Length;
} _PACKED;

struct MsgPort {
	struct ListNode	mp_Node;
	uint8	mp_Flags;
	uint8	mp_SigBits;
	void	*mp_SigTask;
	struct List	mp_MsgList;
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


struct IORequest {
	struct Message	io_Message;
	struct Device	*io_Device;
	struct Unit		*io_Unit;
	uint16			io_Command;
	uint8			io_Flags;
	int8			io_Error;
} _PACKED;

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
} _PACKED;


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

#define OldOpenLibrary(last) \
	LP1(0x198, struct Library *, OldOpenLibrary, /*UBYTE*/const char *, last, a1, \
	, EXEC_BASE_NAME)

#define CloseLibrary(last) \
	LP1NR(0x19e, CloseLibrary, struct Library *, last, a1, \
	, EXEC_BASE_NAME)

#define OpenDevice(par1, par2, par3, last) \
	LP4(0x1bc, int8, OpenDevice, /*UBYTE*/uint8 *, par1, a0, unsigned long, par2, d0, struct IORequest *, par3, a1, unsigned long, last, d1, \
	, EXEC_BASE_NAME)

#define CloseDevice(last) \
	LP1NR(0x1c2, CloseDevice, struct IORequest *, last, a1, \
	, EXEC_BASE_NAME)

#define DoIO(last) \
	LP1(0x1c8, int8, DoIO, struct IORequest *, last, a1, \
	, EXEC_BASE_NAME)

#define OpenLibrary(par1, last) \
	LP2(0x228, struct Library *, OpenLibrary, uint8 *, par1, a1, \
	unsigned long, last, d0, \
	, EXEC_BASE_NAME)

#define CreateIORequest(par1, last) \
	LP2(0x28e, APTR, CreateIORequest, struct MsgPort *, par1, a0, unsigned long, last, d0, \
	, EXEC_BASE_NAME)

#define DeleteIORequest(last) \
	LP1NR(0x294, DeleteIORequest, APTR, last, a0, \
	, EXEC_BASE_NAME)

#define CreateMsgPort() \
	LP0(0x29a, struct MsgPort *, CreateMsgPort, \
	, EXEC_BASE_NAME)

#define ColdReboot() \
	LP0NR(0x2d6, ColdReboot, \
	, EXEC_BASE_NAME)



extern "C" status_t exec_error(int32 err);

#endif /* __ASSEMBLER__ */

//	#pragma mark -

// <graphics/gfx.h>

#ifndef __ASSEMBLER__

struct BitMap {
	uint16	BytesPerRow;
	uint16	Rows;
	uint8	Flags;
	uint8	Depth;
	uint16	pad;
	void	*Planes[8];
};

struct Rectangle {
	int16	MinX, MinY, MaxX, MaxY;
};

struct Point {
	int16	x, y;
};

#endif /* __ASSEMBLER__ */

// <graphics/graphics.h>

#define GRAPHICSNAME	"graphics.library"
#define GRAPHICS_BASE_NAME	GraphicsBase

#ifndef __ASSEMBLER__

struct GfxBase {
	struct Library	LibNode;
	struct View		*ActiView;
	struct copinit	*copinit;
	int32	*cia;
	int32	*blitter;
	uint16	*LOFlist;
	uint16	*SHFlist;
	struct bltnode	*blthd;
	struct bltnode	*blttl;
	struct bltnode	*bsblthd;
	struct bltnode	*bsblttl;
	
	//...
} _PACKED;

struct ViewPort {
	struct ViewPort *Next;
	struct ColorMap	*ColorMap;
	struct CopList	*DspIns;
	struct CopList	*SprIns;
	struct CopList	*ClrIns;
	struct UCopList	*UCopIns;
	int16	DWidth, DHeight;
	int16	DxOffset, DyOffset;
	uint16	Modes;
	uint8	SpritePriorities;
	uint8	ExtendedModes;
	struct RastInfo	*RasInfo;
} _PACKED;

struct RastPort {
	struct Layer	*Layer;
	struct BitMap	*BitMap;
	//...
} _PACKED;

// <graphics/text.h>

struct TextAttr {
	const char *taName;
	uint16	ta_YSize;
	uint8	ta_Style, ta_Flags;
} _PACKED;

struct TextFont {
	struct Message	tf_Message;
	uint16	tf_YSize;
	uint8	tf_Style;
	uint8	tf_Flags;
	uint16	tf_XSize;
	//...
} _PACKED;

extern struct GfxBase *GRAPHICS_BASE_NAME;

#define ClearScreen(last) \
	LP1NR(0x30, ClearScreen, struct RastPort *, last, a1, \
	, GRAPHICS_BASE_NAME)

#define Text(par1, par2, last) \
	LP3(0x3c, int32, Text, struct RastPort *, par1, a1, const char *, par2, a0, unsigned long, last, d0, \
	, GRAPHICS_BASE_NAME)

#define SetFont(par1, last) \
	LP2(0x42, int32, SetFont, struct RastPort *, par1, a1, struct TextFont *, last, a0, \
	, GRAPHICS_BASE_NAME)

#define OpenFont(last) \
	LP1(0x48, struct TextFont *, OpenFont, struct TextAttr *, last, a0, \
	, GRAPHICS_BASE_NAME)

#define LoadRGB4(par1, par2, last) \
	LP3NR(0xc0, LoadRGB4, struct ViewPort *, par1, a0, const uint16 *, par2, a1, long, last, d0, \
	, GRAPHICS_BASE_NAME)

#define Move(par1, par2, last) \
	LP3NR(0xf0, Move, struct RastPort *, par1, a1, long, par2, d0, long, last, d1, \
	, GRAPHICS_BASE_NAME)

#define SetAPen(par1, last) \
	LP2NR(0x156, SetAPen, struct RastPort *, par1, a1, unsigned long, last, d0, \
	, GRAPHICS_BASE_NAME)

#define SetBPen(par1, last) \
	LP2NR(0x15c, SetBPen, struct RastPort *, par1, a1, unsigned long, last, d0, \
	, GRAPHICS_BASE_NAME)

#define SetDrMd(par1, last) \
	LP2NR(0x162, SetDrMd, struct RastPort *, par1, a1, unsigned long, last, d0, \
	, GRAPHICS_BASE_NAME)

#define FindDisplayInfo(last) \
	LP1(0x2d6, DisplayInfoHandle, FindDisplayInfo, unsigned long, last, d0, \
	, GRAPHICS_BASE_NAME)

#define NextDisplayInfo(last) \
	LP1(0x2dc, uint32, NextDisplayInfo, unsigned long, last, d0, \
	, GRAPHICS_BASE_NAME)

#define GetDisplayInfoData(par1, par2, par3, par4, last) \
	LP5(0x2f4, uint32, GetDisplayInfoData, DisplayInfoHandle, par1, a0, uint8 *, par2, a1, unsigned long, par3, d0, unsigned long, par4, d1, unsigned long, last, d2, \
	, GRAPHICS_BASE_NAME)


#endif /* __ASSEMBLER__ */

/* drawing modes */
#define JAM1	0	// only draw foreground
#define JAM2	1	// draw both fg & bg

// <graphics/modeid.h>

#define INVALID_ID	(~0)

// <graphics/displayinfo.h>

#ifndef __ASSEMBLER__

typedef void *DisplayInfoHandle;

struct QueryHeader {
	uint32	StructID;
	uint32	DisplayID;
	uint32	SkipID;
	uint32	Length;
};

struct DisplayInfo {
	struct QueryHeader	Header;
	uint16	NotAvailable;
	uint32	PropertyFlags;
	struct Point	Resolution;
	uint16	PixelSpeed;
	uint16	NumStdSprites;
	uint16	PaletteRange;
	struct Point	SpriteResolution;
	uint8	pad[4];
	uint8	RedBits;
	uint8	GreenBits;
	uint8	BlueBits;
	uint8	pad2[5];
	uint32	reserved[2];
};

struct DimensionInfo {
	struct QueryHeader	Header;
	uint16	MaxDepth;
	uint16	MinRasterWidth;
	uint16	MinRasterHeight;
	uint16	MaxRasterWidth;
	uint16	MaxRasterHeight;
	struct Rectangle	Nominal;
	//... overscan stuff
	struct Rectangle	overscanStuff[4];
	uint8	pad[14];
	uint32	reserved[2];
};

#define DISPLAYNAMELEN 32

struct NameInfo {
	struct QueryHeader	Header;
	uchar	Name[DISPLAYNAMELEN];
	uint32	reserved[2];
};

#endif /* __ASSEMBLER__ */

#define DTAG_DISP	0x80000000
#define DTAG_DIMS	0x80001000
#define DTAG_MNTR	0x80002000
#define DTAG_NAME	0x80003000

#define DIPF_IS_LACE		0x00000001
#define DIPF_IS_DUALPF		0x00000002
#define DIPF_IS_PF2PRI		0x00000004
#define DIPF_IS_HAM			0x00000008
#define DIPF_IS_ECS			0x00000010
#define DIPF_IS_AA			0x00010000
#define DIPF_IS_PAL			0x00000020
#define DIPF_IS_SPRITES		0x00000040
#define DIPF_IS_GENLOCK		0x00000080
#define DIPF_IS_WB			0x00000100
#define DIPF_IS_DRAGGABLE	0x00000200
#define DIPF_IS_PANELLED	0x00000400
#define DIPF_IS_BEAMSYNC	0x00000800
#define DIPF_IS_EXTRAHALDBRITE	0x00001000
//
#define DIPF_IS_FOREIGN		0x80000000

//	#pragma mark -


// <intuition/intuition.h>


#define ALERT_TYPE		0x80000000
#define RECOVERY_ALERT	0x00000000
#define DEADEND_ALERT	0x80000000

#define INTUITION_BASE_NAME IntuitionBase

#define _LVODisplayAlert	(-0x5a)

#ifndef __ASSEMBLER__

struct Window {
	uint8 dummy1[136];
};

struct NewWindow {
	int16	LeftEdge, TopEdge;
	int16	Width, Height;
	uint8	DetailPen, BlockPen;
	uint32	IDCMPFlags;
	uint32	Flags;
	struct Gadget	*FirstGadget;
	struct Image	*CheckMark;
	const char	*Title;
	struct Screen	*Screen;
	struct BitMap	*BitMap;
	int16	MinWidth, MinHeight;
	uint16	MaxWidth, MaxHeight;
	uint16	Type;
};

#endif /* __ASSEMBLER__ */

#define CUSTOMSCREEN 0x000f

#define IDCMP_CLOSEWINDOW	0x00000200

#define WFLG_SIZEGADGET		0x00000001
#define WFLG_DRAGBAR		0x00000002
#define WFLG_DEPTHGADGET	0x00000004
#define WFLG_CLOSEGADGET	0x00000008

#define WFLG_SMART_REFRESH	0x00000000
#define WFLG_SIMPLE_REFRESH	0x00000040

#define WFLG_ACTIVATE		0x00001000

#ifndef __ASSEMBLER__

// <intuition/screen.h>

struct NewScreen {
	int16	LeftEdge, TopEdge, Width, Height, Depth;
	uint8	DetailPen, BlockPen;
	uint16	ViewModes;
	uint16	Type;
	struct TextAttr	*Font;
	/*UBYTE*/const char	*DefaultTitle;
	struct Gadget	*Gadgets;
	struct BitMap	*CustomBitMap;
};


struct Screen {
	struct Screen	*NextScreen;
	struct Window	*FirstWindow;
	int16	LeftEdge, TopEdge;
	int16	Width, Height;
	int16	MouseX, MouseY;
	uint16	Flags;
	const char	*Title;
	const char	*DefaultTitle;
	int8	BarHeight, BarVBorder, BarHBorder, MenuVBorder, MenuHBorder;
	int8	WBorTop, WBorLeft, WBorRight, WBorBottom;
	struct TextAttr	*Font;
	struct ViewPort	ViewPort;
	struct RastPort	RastPort;
	//...
};

extern struct Library *INTUITION_BASE_NAME;

#define CloseScreen(last) \
	LP1(0x42, bool, CloseScreen, struct Screen *, last, a0, \
	, INTUITION_BASE_NAME)

#define DisplayAlert(par1, par2, last) \
	LP3(0x5a, bool, DisplayAlert, unsigned long, par1, d0, void *, \
	par2, a0, unsigned long, last, d1, \
	, INTUITION_BASE_NAME)

#define OpenScreen(last) \
	LP1(0xc6, struct Screen *, OpenScreen, struct NewScreen *, last, a0, \
	, INTUITION_BASE_NAME)

#define OpenWindow(last) \
	LP1(0xcc, struct Window *, OpenWindow, struct NewWindow *, last, a0, \
	, INTUITION_BASE_NAME)

#define RemakeDisplay() \
	LP0(0x180, int32, RemakeDisplay, \
	, INTUITION_BASE_NAME)


#endif /* __ASSEMBLER__ */


// <devices/conunit.h>

#define CONU_LIBRARY -1
#define CONU_STANDARD 0
#define CONU_CHARMAP 1

#ifndef __ASSEMBLER__

struct ConUnit {
	struct MsgPort	cu_MP;
	struct Window	*cu_Window;
	int16	cu_XCP, cu_YCP;
	int16	cu_XMax, cu_YMax;
};

#endif /* __ASSEMBLER__ */

// <devices/console.h>

#define CONSOLENAME "console.device"

// <devices/keymap.h>

#ifndef KEYMAP_BASE_NAME
#define KEYMAP_BASE_NAME KeymapBase
#endif

#define KEYMAPNAME "keymap.library"

#ifndef __ASSEMBLER__
#define MapRawKey(par1, par2, par3, last) \
	LP4(0x2a, int16, MapRawKey, struct InputEvent *, par1, a0, char *, par2, a1, long, par3, d1, struct KeyMap *, last, a2, \
	, KEYMAP_BASE_NAME)

extern struct Library *KEYMAP_BASE_NAME;

#endif /* __ASSEMBLER__ */

// <libraries/lowlevel.h>

#ifndef LOWLEVEL_BASE_NAME
#define LOWLEVEL_BASE_NAME LowLevelBase
#endif

#define LOWLEVELNAME "lowlevel.library"

#ifndef __ASSEMBLER__

#define GetKey() \
	LP0(0x30, uint32, GetKey, \
	, LOWLEVEL_BASE_NAME)

#define QueryKeys(par1, last) \
	LP2NR(0x36, QueryKeys, struct KeyQuery *, par1, a0, unsigned long, last, d1, \
	, LOWLEVEL_BASE_NAME)

extern struct Library *LOWLEVEL_BASE_NAME;

#endif /* __ASSEMBLER__ */


// <devices/keyboard.h>

#define KBD_READEVENT (CMD_NONSTD+0)

// <devices/inputevent.h>

#ifndef __ASSEMBLER__

struct InputEvent {
	struct InputEvent	*ie_NextEvent;
	uint8	ie_Class;
	uint8	ie_SubClass;
	uint16	ie_Code;
	uint16	ie_Qualifier;
	union {
	struct {
		int16	ie_x;
		int16	ie_y;
	}	ie_xy;
	APTR	ie_addr;
	struct {
		uint8	ie_prev1DownCode;
		uint8	ie_prev1DownQual;
		uint8	ie_prev2DownCode;
		uint8	ie_prev2DownQual;
	}	ie_dead;
	} ie_position;
	/*struct timeval*/
	struct { uint32 tv_secs, tv_micro; } ie_TimeStamp;
};

#endif /* __ASSEMBLER__ */

#define IECLASS_RAWKEY 0x01
#define IESUBCLASS_RAWKEY 0x01

#define IECODE_UP_PREFIX 0x80

#define IECODE_KEY_UP		0x4c
#define IECODE_KEY_DOWN		0x4d
#define IECODE_KEY_LEFT		0x4f
#define IECODE_KEY_RIGHT	0x4e
#define IECODE_KEY_PAGE_UP			0x67
#define IECODE_KEY_PAGE_DOWN		0x66

#ifdef __cplusplus
}
#endif

#endif /* _AMICALLS_H */
