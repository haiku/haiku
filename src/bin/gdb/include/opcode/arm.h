/* ARM assembler/disassembler support.
   Copyright 2004 Free Software Foundation, Inc.

   This file is part of GDB and GAS.

   GDB and GAS are free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 1, or (at
   your option) any later version.

   GDB and GAS are distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GDB or GAS; see the file COPYING.  If not, write to the
   Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

/* The following bitmasks control CPU extensions:  */
#define ARM_EXT_V1	 0x00000001	/* All processors (core set).  */
#define ARM_EXT_V2	 0x00000002	/* Multiply instructions.  */
#define ARM_EXT_V2S	 0x00000004	/* SWP instructions.       */
#define ARM_EXT_V3	 0x00000008	/* MSR MRS.                */
#define ARM_EXT_V3M	 0x00000010	/* Allow long multiplies.  */
#define ARM_EXT_V4	 0x00000020	/* Allow half word loads.  */
#define ARM_EXT_V4T	 0x00000040	/* Thumb v1.               */
#define ARM_EXT_V5	 0x00000080	/* Allow CLZ, etc.         */
#define ARM_EXT_V5T	 0x00000100	/* Thumb v2.               */
#define ARM_EXT_V5ExP	 0x00000200	/* DSP core set.           */
#define ARM_EXT_V5E	 0x00000400	/* DSP Double transfers.   */
#define ARM_EXT_V5J	 0x00000800	/* Jazelle extension.	   */
#define ARM_EXT_V6       0x00001000     /* ARM V6.                 */
#define ARM_EXT_V6K      0x00002000     /* ARM V6K.                */
#define ARM_EXT_V6Z      0x00004000     /* ARM V6Z.                */

/* Co-processor space extensions.  */
#define ARM_CEXT_XSCALE   0x00800000	/* Allow MIA etc.          */
#define ARM_CEXT_MAVERICK 0x00400000	/* Use Cirrus/DSP coprocessor.  */
#define ARM_CEXT_IWMMXT   0x00200000    /* Intel Wireless MMX technology coprocessor.   */

/* Architectures are the sum of the base and extensions.  The ARM ARM (rev E)
   defines the following: ARMv3, ARMv3M, ARMv4xM, ARMv4, ARMv4TxM, ARMv4T,
   ARMv5xM, ARMv5, ARMv5TxM, ARMv5T, ARMv5TExP, ARMv5TE.  To these we add
   three more to cover cores prior to ARM6.  Finally, there are cores which
   implement further extensions in the co-processor space.  */
#define ARM_ARCH_V1			  ARM_EXT_V1
#define ARM_ARCH_V2	(ARM_ARCH_V1	| ARM_EXT_V2)
#define ARM_ARCH_V2S	(ARM_ARCH_V2	| ARM_EXT_V2S)
#define ARM_ARCH_V3	(ARM_ARCH_V2S	| ARM_EXT_V3)
#define ARM_ARCH_V3M	(ARM_ARCH_V3	| ARM_EXT_V3M)
#define ARM_ARCH_V4xM	(ARM_ARCH_V3	| ARM_EXT_V4)
#define ARM_ARCH_V4	(ARM_ARCH_V3M	| ARM_EXT_V4)
#define ARM_ARCH_V4TxM	(ARM_ARCH_V4xM	| ARM_EXT_V4T)
#define ARM_ARCH_V4T	(ARM_ARCH_V4	| ARM_EXT_V4T)
#define ARM_ARCH_V5xM	(ARM_ARCH_V4xM	| ARM_EXT_V5)
#define ARM_ARCH_V5	(ARM_ARCH_V4	| ARM_EXT_V5)
#define ARM_ARCH_V5TxM	(ARM_ARCH_V5xM	| ARM_EXT_V4T | ARM_EXT_V5T)
#define ARM_ARCH_V5T	(ARM_ARCH_V5	| ARM_EXT_V4T | ARM_EXT_V5T)
#define ARM_ARCH_V5TExP	(ARM_ARCH_V5T	| ARM_EXT_V5ExP)
#define ARM_ARCH_V5TE	(ARM_ARCH_V5TExP | ARM_EXT_V5E)
#define ARM_ARCH_V5TEJ	(ARM_ARCH_V5TE	| ARM_EXT_V5J)
#define ARM_ARCH_V6     (ARM_ARCH_V5TEJ | ARM_EXT_V6)
#define ARM_ARCH_V6K    (ARM_ARCH_V6    | ARM_EXT_V6K)
#define ARM_ARCH_V6Z    (ARM_ARCH_V6    | ARM_EXT_V6Z)
#define ARM_ARCH_V6ZK   (ARM_ARCH_V6    | ARM_EXT_V6K | ARM_EXT_V6Z)

/* Processors with specific extensions in the co-processor space.  */
#define ARM_ARCH_XSCALE	(ARM_ARCH_V5TE	| ARM_CEXT_XSCALE)
#define ARM_ARCH_IWMMXT	(ARM_ARCH_XSCALE | ARM_CEXT_IWMMXT)

#define FPU_FPA_EXT_V1	 0x80000000	/* Base FPA instruction set.  */
#define FPU_FPA_EXT_V2	 0x40000000	/* LFM/SFM.		      */
#define FPU_VFP_EXT_NONE 0x20000000	/* Use VFP word-ordering.     */
#define FPU_VFP_EXT_V1xD 0x10000000	/* Base VFP instruction set.  */
#define FPU_VFP_EXT_V1	 0x08000000	/* Double-precision insns.    */
#define FPU_VFP_EXT_V2	 0x04000000	/* ARM10E VFPr1.	      */
#define FPU_MAVERICK	 0x02000000	/* Cirrus Maverick.	      */
#define FPU_NONE	 0

#define FPU_ARCH_FPE	 FPU_FPA_EXT_V1
#define FPU_ARCH_FPA	(FPU_ARCH_FPE | FPU_FPA_EXT_V2)

#define FPU_ARCH_VFP       FPU_VFP_EXT_NONE
#define FPU_ARCH_VFP_V1xD (FPU_VFP_EXT_V1xD | FPU_VFP_EXT_NONE)
#define FPU_ARCH_VFP_V1   (FPU_ARCH_VFP_V1xD | FPU_VFP_EXT_V1)
#define FPU_ARCH_VFP_V2	  (FPU_ARCH_VFP_V1 | FPU_VFP_EXT_V2)

#define FPU_ARCH_MAVERICK  FPU_MAVERICK

/* Some useful combinations:  */
#define ARM_ANY		0x0000ffff	/* Any basic core.  */
#define ARM_ALL		0x00ffffff	/* Any core + co-processor */
#define CPROC_ANY	0x00ff0000	/* Any co-processor */
#define FPU_ANY		0xff000000	/* Note this is ~ARM_ALL.  */
