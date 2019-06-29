/*
 * Copyright (c) 2002, Thomas Kurschel
 * Distributed under the terms of the MIT license.
 */

/**	Graphics card detection */


#include "radeon_driver.h"

#include <stdio.h>
#include <string.h>


// this table is gathered from different sources
// and may even contain some wrong entries
#define VENDOR_ID_ATI		0x1002

// R100
#define DEVICE_ID_RADEON_QD		0x5144
#define DEVICE_ID_RADEON_QE		0x5145
#define DEVICE_ID_RADEON_QF		0x5146
#define DEVICE_ID_RADEON_QG		0x5147

// RV100
#define DEVICE_ID_RADEON_QY		0x5159
#define DEVICE_ID_RADEON_QZ		0x515a

#define DEVICE_ID_RN50_515E		0x515E
#define DEVICE_ID_RN50_5969		0x5969

// M6
#define DEVICE_ID_RADEON_LY		0x4c59
#define DEVICE_ID_RADEON_LZ		0x4c5a

// RV200
#define DEVICE_ID_RADEON_QW		0x5157
#define DEVICE_ID_RADEON_QX		0x5158

// R200 mobility
#define DEVICE_ID_RADEON_LW		0x4c57
#define DEVICE_ID_RADEON_LX		0x4c58

// R200
#define DEVICE_ID_RADEON_QH		0x5148
#define DEVICE_ID_RADEON_QI		0x5149
#define DEVICE_ID_RADEON_QJ		0x514a
#define DEVICE_ID_RADEON_QK		0x514b
#define DEVICE_ID_RADEON_QL		0x514c
#define DEVICE_ID_RADEON_QM		0x514d

#define DEVICE_ID_RADEON_Qh		0x5168
#define DEVICE_ID_RADEON_Qi		0x5169
#define DEVICE_ID_RADEON_Qj		0x516a
#define DEVICE_ID_RADEON_Qk		0x516b

#define DEVICE_ID_RADEON_BB		0x4242
#define DEVICE_ID_RADEON_BC		0x4243

// RV250
#define DEVICE_ID_RADEON_If     0x4966
#define DEVICE_ID_RADEON_Ig     0x4967

// M9 (RV250)
#define DEVICE_ID_RADEON_Ld     0x4c64
#define DEVICE_ID_RADEON_Le     0x4c65
#define DEVICE_ID_RADEON_Lf     0x4c66
#define DEVICE_ID_RADEON_Lg     0x4c67

// RV280
#define DEVICE_ID_RADEON_5960	0x5960
#define DEVICE_ID_RADEON_Za		0x5961
#define DEVICE_ID_RADEON_Zb		0x5962 // new
#define DEVICE_ID_RADEON_Zd		0x5964
#define DEVICE_ID_RADEON_Ze		0x5965 // new

// M9+ (RV280)
#define DEVICE_ID_RADEON_5c61   0x5c61
#define DEVICE_ID_RADEON_5c63   0x5c63 // new

// r300
#define DEVICE_ID_RADEON_ND     0x4e44
#define DEVICE_ID_RADEON_NE     0x4e45
#define DEVICE_ID_RADEON_NF     0x4e46
#define DEVICE_ID_RADEON_NG     0x4e47

// r300-4P
#define DEVICE_ID_RADEON_AD     0x4144
#define DEVICE_ID_RADEON_AE     0x4145
#define DEVICE_ID_RADEON_AF     0x4146
#define DEVICE_ID_RADEON_AG     0x4147

// rv350
#define DEVICE_ID_RADEON_AP		0x4150
#define DEVICE_ID_RADEON_AQ		0x4151
#define DEVICE_ID_RADEON_AR		0x4152 // RS360
#define DEVICE_ID_RADEON_AS		0x4153 // RV350 ?? on X.org
#define DEVICE_ID_RADEON_AT		0x4154 // new
#define DEVICE_ID_RADEON_4155	0x4155 // new
#define DEVICE_ID_RADEON_AV		0x4156 // new

// m10 (rv350)
#define DEVICE_ID_RADEON_NP		0x4e50
#define DEVICE_ID_RADEON_NQ		0x4e51 // new
#define DEVICE_ID_RADEON_NR		0x4e52 // new
#define DEVICE_ID_RADEON_NS		0x4e53 // new
#define DEVICE_ID_RADEON_NT		0x4e54
#define DEVICE_ID_RADEON_NV		0x4e56 // new

// r350
#define DEVICE_ID_RADEON_AH		0x4148
#define DEVICE_ID_RADEON_AI		0x4149 // new
#define DEVICE_ID_RADEON_AJ		0x414a // new
#define DEVICE_ID_RADEON_AK		0x414b // new
#define DEVICE_ID_RADEON_NH		0x4e48
#define DEVICE_ID_RADEON_NI		0x4e49
#define DEVICE_ID_RADEON_NK		0x4e4b // new

// r360
#define DEVICE_ID_RADEON_NJ		0x4e4a

