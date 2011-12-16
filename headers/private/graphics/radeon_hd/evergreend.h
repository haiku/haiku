/*
 * Copyright 2010 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Alex Deucher
 */
#ifndef R800D_REG_H
#define R800D_REG_H

#define	EVERGREEN_MC_ARB_RAMCFG						0x2760
#define		NOOFBANK_SHIFT							0
#define		NOOFBANK_MASK							0x00000003
#define		NOOFRANK_SHIFT							2
#define		NOOFRANK_MASK							0x00000004
#define		NOOFROWS_SHIFT							3
#define		NOOFROWS_MASK							0x00000038
#define		NOOFCOLS_SHIFT							6
#define		NOOFCOLS_MASK							0x000000C0
#define		CHANSIZE_SHIFT							8
#define		CHANSIZE_MASK							0x00000100
#define		BURSTLENGTH_SHIFT						9
#define		BURSTLENGTH_MASK						0x00000200
#define		CHANSIZE_OVERRIDE						(1 << 11)
#define	EVERGREEN_FUS_MC_ARB_RAMCFG					0x2768
#define	EVERGREEN_MC_VM_AGP_TOP						0x2028
#define	EVERGREEN_MC_VM_AGP_BOT						0x202C
#define	EVERGREEN_MC_VM_AGP_BASE					0x2030
#define	EVERGREEN_MC_VM_FB_LOCATION					0x2024
#define	EVERGREEN_MC_FUS_VM_FB_OFFSET				0x2898
#define	EVERGREEN_MC_VM_MB_L1_TLB0_CNTL				0x2234
#define	EVERGREEN_MC_VM_MB_L1_TLB1_CNTL				0x2238
#define	EVERGREEN_MC_VM_MB_L1_TLB2_CNTL				0x223C
#define	EVERGREEN_MC_VM_MB_L1_TLB3_CNTL				0x2240
#define		ENABLE_L1_TLB							(1 << 0)
#define		ENABLE_L1_FRAGMENT_PROCESSING			(1 << 1)
#define		SYSTEM_ACCESS_MODE_PA_ONLY				(0 << 3)
#define		SYSTEM_ACCESS_MODE_USE_SYS_MAP			(1 << 3)
#define		SYSTEM_ACCESS_MODE_IN_SYS				(2 << 3)
#define		SYSTEM_ACCESS_MODE_NOT_IN_SYS			(3 << 3)
#define		SYSTEM_APERTURE_UNMAPPED_ACCESS_PASS_THRU (0 << 5)
#define		EFFECTIVE_L1_TLB_SIZE(x)				((x)<<15)
#define		EFFECTIVE_L1_QUEUE_SIZE(x)				((x)<<18)
#define	EVERGREEN_MC_VM_MD_L1_TLB0_CNTL				0x2654
#define	EVERGREEN_MC_VM_MD_L1_TLB1_CNTL				0x2658
#define	EVERGREEN_MC_VM_MD_L1_TLB2_CNTL				0x265C

#define	EVERGREEN_FUS_MC_VM_MD_L1_TLB0_CNTL			0x265C
#define	EVERGREEN_FUS_MC_VM_MD_L1_TLB1_CNTL			0x2660
#define	EVERGREEN_FUS_MC_VM_MD_L1_TLB2_CNTL			0x2664

#define	EVERGREEN_MC_VM_SYSTEM_APERTURE_DEFAULT_ADDR 0x203C
#define	EVERGREEN_MC_VM_SYSTEM_APERTURE_HIGH_ADDR	0x2038
#define	EVERGREEN_MC_VM_SYSTEM_APERTURE_LOW_ADDR	0x2034

#define	EVERGREEN_BIOS_0_SCRATCH					0x8500
#define	EVERGREEN_BIOS_1_SCRATCH					0x8504
#define	EVERGREEN_BIOS_2_SCRATCH					0x8508
#define	EVERGREEN_BIOS_3_SCRATCH					0x850C
#define	EVERGREEN_BIOS_4_SCRATCH					0x8510
#define	EVERGREEN_BIOS_5_SCRATCH					0x8514
#define	EVERGREEN_BIOS_6_SCRATCH					0x8518
#define	EVERGREEN_BIOS_7_SCRATCH					0x851C
#define	EVERGREEN_SCRATCH_UMSK						0x8540
#define	EVERGREEN_SCRATCH_ADDR						0x8544

/* evergreen */
#define EVERGREEN_CG_THERMAL_CTRL					0x72c
#define		EVERGREEN_TOFFSET_MASK					0x00003FE0
#define		EVERGREEN_TOFFSET_SHIFT					5
#define EVERGREEN_CG_MULT_THERMAL_STATUS			0x740
#define		EVERGREEN_ASIC_T(x)						((x) << 16)
#define		EVERGREEN_ASIC_T_MASK					0x07FF0000
#define     EVERGREEN_ASIC_T_SHIFT					16
#define EVERGREEN_CG_TS0_STATUS						0x760
#define		EVERGREEN_TS0_ADC_DOUT_MASK				0x000003FF
#define		EVERGREEN_TS0_ADC_DOUT_SHIFT			0
/* APU */
#define EVERGREEN_CG_THERMAL_STATUS					0x678

#endif
