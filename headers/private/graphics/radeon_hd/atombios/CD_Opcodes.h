/*
 * Copyright 2006-2007 Advanced Micro Devices, Inc.  
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
 */

/*++

Module Name:

CD_OPCODEs.h

Abstract:

Defines Command Decoder OPCODEs

Revision History:

NEG:24.09.2002	Initiated.
--*/
#ifndef _CD_OPCODES_H_
#define _CD_OPCODES_H_

typedef enum _OPCODE {
    Reserved_00= 0,				//	0	= 0x00
    // MOVE_ group
    MOVE_REG_OPCODE,			//	1	= 0x01
    FirstValidCommand=MOVE_REG_OPCODE,
    MOVE_PS_OPCODE,				//	2	= 0x02
    MOVE_WS_OPCODE,				//	3	= 0x03
    MOVE_FB_OPCODE,				//	4	= 0x04
    MOVE_PLL_OPCODE,			//	5	= 0x05
    MOVE_MC_OPCODE,				//	6	= 0x06
    // Logic group
    AND_REG_OPCODE,				//	7	= 0x07
    AND_PS_OPCODE,				//	8	= 0x08
    AND_WS_OPCODE,				//	9	= 0x09
    AND_FB_OPCODE,				//	10	= 0x0A
    AND_PLL_OPCODE,				//	11	= 0x0B
    AND_MC_OPCODE,				//	12	= 0x0C
    OR_REG_OPCODE,				//	13	= 0x0D
    OR_PS_OPCODE,				//	14	= 0x0E
    OR_WS_OPCODE,				//	15	= 0x0F
    OR_FB_OPCODE,				//	16	= 0x10
    OR_PLL_OPCODE,				//	17	= 0x11
    OR_MC_OPCODE,				//	18	= 0x12
    SHIFT_LEFT_REG_OPCODE,		//	19	= 0x13
    SHIFT_LEFT_PS_OPCODE,		//	20	= 0x14
    SHIFT_LEFT_WS_OPCODE,		//	21	= 0x15
    SHIFT_LEFT_FB_OPCODE,		//	22	= 0x16
    SHIFT_LEFT_PLL_OPCODE,		//	23	= 0x17
    SHIFT_LEFT_MC_OPCODE,		//	24	= 0x18
    SHIFT_RIGHT_REG_OPCODE,		//	25	= 0x19
    SHIFT_RIGHT_PS_OPCODE,		//	26	= 0x1A
    SHIFT_RIGHT_WS_OPCODE,		//	27	= 0x1B
    SHIFT_RIGHT_FB_OPCODE,		//	28	= 0x1C
    SHIFT_RIGHT_PLL_OPCODE,		//	29	= 0x1D
    SHIFT_RIGHT_MC_OPCODE,		//	30	= 0x1E
    // Arithmetic group
    MUL_REG_OPCODE,				//	31	= 0x1F
    MUL_PS_OPCODE,				//	32	= 0x20
    MUL_WS_OPCODE,				//	33	= 0x21
    MUL_FB_OPCODE,				//	34	= 0x22
    MUL_PLL_OPCODE,				//	35	= 0x23
    MUL_MC_OPCODE,				//	36	= 0x24
    DIV_REG_OPCODE,				//	37	= 0x25
    DIV_PS_OPCODE,				//	38	= 0x26
    DIV_WS_OPCODE,				//	39	= 0x27
    DIV_FB_OPCODE,				//	40	= 0x28
    DIV_PLL_OPCODE,				//	41	= 0x29
    DIV_MC_OPCODE,				//	42	= 0x2A
    ADD_REG_OPCODE,				//	43	= 0x2B
    ADD_PS_OPCODE,				//	44	= 0x2C
    ADD_WS_OPCODE,				//	45	= 0x2D
    ADD_FB_OPCODE,				//	46	= 0x2E
    ADD_PLL_OPCODE,				//	47	= 0x2F
    ADD_MC_OPCODE,				//	48	= 0x30
    SUB_REG_OPCODE,				//	49	= 0x31
    SUB_PS_OPCODE,				//	50	= 0x32
    SUB_WS_OPCODE,				//	51	= 0x33
    SUB_FB_OPCODE,				//	52	= 0x34
    SUB_PLL_OPCODE,				//	53	= 0x35
    SUB_MC_OPCODE,				//	54	= 0x36
    // Control grouop
    SET_ATI_PORT_OPCODE,		//	55	= 0x37
    SET_PCI_PORT_OPCODE,		//	56	= 0x38
    SET_SYS_IO_PORT_OPCODE,		//	57	= 0x39
    SET_REG_BLOCK_OPCODE,		//	58	= 0x3A
    SET_FB_BASE_OPCODE,			//	59	= 0x3B
    COMPARE_REG_OPCODE,			//	60	= 0x3C
    COMPARE_PS_OPCODE,			//	61	= 0x3D
    COMPARE_WS_OPCODE,			//	62	= 0x3E
    COMPARE_FB_OPCODE,			//	63	= 0x3F
    COMPARE_PLL_OPCODE,			//	64	= 0x40
    COMPARE_MC_OPCODE,			//	65	= 0x41
    SWITCH_OPCODE,				//	66	= 0x42
    JUMP__OPCODE,				//	67	= 0x43
    JUMP_EQUAL_OPCODE,			//	68	= 0x44
    JUMP_BELOW_OPCODE,			//	69	= 0x45
    JUMP_ABOVE_OPCODE,			//	70	= 0x46
    JUMP_BELOW_OR_EQUAL_OPCODE,	//	71	= 0x47
    JUMP_ABOVE_OR_EQUAL_OPCODE,	//	72	= 0x48
    JUMP_NOT_EQUAL_OPCODE,		//	73	= 0x49
    TEST_REG_OPCODE,			//	74	= 0x4A
    TEST_PS_OPCODE,				//	75	= 0x4B
    TEST_WS_OPCODE,				//	76	= 0x4C
    TEST_FB_OPCODE,				//	77	= 0x4D
    TEST_PLL_OPCODE,			//	78	= 0x4E
    TEST_MC_OPCODE,				//	79	= 0x4F
    DELAY_MILLISEC_OPCODE,		//	80	= 0x50
    DELAY_MICROSEC_OPCODE,		//	81	= 0x51
    CALL_TABLE_OPCODE,			//	82	= 0x52
    REPEAT_OPCODE,				//	83	= 0x53
    //	Miscellaneous	group
    CLEAR_REG_OPCODE,			//	84	= 0x54
    CLEAR_PS_OPCODE,			//	85	= 0x55
    CLEAR_WS_OPCODE,			//	86	= 0x56
    CLEAR_FB_OPCODE,			//	87	= 0x57
    CLEAR_PLL_OPCODE,			//	88	= 0x58
    CLEAR_MC_OPCODE,			//	89	= 0x59
    NOP_OPCODE,					//	90	= 0x5A
    EOT_OPCODE,					//	91	= 0x5B
    MASK_REG_OPCODE,			//	92	= 0x5C
    MASK_PS_OPCODE,				//	93	= 0x5D
    MASK_WS_OPCODE,				//	94	= 0x5E
    MASK_FB_OPCODE,				//	95	= 0x5F
    MASK_PLL_OPCODE,			//	96	= 0x60
    MASK_MC_OPCODE,				//	97	= 0x61
    // BIOS dedicated group
    POST_CARD_OPCODE,			//	98	= 0x62
    BEEP_OPCODE,				//	99	= 0x63
    SAVE_REG_OPCODE,			//	100 = 0x64
    RESTORE_REG_OPCODE,			//	101	= 0x65
    SET_DATA_BLOCK_OPCODE,			//	102     = 0x66

    XOR_REG_OPCODE,				//	103	= 0x67
    XOR_PS_OPCODE,				//	104	= 0x68
    XOR_WS_OPCODE,				//	105	= 0x69
    XOR_FB_OPCODE,				//	106	= 0x6a
    XOR_PLL_OPCODE,				//	107	= 0x6b
    XOR_MC_OPCODE,				//	108	= 0x6c

    SHL_REG_OPCODE,				//	109	= 0x6d
    SHL_PS_OPCODE,				//	110	= 0x6e
    SHL_WS_OPCODE,				//	111	= 0x6f
    SHL_FB_OPCODE,				//	112	= 0x70
    SHL_PLL_OPCODE,				//	113	= 0x71
    SHL_MC_OPCODE,				//	114	= 0x72

    SHR_REG_OPCODE,				//	115	= 0x73
    SHR_PS_OPCODE,				//	116	= 0x74
    SHR_WS_OPCODE,				//	117	= 0x75
    SHR_FB_OPCODE,				//	118	= 0x76
    SHR_PLL_OPCODE,				//	119	= 0x77
    SHR_MC_OPCODE,				//	120	= 0x78

    DEBUG_OPCODE,                           //	121	= 0x79
    CTB_DS_OPCODE,                          //	122	= 0x7A

    LastValidCommand = CTB_DS_OPCODE,
    //	Extension specificaTOR
    Extension	= 0x80,			//	128 = 0x80	// Next byte is an OPCODE as well
    Reserved_FF = 255			//	255 = 0xFF
}OPCODE;
#endif		// _CD_OPCODES_H_