// rv370 X300
//#define DEVICE_ID_RADEON_5b50	0x5b50
#define DEVICE_ID_RADEON_5b60	0x5b60
#define DEVICE_ID_RADEON_5b62	0x5b62
#define DEVICE_ID_RADEON_5b63   0x5b63 // new
#define DEVICE_ID_RADEON_5b64	0x5b64 // new
#define DEVICE_ID_RADEON_5b65	0x5b65 // new
#define DEVICE_ID_RADEON_5460	0x5460
#define DEVICE_ID_RADEON_5464	0x5464 // new

// rv380 X600
#define DEVICE_ID_RADEON_3e50	0x3e50
#define DEVICE_ID_RADEON_3e54	0x3e54 // new
#define DEVICE_ID_RADEON_3150	0x3150 // new
#define DEVICE_ID_RADEON_3154	0x3154 // new
#define DEVICE_ID_RADEON_5462	0x5462 // new X600SE on Toshiba M50 an X300???

// rv380 X600AIW
#define DEVICE_ID_RADEON_5b62	0x5b62

// rv410 X700 pro
#define DEVICE_ID_RADEON_5e48	0x5e48 // new
#define DEVICE_ID_RADEON_564a	0x564a // new
#define DEVICE_ID_RADEON_564b	0x564b // new
#define DEVICE_ID_RADEON_564f	0x564f // new
#define DEVICE_ID_RADEON_5652	0x5652 // new
#define DEVICE_ID_RADEON_5653	0x5653 // new
#define DEVICE_ID_RADEON_5e4b	0x5e4b
#define DEVICE_ID_RADEON_5e4a	0x5e4a // new
#define DEVICE_ID_RADEON_5e4d	0x5e4d // new
#define DEVICE_ID_RADEON_5e4c	0x5e4c // new
#define DEVICE_ID_RADEON_5e4f	0x5e4f // new


// r420 X800
#define DEVICE_ID_RADEON_JH 	0x4a48 // new
#define DEVICE_ID_RADEON_JI		0x4a49
#define DEVICE_ID_RADEON_JJ 	0x4a4a
#define DEVICE_ID_RADEON_JK		0x4a4b
#define DEVICE_ID_RADEON_JL		0x4a4c // new mobility
#define DEVICE_ID_RADEON_JM 	0x4a4d // new
#define DEVICE_ID_RADEON_JN		0x4a4e // new
#define DEVICE_ID_RADEON_JP		0x4a50
#define DEVICE_ID_RADEON_4a4f	0x4a4f // new

// r423 X800
#define DEVICE_ID_RADEON_UH		0x5548 // new
#define DEVICE_ID_RADEON_UI		0x5549
#define DEVICE_ID_RADEON_UJ		0x554a
#define DEVICE_ID_RADEON_UK		0x554b
#define DEVICE_ID_RADEON_UQ		0x5551 // new
#define DEVICE_ID_RADEON_UR		0x5552 // new
#define DEVICE_ID_RADEON_UT		0x5554 // new

#define DEVICE_ID_RADEON_UM		0x554d // ?
#define DEVICE_ID_RADEON_UO		0x554f // ?

#define DEVICE_ID_RADEON_5d57	0x5d57
#define DEVICE_ID_RADEON_5550	0x5550 // new

// r430 X850
#define DEVICE_ID_RADEON_5d49	0x5d49 // new mob
#define DEVICE_ID_RADEON_5d4a	0x5d4a // new mob
#define DEVICE_ID_RADEON_5d48	0x5d48 // new mob
#define DEVICE_ID_RADEON_554f	0x554f // new
#define DEVICE_ID_RADEON_554d	0x554d // new
#define DEVICE_ID_RADEON_554e	0x554e // new
#define DEVICE_ID_RADEON_554c	0x554c // new

// r480
#define DEVICE_ID_RADEON_5d4c	0x5d4c // new
#define DEVICE_ID_RADEON_5d50	0x5d50 // new
#define DEVICE_ID_RADEON_5d4e	0x5d4e // new
#define DEVICE_ID_RADEON_5d4f	0x5d4f // new
#define DEVICE_ID_RADEON_5d52	0x5d52 // new
#define DEVICE_ID_RADEON_5d4d	0x5d4d // new

// r481
#define DEVICE_ID_RADEON_KJ		0x4b4a
#define DEVICE_ID_RADEON_KK		0x4b4b
#define DEVICE_ID_RADEON_KL		0x4b4c
#define DEVICE_ID_RADEON_KI		0x4b49

// rs100
#define DEVICE_ID_RS100_4136	0x4136
#define DEVICE_ID_RS100_4336	0x4336

// rs200
#define DEVICE_ID_RS200_4337	0x4337
#define DEVICE_ID_RS200_4137	0x4137

// rs250
#define DEVICE_ID_RS250_4237	0x4237
#define DEVICE_ID_RS250_4437	0x4437

// rs300
#define DEVICE_ID_RS300_5834	0x5834
#define DEVICE_ID_RS300_5835	0x5835

// rs350
#define DEVICE_ID_RS350_7834	0x7834
#define DEVICE_ID_RS350_7835	0x7835

