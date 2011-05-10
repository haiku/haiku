/*
 * RadeonHD R6xx, R7xx Register documentation
 *
 * Copyright (C) 2008-2009  Advanced Micro Devices, Inc.
 * Copyright (C) 2008-2009  Matthias Hopf
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _R600_REG_R7xx_H_
#define _R600_REG_R7xx_H_

/*
 * Register update for R7xx chips
 */

enum {

    /* R7XX_MC_VM_FB_LOCATION                             = 0x00002024, */

/*  GRBM_STATUS                                           = 0x00008010, */
	R7XX_TA_BUSY_bit                                  = 1 << 14,

    R7xx_SQ_DYN_GPR_CNTL_PS_FLUSH_REQ                     = 0x00008d8c,
	RING0_OFFSET_mask                                 = 0xff << 0,
	RING0_OFFSET_shift                                = 0,
	ISOLATE_ES_ENABLE_bit                             = 1 << 12,
	ISOLATE_GS_ENABLE_bit                             = 1 << 13,
	VS_PC_LIMIT_ENABLE_bit                            = 1 << 14,

/*  SQ_ALU_WORD0                                          = 0x00008dfc, */
/*	SRC0_SEL_mask                                     = 0x1ff << 0, */
/* 	SRC1_SEL_mask                                     = 0x1ff << 13, */
	    R7xx_SQ_ALU_SRC_1_DBL_L                       = 0xf4,
	    R7xx_SQ_ALU_SRC_1_DBL_M                       = 0xf5,
	    R7xx_SQ_ALU_SRC_0_5_DBL_L                     = 0xf6,
	    R7xx_SQ_ALU_SRC_0_5_DBL_M                     = 0xf7,
/* 	INDEX_MODE_mask                                   = 0x07 << 26, */
	    R7xx_SQ_INDEX_GLOBAL                          = 0x05,
	    R7xx_SQ_INDEX_GLOBAL_AR_X                     = 0x06,
    R6xx_SQ_ALU_WORD1_OP2                                 = 0x00008dfc,
    R7xx_SQ_ALU_WORD1_OP2_V2                              = 0x00008dfc,
	R6xx_FOG_MERGE_bit                                = 1 << 5,
	R6xx_OMOD_mask                                    = 0x03 << 6,
	R7xx_OMOD_mask                                    = 0x03 << 5,
	R6xx_OMOD_shift                                   = 6,
	R7xx_OMOD_shift                                   = 5,
	R6xx_SQ_ALU_WORD1_OP2__ALU_INST_mask              = 0x3ff << 8,
	R7xx_SQ_ALU_WORD1_OP2_V2__ALU_INST_mask           = 0x7ff << 7,
	R6xx_SQ_ALU_WORD1_OP2__ALU_INST_shift             = 8,
	R7xx_SQ_ALU_WORD1_OP2_V2__ALU_INST_shift          = 7,
	    R7xx_SQ_OP2_INST_FREXP_64                     = 0x07,
	    R7xx_SQ_OP2_INST_ADD_64                       = 0x17,
	    R7xx_SQ_OP2_INST_MUL_64                       = 0x1b,
	    R7xx_SQ_OP2_INST_FLT64_TO_FLT32               = 0x1c,
	    R7xx_SQ_OP2_INST_FLT32_TO_FLT64               = 0x1d,
	    R7xx_SQ_OP2_INST_LDEXP_64                     = 0x7a,
	    R7xx_SQ_OP2_INST_FRACT_64                     = 0x7b,
	    R7xx_SQ_OP2_INST_PRED_SETGT_64                = 0x7c,
	    R7xx_SQ_OP2_INST_PRED_SETE_64                 = 0x7d,
	    R7xx_SQ_OP2_INST_PRED_SETGE_64                = 0x7e,
/*  SQ_ALU_WORD1_OP3                                      = 0x00008dfc, */
/*	SRC2_SEL_mask                                     = 0x1ff << 0, */
/*	    R7xx_SQ_ALU_SRC_1_DBL_L                       = 0xf4, */
/*	    R7xx_SQ_ALU_SRC_1_DBL_M                       = 0xf5, */
/*	    R7xx_SQ_ALU_SRC_0_5_DBL_L                     = 0xf6, */
/*	    R7xx_SQ_ALU_SRC_0_5_DBL_M                     = 0xf7, */
/* 	SQ_ALU_WORD1_OP3__ALU_INST_mask                   = 0x1f << 13, */
	    R7xx_SQ_OP3_INST_MULADD_64                    = 0x08,
	    R7xx_SQ_OP3_INST_MULADD_64_M2                 = 0x09,
	    R7xx_SQ_OP3_INST_MULADD_64_M4                 = 0x0a,
	    R7xx_SQ_OP3_INST_MULADD_64_D2                 = 0x0b,
/*  SQ_CF_ALU_WORD1                                       = 0x00008dfc, */
	R6xx_USES_WATERFALL_bit                           = 1 << 25,
	R7xx_SQ_CF_ALU_WORD1__ALT_CONST_bit               = 1 << 25,
/*  SQ_CF_ALLOC_EXPORT_WORD0                              = 0x00008dfc, */
/*	ARRAY_BASE_mask                                   = 0x1fff << 0, */
/*	TYPE_mask                                         = 0x03 << 13, */
/*	    SQ_EXPORT_PARAM                               = 0x02, */
/*	    X_UNUSED_FOR_SX_EXPORTS                       = 0x03, */
/*	ELEM_SIZE_mask                                    = 0x03 << 30, */
/*  SQ_CF_ALLOC_EXPORT_WORD1                              = 0x00008dfc, */
/*	SQ_CF_ALLOC_EXPORT_WORD1__CF_INST_mask            = 0x7f << 23, */
	    R7xx_SQ_CF_INST_MEM_EXPORT                    = 0x3a,
/*  SQ_CF_WORD1                                           = 0x00008dfc, */
/*	SQ_CF_WORD1__COUNT_mask                           = 0x07 << 10, */
	R7xx_COUNT_3_bit                                  = 1 << 19,
/*	SQ_CF_WORD1__CF_INST_mask                         = 0x7f << 23, */
	    R7xx_SQ_CF_INST_END_PROGRAM                   = 0x19,
	    R7xx_SQ_CF_INST_WAIT_ACK                      = 0x1a,
	    R7xx_SQ_CF_INST_TEX_ACK                       = 0x1b,
	    R7xx_SQ_CF_INST_VTX_ACK                       = 0x1c,
	    R7xx_SQ_CF_INST_VTX_TC_ACK                    = 0x1d,
/*  SQ_VTX_WORD0                                          = 0x00008dfc, */
/*	VTX_INST_mask                                     = 0x1f << 0, */
	    R7xx_SQ_VTX_INST_MEM                          = 0x02,
/*  SQ_VTX_WORD2                                          = 0x00008dfc, */
	R7xx_SQ_VTX_WORD2__ALT_CONST_bit                  = 1 << 20,

/*  SQ_TEX_WORD0                                          = 0x00008dfc, */
/*	TEX_INST_mask                                     = 0x1f << 0, */
	    R7xx_X_MEMORY_READ                            = 0x02,
	    R7xx_SQ_TEX_INST_KEEP_GRADIENTS               = 0x0a,
	    R7xx_X_FETCH4_LOAD4_INSTRUCTION_FOR_DX10_1    = 0x0f,
	R7xx_SQ_TEX_WORD0__ALT_CONST_bit                  = 1 << 24,

