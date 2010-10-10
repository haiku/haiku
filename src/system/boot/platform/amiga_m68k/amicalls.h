/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		François Revol, revol@free.fr.
 * 
 * Contains prototypes & macros from the Amiga SDK:
**
**	(C) Copyright 1985,1986,1987,1988,1989 Commodore-Amiga, Inc.
**	    All Rights Reserved
 */
#ifndef _AMICALLS_H
#define _AMICALLS_H


#ifdef __cplusplus
extern "C" {
#endif

#ifndef __ASSEMBLER__
#include <OS.h>
#endif /* __ASSEMBLER__ */


// <exec/types.h>

#ifndef __ASSEMBLER__

typedef void	       *APTR;	    /* 32-bit untyped pointer */

typedef long            LONG;       /* signed 32-bit quantity */
typedef unsigned long   ULONG;      /* unsigned 32-bit quantity */
typedef unsigned long   LONGBITS;   /* 32 bits manipulated individually */
typedef short           WORD;       /* signed 16-bit quantity */
typedef unsigned short  UWORD;      /* unsigned 16-bit quantity */
typedef unsigned short  WORDBITS;   /* 16 bits manipulated individually */
#if __STDC__
typedef signed char	BYTE;	    /* signed 8-bit quantity */
#else
typedef char		BYTE;	    /* signed 8-bit quantity */
#endif
typedef unsigned char   UBYTE;      /* unsigned 8-bit quantity */
typedef unsigned char   BYTEBITS;   /* 8 bits manipulated individually */
typedef unsigned short	RPTR;	    /* signed relative pointer */

#ifdef __cplusplus
typedef char           *STRPTR;     /* string pointer (NULL terminated) */
#else
typedef unsigned char  *STRPTR;     /* string pointer (NULL terminated) */
#endif


// <exec/nodes.h>

struct Node {
    struct  Node *ln_Succ;	/* Pointer to next (successor) */
    struct  Node *ln_Pred;	/* Pointer to previous (predecessor) */
    UBYTE   ln_Type;
    BYTE    ln_Pri; 		/* Priority, for sorting */
    char    *ln_Name; 		/* ID string, null terminated */
};	/* Note: word aligned */

// <exec/lists.h>

struct List {
   struct  Node *lh_Head;
   struct  Node *lh_Tail;
   struct  Node *lh_TailPred;
   UBYTE   lh_Type;
   UBYTE   l_pad;
};	/* word aligned */


// <exec/interrupts.h>

struct IntVector {              /* For EXEC use ONLY! */
    APTR    iv_Data;
    VOID    (*iv_Code)();
    struct  Node *iv_Node;
};

// <exec/library.h>

struct Library {
    struct  Node lib_Node;
    UBYTE   lib_Flags;
    UBYTE   lib_pad;
    UWORD   lib_NegSize;            /* number of bytes before library */
    UWORD   lib_PosSize;            /* number of bytes after library */
    UWORD   lib_Version;	    /* major */
    UWORD   lib_Revision;	    /* minor */
    APTR    lib_IdString;	    /* ASCII identification */
    ULONG   lib_Sum;                /* the checksum itself */
    UWORD   lib_OpenCnt;            /* number of current opens */
};	/* Warning: size is not a longword multiple! */

// <exec/>

struct ExecBase {
	struct Library LibNode; /* Standard library node */

/******** Static System Variables ********/

	UWORD	SoftVer;	/* kickstart release number (obs.) */
	WORD	LowMemChkSum;	/* checksum of 68000 trap vectors */
	ULONG	ChkBase;	/* system base pointer complement */
	APTR	ColdCapture;	/* coldstart soft capture vector */
	APTR	CoolCapture;	/* coolstart soft capture vector */
	APTR	WarmCapture;	/* warmstart soft capture vector */
	APTR	SysStkUpper;	/* system stack base   (upper bound) */
	APTR	SysStkLower;	/* top of system stack (lower bound) */
	ULONG	MaxLocMem;	/* top of chip memory */
	APTR	DebugEntry;	/* global debugger entry point */
	APTR	DebugData;	/* global debugger data segment */
	APTR	AlertData;	/* alert data segment */
	APTR	MaxExtMem;	/* top of extended mem, or null if none */

	UWORD	ChkSum; 	/* for all of the above (minus 2) */

/****** Interrupt Related ***************************************/

	struct	IntVector IntVects[16];

/****** Dynamic System Variables *************************************/

	struct	Task *ThisTask; /* pointer to current task (readable) */

	ULONG	IdleCount;	/* idle counter */
	ULONG	DispCount;	/* dispatch counter */
	UWORD	Quantum;	/* time slice quantum */
	UWORD	Elapsed;	/* current quantum ticks */
	UWORD	SysFlags;	/* misc internal system flags */
	BYTE	IDNestCnt;	/* interrupt disable nesting count */
	BYTE	TDNestCnt;	/* task disable nesting count */

	UWORD	AttnFlags;	/* special attention flags (readable) */

	UWORD	AttnResched;	/* rescheduling attention */
	APTR	ResModules;	/* resident module array pointer */
	APTR	TaskTrapCode;
	APTR	TaskExceptCode;
	APTR	TaskExitCode;
	ULONG	TaskSigAlloc;
	UWORD	TaskTrapAlloc;

	// and more...
};

#endif /* __ASSEMBLER__ */

/*  Processors and Co-processors: */
#define AFB_68010	0	/* also set for 68020 */
#define AFB_68020	1	/* also set for 68030 */
#define AFB_68030	2	/* also set for 68040 */
#define AFB_68040	3
#define AFB_68881	4	/* also set for 68882 */
#define AFB_68882	5
#define	AFB_FPU40	6	/* Set if 68040 FPU */

#define AFF_68010	(1L<<0)
#define AFF_68020	(1L<<1)
#define AFF_68030	(1L<<2)
#define AFF_68040	(1L<<3)
#define AFF_68881	(1L<<4)
#define AFF_68882	(1L<<5)
#define	AFF_FPU40	(1L<<6)

#ifndef __ASSEMBLER__

// <exec/ports.h>

struct MsgPort {
    struct  Node mp_Node; 
    UBYTE   mp_Flags; 
    UBYTE   mp_SigBit;		/* signal bit number    */
    void   *mp_SigTask;		/* object to be signalled */
    struct  List mp_MsgList;	/* message linked list  */
};

struct Message {
    struct  Node mn_Node; 
    struct  MsgPort *mn_ReplyPort;  /* message reply port */
    UWORD   mn_Length;              /* total message length, in bytes */
				    /* (include the size of the Message */
				    /* structure in the length) */
};


// <exec/io.h>

struct IORequest {
    struct  Message io_Message;
    struct  Device  *io_Device;     /* device node pointer  */
    struct  Unit    *io_Unit;       /* unit (driver private)*/
    UWORD   io_Command;             /* device command */
    UBYTE   io_Flags;
    BYTE    io_Error;               /* error or warning num */
};

struct IOStdReq {
    struct  Message io_Message;
    struct  Device  *io_Device;     /* device node pointer  */
    struct  Unit    *io_Unit;       /* unit (driver private)*/
    UWORD   io_Command;             /* device command */
    UBYTE   io_Flags;
    BYTE    io_Error;               /* error or warning num */
    ULONG   io_Actual;              /* actual number of bytes transferred */
    ULONG   io_Length;              /* requested number bytes transferred*/
    APTR    io_Data;                /* points to data area */
    ULONG   io_Offset;              /* offset for block structured devices */
};

#endif /* __ASSEMBLER__ */

/* library vector offsets for device reserved vectors */
#define DEV_BEGINIO     (-30)
#define DEV_ABORTIO     (-36)

/* io_Flags defined bits */
#define IOB_QUICK       0
#define IOF_QUICK       (1<<0)


#define CMD_INVALID     0
#define CMD_RESET       1
#define CMD_READ        2
#define CMD_WRITE       3
#define CMD_UPDATE      4
#define CMD_CLEAR       5
#define CMD_STOP        6
#define CMD_START       7
#define CMD_FLUSH       8

#define CMD_NONSTD      9

#ifndef __ASSEMBLER__

// <exec/devices.h>

struct Device { 
    struct  Library dd_Library;
};

struct Unit {
    struct  MsgPort unit_MsgPort;	/* queue for unprocessed messages */
					/* instance of msgport is recommended */
    UBYTE   unit_flags;
    UBYTE   unit_pad;
    UWORD   unit_OpenCnt;		/* number of active opens */
};

#endif /* __ASSEMBLER__ */

#define UNITF_ACTIVE	(1<<0)
#define UNITF_INTASK	(1<<1)

// <exec/errors.h>

#define IOERR_OPENFAIL	 (-1) /* device/unit failed to open */
#define IOERR_ABORTED	 (-2) /* request terminated early [after AbortIO()] */
#define IOERR_NOCMD	 (-3) /* command not supported by device */
#define IOERR_BADLENGTH	 (-4) /* not a valid length (usually IO_LENGTH) */
#define IOERR_BADADDRESS (-5) /* invalid address (misaligned or bad range) */
#define IOERR_UNITBUSY	 (-6) /* device opens ok, but requested unit is busy */
#define IOERR_SELFTEST   (-7) /* hardware failed self-test */

#ifndef __ASSEMBLER__
extern "C" status_t exec_error(int32 err);
#endif /* __ASSEMBLER__ */


// <intuition/intuition.h>

#define ALERT_TYPE	0x80000000
#define RECOVERY_ALERT	0x00000000	/* the system can recover from this */
#define DEADEND_ALERT 	0x80000000	/* no recovery possible, this is it */



#ifdef __cplusplus
}
#endif

#endif /* _TOSCALLS_H */