// rs400
#define DEVICE_ID_RS400_5a41	0x5a41
#define DEVICE_ID_RS400_5a42	0x5a42

// rs410
#define DEVICE_ID_RS410_5a61	0x5a61
#define DEVICE_ID_RS410_5a62	0x5a62

// rs480/82
#define DEVICE_ID_RS480_5954	0x5954
#define DEVICE_ID_RS480_5955	0x5955
#define DEVICE_ID_RS482_5974	0x5974
#define DEVICE_ID_RS482_5975	0x5975

typedef struct {
	uint16 device_id;
	radeon_type asic;
	uint32 features;
	char *name;
} RadeonDevice;

#define STD_RADEON 0 // common as muck PC graphics card (if there is such a thing)
#define ISMOBILITY 1 // is mobility
#define INTEGRATED 2 // is IGP (Integrated Graphics Processor) onboard video
#define MOBILE_IGP ISMOBILITY | INTEGRATED // 2 disabilites for the price of 1

// list of supported devices
RadeonDevice radeon_device_list[] = {
	// original Radeons, now called r100
	{ DEVICE_ID_RADEON_QD, rt_r100, STD_RADEON, "Radeon 7200 / Radeon / ALL-IN-WONDER Radeon" },
	{ DEVICE_ID_RADEON_QE, rt_r100, STD_RADEON, "Radeon QE" },
	{ DEVICE_ID_RADEON_QF, rt_r100, STD_RADEON, "Radeon QF" },
	{ DEVICE_ID_RADEON_QG, rt_r100, STD_RADEON, "Radeon QG" },

	// Radeon VE (low-cost, dual CRT, no TCL), was rt_ve now refered to as rv100
	{ DEVICE_ID_RADEON_QY, rt_rv100, STD_RADEON, "Radeon 7000 / Radeon VE" },
	{ DEVICE_ID_RADEON_QZ, rt_rv100, STD_RADEON, "Radeon QZ VE" },

	{ DEVICE_ID_RN50_515E, rt_rv100, STD_RADEON, "ES1000 515E (PCI)" }, // Evans and Sutherland something or other?
	{ DEVICE_ID_RN50_5969, rt_rv100, STD_RADEON, "ES1000 5969 (PCI)" },

	// mobility version of original Radeon (based on VE), now called M6
	{ DEVICE_ID_RADEON_LY, rt_rv100, ISMOBILITY, "Radeon Mobility" },
	{ DEVICE_ID_RADEON_LZ, rt_rv100, ISMOBILITY, "Radeon Mobility M6 LZ" },

	// RV200 (dual CRT)
	{ DEVICE_ID_RADEON_QW, rt_rv200, STD_RADEON, "Radeon 7500 / ALL-IN-WONDER Radeon 7500" },
	{ DEVICE_ID_RADEON_QX, rt_rv200, STD_RADEON, "Radeon 7500 QX" },

	// M7 (based on RV200) was rt_m 7
	{ DEVICE_ID_RADEON_LW, rt_rv200, ISMOBILITY, "Radeon Mobility 7500" },
	{ DEVICE_ID_RADEON_LX, rt_rv200, ISMOBILITY, "Radeon Mobility 7500 GL" },

	// R200
	{ DEVICE_ID_RADEON_QH, rt_r200, STD_RADEON, "Fire GL E1" },	// chip fgl8800
	{ DEVICE_ID_RADEON_QI, rt_r200, STD_RADEON, "Radeon 8500 QI" },
	{ DEVICE_ID_RADEON_QJ, rt_r200, STD_RADEON, "Radeon 8500 QJ" },
	{ DEVICE_ID_RADEON_QK, rt_r200, STD_RADEON, "Radeon 8500 QK" },
	{ DEVICE_ID_RADEON_QL, rt_r200, STD_RADEON, "Radeon 8500 / 8500LE / ALL-IN-WONDER Radeon 8500" },
	{ DEVICE_ID_RADEON_QM, rt_r200, STD_RADEON, "Radeon 9100" },

	{ DEVICE_ID_RADEON_Qh, rt_r200, STD_RADEON, "Radeon 8500 Qh" },
	{ DEVICE_ID_RADEON_Qi, rt_r200, STD_RADEON, "Radeon 8500 Qi" },
	{ DEVICE_ID_RADEON_Qj, rt_r200, STD_RADEON, "Radeon 8500 Qj" },
	{ DEVICE_ID_RADEON_Qk, rt_r200, STD_RADEON, "Radeon 8500 Qk" },

	{ DEVICE_ID_RADEON_BB, rt_r200, STD_RADEON, "ALL-IN-Wonder Radeon 8500 DV (BB)" },
	{ DEVICE_ID_RADEON_BC, rt_r200, STD_RADEON, "ALL-IN-Wonder Radeon 8500 DV (BC)" },

	// RV250 (cut-down R200 with integrated TV-Out)
	{ DEVICE_ID_RADEON_If, rt_rv250, STD_RADEON, "Radeon 9000" },
	{ DEVICE_ID_RADEON_Ig, rt_rv250, STD_RADEON, "Radeon 9000 Ig" },

	// M9 (based on rv250) was rt_m9
	{ DEVICE_ID_RADEON_Ld, rt_rv250, ISMOBILITY, "Radeon Mobility 9000 Ld" },
	{ DEVICE_ID_RADEON_Le, rt_rv250, ISMOBILITY, "Radeon Mobility 9000 Le" },
	{ DEVICE_ID_RADEON_Lf, rt_rv250, ISMOBILITY, "Radeon Mobility 9000 Lf" },
	{ DEVICE_ID_RADEON_Lg, rt_rv250, ISMOBILITY, "Radeon Mobility 9000 Lg" },

	// RV280 (rv250 but faster)
	{ DEVICE_ID_RADEON_5960, rt_rv280, STD_RADEON, "Radeon 9200 Pro" },
	{ DEVICE_ID_RADEON_Za, rt_rv280, STD_RADEON, "Radeon 9200" },
	{ DEVICE_ID_RADEON_Zb, rt_rv280, STD_RADEON, "Radeon 9200" },
	{ DEVICE_ID_RADEON_Zd, rt_rv280, STD_RADEON, "Radeon 9200 SE" },
	{ DEVICE_ID_RADEON_Ze, rt_rv280, STD_RADEON, "Ati FireMV 2200" },

	// M9+ (based on rv280) was rt_m9plus
	{ DEVICE_ID_RADEON_5c61, rt_rv280, ISMOBILITY, "Radeon Mobility 9200" },
	{ DEVICE_ID_RADEON_5c63, rt_rv280, ISMOBILITY, "Radeon Mobility 9200" },

	// R300
	{ DEVICE_ID_RADEON_ND, rt_r300, STD_RADEON, "Radeon 9700 ND" },
	{ DEVICE_ID_RADEON_NE, rt_r300, STD_RADEON, "Radeon 9700 NE" },
	{ DEVICE_ID_RADEON_NF, rt_r300, STD_RADEON, "Radeon 9600 XT" },
	{ DEVICE_ID_RADEON_NG, rt_r300, STD_RADEON, "Radeon 9700 NG" },

	// r300-4P
	{ DEVICE_ID_RADEON_AD, rt_r300, STD_RADEON, "Radeon 9700 AD" },
	{ DEVICE_ID_RADEON_AE, rt_r300, STD_RADEON, "Radeon 9700 AE" },
	{ DEVICE_ID_RADEON_AF, rt_r300, STD_RADEON, "Radeon 9700 AF" },
	{ DEVICE_ID_RADEON_AG, rt_r300, STD_RADEON, "Radeon 9700 AG" },

	// RV350
	{ DEVICE_ID_RADEON_AP, rt_rv350, STD_RADEON, "Radeon 9600 AP" },
	{ DEVICE_ID_RADEON_AQ, rt_rv350, STD_RADEON, "Radeon 9600SE AQ" },
	{ DEVICE_ID_RADEON_AR, rt_rv350, STD_RADEON, "Radeon 9600XT AR" },
	{ DEVICE_ID_RADEON_AS, rt_rv350, STD_RADEON, "Radeon 9550 AS" },
	{ DEVICE_ID_RADEON_AT, rt_rv350, STD_RADEON, "FireGL T2 AT" },
	{ DEVICE_ID_RADEON_4155, rt_rv350, STD_RADEON, "Radeon 9650 4155" },
	{ DEVICE_ID_RADEON_AV, rt_rv350, STD_RADEON, "Radeon 9600 AQ" },

	// rv350 M10 (based on rv350) was rt_m10
	{ DEVICE_ID_RADEON_NP, rt_rv350, ISMOBILITY, "Radeon Mobility 9600/9700 (M10/M11) NP " },
	{ DEVICE_ID_RADEON_NQ, rt_rv350, ISMOBILITY, "Radeon Mobility 9600 (M10) NQ " },
	{ DEVICE_ID_RADEON_NR, rt_rv350, ISMOBILITY, "Radeon Mobility 9600 (M11) NR " },
	{ DEVICE_ID_RADEON_NS, rt_rv350, ISMOBILITY, "Radeon Mobility 9600 (M10) NS " },
	{ DEVICE_ID_RADEON_NT, rt_rv350, ISMOBILITY, "ATI FireGL Mobility T2 (M10) NT" },
	{ DEVICE_ID_RADEON_NV, rt_rv350, ISMOBILITY, "ATI FireGL Mobility T2e (M11) NV" },

	// R350
	{ DEVICE_ID_RADEON_AH, rt_r350, STD_RADEON, "Radeon 9800SE AH" },
	{ DEVICE_ID_RADEON_AI, rt_r350, STD_RADEON, "Radeon 9800 AI" },
	{ DEVICE_ID_RADEON_AJ, rt_r350, STD_RADEON, "Radeon 9800 AJ" },
	{ DEVICE_ID_RADEON_AK, rt_r350, STD_RADEON, "FireGL X2 AK" },
	{ DEVICE_ID_RADEON_NH, rt_r350, STD_RADEON, "Radeon 9800 Pro NH" },
	{ DEVICE_ID_RADEON_NI, rt_r350, STD_RADEON, "Radeon 9800 NI" },
	{ DEVICE_ID_RADEON_NK, rt_r350, STD_RADEON, "FireGL X2 NK" },
	{ DEVICE_ID_RADEON_NJ, rt_r350, STD_RADEON, "Radeon 9800 XT" },

	// rv370
	{ DEVICE_ID_RADEON_5b60, rt_rv380, STD_RADEON, "Radeon X300 (RV370) 5B60" },
	{ DEVICE_ID_RADEON_5b62, rt_rv380, STD_RADEON, "Radeon X600 (RV370) 5B62" },
	{ DEVICE_ID_RADEON_5b63, rt_rv380, STD_RADEON, "Radeon X1050 (RV370) 5B63" },
	{ DEVICE_ID_RADEON_5b64, rt_rv380, STD_RADEON, "FireGL V3100 (RV370) 5B64" },
	{ DEVICE_ID_RADEON_5b65, rt_rv380, STD_RADEON, "FireGL D1100 (RV370) 5B65" },
	{ DEVICE_ID_RADEON_5460, rt_rv380, ISMOBILITY, "Radeon Mobility M300 (M22) 5460" },
	{ DEVICE_ID_RADEON_5464, rt_rv380, ISMOBILITY, "FireGL M22 GL 5464" },

	// rv380
	{ DEVICE_ID_RADEON_3e50, rt_rv380, STD_RADEON, "Radeon X600 (RV380) 3E50" },
	{ DEVICE_ID_RADEON_3e54, rt_rv380, STD_RADEON, "FireGL V3200 (RV380) 3E54" },
	{ DEVICE_ID_RADEON_3150, rt_rv380, ISMOBILITY, "Radeon Mobility X600 (M24) 3150" },
	{ DEVICE_ID_RADEON_3154, rt_rv380, ISMOBILITY, "FireGL M24 GL 3154" },
	{ DEVICE_ID_RADEON_5462, rt_rv380, ISMOBILITY, "Radeon X600SE (RV3?0) 5462" },

	// rv380
	{ DEVICE_ID_RADEON_5b62, rt_rv380, STD_RADEON, "Radeon X600 AIW" },

	// rv410
	{ DEVICE_ID_RADEON_5e48, rt_r420, STD_RADEON, "FireGL V5000 (RV410)" },
	{ DEVICE_ID_RADEON_564a, rt_r420, ISMOBILITY, "Mobility FireGL V5000 (M26)" },
	{ DEVICE_ID_RADEON_564b, rt_r420, ISMOBILITY, "Mobility FireGL V5000 (M26)" },
	{ DEVICE_ID_RADEON_564f, rt_r420, ISMOBILITY, "Mobility Radeon X700 XL (M26)" },
	{ DEVICE_ID_RADEON_5652, rt_r420, ISMOBILITY, "Mobility Radeon X700 (M26)" },
	{ DEVICE_ID_RADEON_5653, rt_r420, ISMOBILITY, "Mobility Radeon X700 (M26)" },
	{ DEVICE_ID_RADEON_5e4b, rt_r420, STD_RADEON, "Radeon X700 PRO (RV410)" },
	{ DEVICE_ID_RADEON_5e4a, rt_r420, STD_RADEON, "Radeon X700 XT (RV410)" },
	{ DEVICE_ID_RADEON_5e4d, rt_r420, STD_RADEON, "Radeon X700 (RV410)" },
	{ DEVICE_ID_RADEON_5e4c, rt_r420, STD_RADEON, "Radeon X700 SE (RV410)" },
	{ DEVICE_ID_RADEON_5e4f, rt_r420, STD_RADEON, "Radeon X700 SE (RV410)" },

	// r420
	{ DEVICE_ID_RADEON_JH, rt_r420, STD_RADEON, "Radeon X800 (R420) JH" },
	{ DEVICE_ID_RADEON_JI, rt_r420, STD_RADEON, "Radeon X800PRO (R420) JI" },
	{ DEVICE_ID_RADEON_JJ, rt_r420, STD_RADEON, "Radeon X800SE (R420) JJ" },
	{ DEVICE_ID_RADEON_JK, rt_r420, STD_RADEON, "Radeon X800 (R420) JK" },
	{ DEVICE_ID_RADEON_JL, rt_r420, STD_RADEON, "Radeon X800 (R420) JL" },
	{ DEVICE_ID_RADEON_JM, rt_r420, STD_RADEON, "FireGL X3 (R420) JM" },
	{ DEVICE_ID_RADEON_JN, rt_r420, ISMOBILITY, "Radeon Mobility 9800 (M18) JN" },
	{ DEVICE_ID_RADEON_JP, rt_r420, STD_RADEON, "Radeon X800XT (R420) JP" },
	{ DEVICE_ID_RADEON_4a4f, rt_r420, STD_RADEON, "Radeon X800 SE (R420)" },

	// r423
	{ DEVICE_ID_RADEON_UH, rt_r420, STD_RADEON, "Radeon X800 (R423) UH" },
	{ DEVICE_ID_RADEON_UI, rt_r420, STD_RADEON, "Radeon X800PRO (R423) UI" },
	{ DEVICE_ID_RADEON_UJ, rt_r420, STD_RADEON, "Radeon X800LE (R423) UJ" },
	{ DEVICE_ID_RADEON_UK, rt_r420, STD_RADEON, "Radeon X800SE (R423) UK" },
	{ DEVICE_ID_RADEON_UQ, rt_r420, STD_RADEON, "FireGL V7200 (R423) UQ" },
	{ DEVICE_ID_RADEON_UR, rt_r420, STD_RADEON, "FireGL V5100 (R423) UR" },
	{ DEVICE_ID_RADEON_UT, rt_r420, STD_RADEON, "FireGL V7100 (R423) UT" },

	{ DEVICE_ID_RADEON_UO, rt_r420, STD_RADEON, "Radeon X800 UO" },
	{ DEVICE_ID_RADEON_UM, rt_r420, STD_RADEON, "Radeon X800 UM" },

	{ DEVICE_ID_RADEON_5d57, rt_r420, STD_RADEON, "Radeon X800 XT" },
	{ DEVICE_ID_RADEON_5550, rt_r420, STD_RADEON, "FireGL V7100 (R423)" },

	// r430
	{ DEVICE_ID_RADEON_5d49, rt_r420, ISMOBILITY, "Mobility FireGL V5100 (M28)" },
	{ DEVICE_ID_RADEON_5d4a, rt_r420, ISMOBILITY, "Mobility Radeon X800 (M28)" },
	{ DEVICE_ID_RADEON_5d48, rt_r420, ISMOBILITY, "Mobility Radeon X800 XT (M28)" },
	{ DEVICE_ID_RADEON_554f, rt_r420, STD_RADEON, "Radeon X800 (R430)" },
	{ DEVICE_ID_RADEON_554d, rt_r420, STD_RADEON, "Radeon X800 XL (R430)" },
	{ DEVICE_ID_RADEON_554e, rt_r420, STD_RADEON, "Radeon X800 SE (R430)" },
	{ DEVICE_ID_RADEON_554c, rt_r420, STD_RADEON, "Radeon X800 XTP (R430)" },

	// r480
	{ DEVICE_ID_RADEON_5d4c, rt_r420, STD_RADEON, "Radeon X850 5D4C" },
	{ DEVICE_ID_RADEON_5d50, rt_r420, STD_RADEON, "Radeon FireGL (R480) GL 5D50" },
	{ DEVICE_ID_RADEON_5d4e, rt_r420, STD_RADEON, "Radeon X850 SE (R480)" },
	{ DEVICE_ID_RADEON_5d4f, rt_r420, STD_RADEON, "Radeon X850 PRO (R480)" },
	{ DEVICE_ID_RADEON_5d52, rt_r420, STD_RADEON, "Radeon X850 XT (R480)" },
	{ DEVICE_ID_RADEON_5d4d, rt_r420, STD_RADEON, "Radeon X850 XT PE (R480)" },

	// r481
	{ DEVICE_ID_RADEON_KJ, rt_r420, STD_RADEON, "Radeon X850 PRO (R480)" },
	{ DEVICE_ID_RADEON_KK, rt_r420, STD_RADEON, "Radeon X850 SE (R480)" },
	{ DEVICE_ID_RADEON_KL, rt_r420, STD_RADEON, "Radeon X850 XT (R480)" },
	{ DEVICE_ID_RADEON_KI, rt_r420, STD_RADEON, "Radeon X850 XT PE (R480)" },

	// rs100 (aka IGP 320)
	{ DEVICE_ID_RS100_4136, rt_rs100, INTEGRATED, "Radeon IGP320 (A3) 4136" },
	{ DEVICE_ID_RS100_4336, rt_rs100, MOBILE_IGP, "Radeon IGP320M (U1) 4336" },

	// rs200 (aka IGP 340)
	{ DEVICE_ID_RS200_4137, rt_rs200, INTEGRATED, "Radeon IGP330/340/350 (A4) 4137" },
	{ DEVICE_ID_RS200_4337, rt_rs200, MOBILE_IGP, "Radeon IGP330M/340M/350M (U2) 4337" },

	// rs250 (aka 7000 IGP)
	{ DEVICE_ID_RS250_4237, rt_rs200, INTEGRATED, "IGP330M/340M/350M (U2) 4337" },
	{ DEVICE_ID_RS250_4437, rt_rs200, MOBILE_IGP, "Radeon Mobility 7000 IGP 4437" },

	// rs300
	{ DEVICE_ID_RS300_5834, rt_rs300, INTEGRATED, "Radeon 9100 IGP (A5) 5834" },
	{ DEVICE_ID_RS300_5835, rt_rs300, MOBILE_IGP, "Radeon Mobility 9100 IGP (U3) 5835" },

	// rs350
	{ DEVICE_ID_RS350_7834, rt_rs300, INTEGRATED, "Radeon 9100 PRO IGP 7834" },
	{ DEVICE_ID_RS350_7835, rt_rs300, MOBILE_IGP, "Radeon Mobility 9200 IGP 7835" },

	// rs400
	{ DEVICE_ID_RS400_5a41, rt_rv380, STD_RADEON, "Radeon XPRESS 200 5A41" }, // X.org people unsure what this is for now
	{ DEVICE_ID_RS400_5a42, rt_rv380, ISMOBILITY, "Radeon XPRESS 200M 5A42" },

	// rs410
	{ DEVICE_ID_RS410_5a61, rt_rv380, STD_RADEON, "Radeon XPRESS 200 5A61" }, // X.org people unsure what this is for now
	{ DEVICE_ID_RS410_5a62, rt_rv380, ISMOBILITY, "Radeon XPRESS 200M 5A62" },

	// rs480
	{ DEVICE_ID_RS480_5954, rt_rv380, STD_RADEON, "Radeon XPRESS 200 5954" }, // X.org people unsure what this is for now
	{ DEVICE_ID_RS480_5955, rt_rv380, ISMOBILITY, "Radeon XPRESS 200M 5955" },
	{ DEVICE_ID_RS482_5974, rt_rv380, STD_RADEON, "Radeon XPRESS 200 5974" }, // X.org people unsure what this is for now
	{ DEVICE_ID_RS482_5975, rt_rv380, ISMOBILITY, "Radeon XPRESS 200M 5975" },

	{ 0, 0, 0, NULL }
};