    R7xx_PA_SC_EDGERULE                                   = 0x00028230,
    R7xx_SPI_THREAD_GROUPING                              = 0x000286c8,
	PS_GROUPING_mask                                  = 0x1f << 0,
	PS_GROUPING_shift                                 = 0,
	VS_GROUPING_mask                                  = 0x1f << 8,
	VS_GROUPING_shift                                 = 8,
	GS_GROUPING_mask                                  = 0x1f << 16,
	GS_GROUPING_shift                                 = 16,
	ES_GROUPING_mask                                  = 0x1f << 24,
	ES_GROUPING_shift                                 = 24,
    R7xx_CB_SHADER_CONTROL                                = 0x000287a0,
	RT0_ENABLE_bit                                    = 1 << 0,
	RT1_ENABLE_bit                                    = 1 << 1,
	RT2_ENABLE_bit                                    = 1 << 2,
	RT3_ENABLE_bit                                    = 1 << 3,
	RT4_ENABLE_bit                                    = 1 << 4,
	RT5_ENABLE_bit                                    = 1 << 5,
	RT6_ENABLE_bit                                    = 1 << 6,
	RT7_ENABLE_bit                                    = 1 << 7,
/*  DB_ALPHA_TO_MASK                                      = 0x00028d44, */
	R7xx_OFFSET_ROUND_bit                             = 1 << 16,
/*  SQ_TEX_SAMPLER_MISC_0                                 = 0x0003d03c, */
	R7xx_TRUNCATE_COORD_bit                           = 1 << 9,
	R7xx_DISABLE_CUBE_WRAP_bit                        = 1 << 10

} ;

#endif /* _R600_REG_R7xx_H_ */