// list of supported vendors (there aren't many ;)
static struct {
	uint16	vendor_id;
	RadeonDevice *devices;
} SupportedVendors[] = {
	{ VENDOR_ID_ATI, radeon_device_list },
	{ 0x0000, NULL }
};

// check, whether there is *any* supported card plugged in
bool Radeon_CardDetect(void)
{
	long		pci_index = 0;
	pci_info	pcii;
	bool		found_one = FALSE;

	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci_bus) != B_OK)
		return B_ERROR;

	while ((*pci_bus->get_nth_pci_info)(pci_index, &pcii) == B_NO_ERROR) {
		int vendor = 0;

		while (SupportedVendors[vendor].vendor_id) {
			if (SupportedVendors[vendor].vendor_id == pcii.vendor_id) {
				RadeonDevice *devices = SupportedVendors[vendor].devices;

				while (devices->device_id) {
					if (devices->device_id == pcii.device_id) {
						rom_info ri;

						if (Radeon_MapBIOS(&pcii, &ri) == B_OK) {
							Radeon_UnmapBIOS(&ri);

							SHOW_INFO(0, "found supported device"
								" pci index %ld, device 0x%04x/0x%04x",
								pci_index, pcii.vendor_id, pcii.device_id);
							found_one = TRUE;
							goto done;
						}
					}
					devices++;
				}
			}
			vendor++;
		}

		pci_index++;
	}
	SHOW_INFO0(0, "no supported devices found");

done:
	put_module(B_PCI_MODULE_NAME);

	return (found_one ? B_OK : B_ERROR);
}


// !extend this array whenever a new ASIC is a added!
static struct {
	const char 		*name;			// name of ASIC
	tv_chip_type	tv_chip;		// TV-Out chip (if any)
	bool 			has_crtc2;		// has second CRTC
	bool			has_vip;		// has VIP/I2C
	bool			new_pll;		// reference divider of PPLL moved to other location
} asic_properties[] =
{
	{ "r100",	tc_external_rt1,	false,	true,	false},
	{ "rv100",	tc_internal_rt1, 	true,	true,	false},
	{ "rs100",	tc_internal_rt1,	true,	false,	false},
	{ "rv200",	tc_internal_rt2, 	true,	true,	false},
	{ "rs200",	tc_internal_rt1, 	true,	false,	false},
	{ "r200",	tc_external_rt1, 	true,	true,	false},
	{ "rv250",	tc_internal_rt2, 	true,	true,	false},
	{ "rs300",	tc_internal_rt1, 	true,	false,	false},
	{ "rv280",	tc_internal_rt2, 	true,	true,	false},
	{ "r300",	tc_internal_rt2, 	true,	true,	true },
	{ "r350",	tc_internal_rt2, 	true,	true,	true },
	{ "rv350",	tc_internal_rt2, 	true,	true,	true },
	{ "rv380",	tc_internal_rt2, 	true,	true,	true },
	{ "r420",	tc_internal_rt2, 	true,	true,	true }

};


// get next supported device
static bool probeDevice(device_info *di)
{
	int vendor;

	/* if we match a supported vendor */
	for (vendor = 0; SupportedVendors[vendor].vendor_id; ++vendor) {
		RadeonDevice *device;

		if (SupportedVendors[vendor].vendor_id != di->pcii.vendor_id)
			continue;

		for (device = SupportedVendors[vendor].devices; device->device_id;
			++device) {
			// avoid double-detection
			if (device->device_id != di->pcii.device_id)
				continue;

			di->num_crtc = asic_properties[device->asic].has_crtc2 ? 2 : 1;
			di->tv_chip = asic_properties[device->asic].tv_chip;
			di->asic = device->asic;
			di->is_mobility = (device->features & ISMOBILITY) ? true : false;
			di->has_vip = asic_properties[device->asic].has_vip;
			di->new_pll = asic_properties[device->asic].new_pll;
			di->is_igp = (device->features & INTEGRATED) ? true : false;

			// detect chips with broken I2C
			switch (device->device_id) {
				case DEVICE_ID_RADEON_LY:
				case DEVICE_ID_RADEON_LZ:
				case DEVICE_ID_RADEON_LW:
				case DEVICE_ID_RADEON_LX:
				case DEVICE_ID_RADEON_If:
				case DEVICE_ID_RADEON_Ig:
				case DEVICE_ID_RADEON_5460:
				case DEVICE_ID_RADEON_5464:
					di->has_no_i2c = true;
					break;
				default:
					di->has_no_i2c = false;
			}

			// disable 2d DMA engine for chips that don't work with our
			// dma code (yet).  I would have used asic type, but R410s are
			// treated same asic as r420s in code, and the AGP x800 works fine.
			switch (device->device_id) {
				// rv370
				case DEVICE_ID_RADEON_5b60:
				case DEVICE_ID_RADEON_5b62:
				case DEVICE_ID_RADEON_5b64:
				case DEVICE_ID_RADEON_5b65:
				case DEVICE_ID_RADEON_5460:
				case DEVICE_ID_RADEON_5464:

				// rv380
				case DEVICE_ID_RADEON_3e50:
				case DEVICE_ID_RADEON_3e54:
				case DEVICE_ID_RADEON_3150:
				case DEVICE_ID_RADEON_3154:
				case DEVICE_ID_RADEON_5462:

				// rv410
				case DEVICE_ID_RADEON_5e48:
				case DEVICE_ID_RADEON_564a:
				case DEVICE_ID_RADEON_564b:
				case DEVICE_ID_RADEON_564f:
				case DEVICE_ID_RADEON_5652:
				case DEVICE_ID_RADEON_5653:
				case DEVICE_ID_RADEON_5e4b:
				case DEVICE_ID_RADEON_5e4a:
				case DEVICE_ID_RADEON_5e4d:
				case DEVICE_ID_RADEON_5e4c:
				case DEVICE_ID_RADEON_5e4f:

				// rs400
				case DEVICE_ID_RS400_5a41:
				case DEVICE_ID_RS400_5a42:

				// rs410
				case DEVICE_ID_RS410_5a61:
				case DEVICE_ID_RS410_5a62:

				// r430
				case DEVICE_ID_RADEON_UM:

				// rs480
				case DEVICE_ID_RS480_5954:
				case DEVICE_ID_RS480_5955:
				case DEVICE_ID_RS482_5974:
				case DEVICE_ID_RS482_5975:
					di->acc_dma = false;
					break;
				default:
					di->acc_dma = true;
					break;
			}

			if (Radeon_MapBIOS(&di->pcii, &di->rom) != B_OK)
				// give up checking this device - no BIOS, no fun
				return false;

			if (Radeon_ReadBIOSData(di) != B_OK) {
				Radeon_UnmapBIOS(&di->rom);
				return false;
			}

			// we don't need BIOS any more
			Radeon_UnmapBIOS(&di->rom);

			SHOW_INFO(0, "found %s; ASIC: %s", device->name,
				asic_properties[device->asic].name);

			sprintf(di->name, "graphics/%04X_%04X_%02X%02X%02X",
				di->pcii.vendor_id, di->pcii.device_id,
				di->pcii.bus, di->pcii.device, di->pcii.function);
			SHOW_FLOW(3, "making /dev/%s", di->name);

			// we always publish it as a video grabber; we should check for Rage
			// Theater, but the corresponding code (vip.c) needs a fully
			// initialized driver, and this is too much hazzly, so we leave it
			// to the media add-on to verify that the card supports video-in
			sprintf(di->video_name, "video/radeon/%04X_%04X_%02X%02X%02X",
				di->pcii.vendor_id, di->pcii.device_id,
				di->pcii.bus, di->pcii.device, di->pcii.function);

			di->is_open = 0;
			di->shared_area = -1;
			di->si = NULL;

			return true;
		}
	}

	return false;
}


// gather list of supported devices
// (currently, we rely on proper BIOS detection, which
//  only works for primary graphics adapter, so multiple
//  devices won't really work)
void Radeon_ProbeDevices(void)
{
	uint32 pci_index = 0;
	uint32 count = 0;
	device_info *di = devices->di;

	while (count < MAX_DEVICES) {
		memset(di, 0, sizeof(*di));

		if ((*pci_bus->get_nth_pci_info)(pci_index, &(di->pcii)) != B_NO_ERROR)
			break;

		if (probeDevice(di)) {
			devices->device_names[2 * count] = di->name;
			devices->device_names[2 * count + 1] = di->video_name;
			di++;
			count++;
		}

		pci_index++;
	}

	devices->count = count;
	devices->device_names[2 * count] = NULL;

	SHOW_INFO(0, "%" B_PRIu32 " supported devices", count);
}
