/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon driver
		
	3D registers
*/


#ifndef _3D_REGS_H
#define _3D_REGS_H


#define RADEON_PP_BORDER_COLOR_0            0x1d40
#define RADEON_PP_BORDER_COLOR_1            0x1d44
#define RADEON_PP_BORDER_COLOR_2            0x1d48
#define RADEON_PP_CNTL                      0x1c38
#       define RADEON_STIPPLE_ENABLE        (1 <<  0)
#       define RADEON_SCISSOR_ENABLE        (1 <<  1)
#       define RADEON_PATTERN_ENABLE        (1 <<  2)
#       define RADEON_SHADOW_ENABLE         (1 <<  3)
#       define RADEON_TEX_ENABLE_MASK       (0xf << 4)
#       define RADEON_TEX_0_ENABLE          (1 <<  4)
#       define RADEON_TEX_1_ENABLE          (1 <<  5)
#       define RADEON_TEX_2_ENABLE          (1 <<  6)
#       define RADEON_TEX_3_ENABLE          (1 <<  7)
#       define RADEON_TEX_BLEND_ENABLE_MASK (0xf << 12)
#       define RADEON_TEX_BLEND_0_ENABLE    (1 << 12)
#       define RADEON_TEX_BLEND_1_ENABLE    (1 << 13)
#       define RADEON_TEX_BLEND_2_ENABLE    (1 << 14)
#       define RADEON_TEX_BLEND_3_ENABLE    (1 << 15)
#       define RADEON_PLANAR_YUV_ENABLE     (1 << 20)
#       define RADEON_SPECULAR_ENABLE       (1 << 21)
#       define RADEON_FOG_ENABLE            (1 << 22)
#       define RADEON_ALPHA_TEST_ENABLE     (1 << 23)
#       define RADEON_ANTI_ALIAS_NONE       (0 << 24)
#       define RADEON_ANTI_ALIAS_LINE       (1 << 24)
#       define RADEON_ANTI_ALIAS_POLY       (2 << 24)
#       define RADEON_ANTI_ALIAS_LINE_POLY  (3 << 24)
#       define RADEON_BUMP_MAP_ENABLE       (1 << 26)
#       define RADEON_BUMPED_MAP_T0         (0 << 27)
#       define RADEON_BUMPED_MAP_T1         (1 << 27)
#       define RADEON_BUMPED_MAP_T2         (2 << 27)
#       define RADEON_TEX_3D_ENABLE_0       (1 << 29)
#       define RADEON_TEX_3D_ENABLE_1       (1 << 30)
#       define RADEON_MC_ENABLE             (1 << 31)
#define RADEON_PP_FOG_COLOR                 0x1c18
#       define RADEON_FOG_COLOR_MASK        0x00ffffff
#       define RADEON_FOG_VERTEX            (0 << 24)
#       define RADEON_FOG_TABLE             (1 << 24)
#       define RADEON_FOG_USE_DEPTH         (0 << 25)
#       define RADEON_FOG_USE_DIFFUSE_ALPHA (2 << 25)
#       define RADEON_FOG_USE_SPEC_ALPHA    (3 << 25)
#define RADEON_PP_LUM_MATRIX                0x1d00
#define RADEON_PP_MISC                      0x1c14
#       define RADEON_REF_ALPHA_MASK        0x000000ff
#       define RADEON_ALPHA_TEST_FAIL       (0 << 8)
#       define RADEON_ALPHA_TEST_LESS       (1 << 8)
#       define RADEON_ALPHA_TEST_LEQUAL     (2 << 8)
#       define RADEON_ALPHA_TEST_EQUAL      (3 << 8)
#       define RADEON_ALPHA_TEST_GEQUAL     (4 << 8)
#       define RADEON_ALPHA_TEST_GREATER    (5 << 8)
#       define RADEON_ALPHA_TEST_NEQUAL     (6 << 8)
#       define RADEON_ALPHA_TEST_PASS       (7 << 8)
#       define RADEON_ALPHA_TEST_OP_MASK    (7 << 8)
#       define RADEON_CHROMA_FUNC_FAIL      (0 << 16)
#       define RADEON_CHROMA_FUNC_PASS      (1 << 16)
#       define RADEON_CHROMA_FUNC_NEQUAL    (2 << 16)
#       define RADEON_CHROMA_FUNC_EQUAL     (3 << 16)
#       define RADEON_CHROMA_KEY_NEAREST    (0 << 18)
#       define RADEON_CHROMA_KEY_ZERO       (1 << 18)
#       define RADEON_SHADOW_ID_AUTO_INC    (1 << 20)
#       define RADEON_SHADOW_FUNC_EQUAL     (0 << 21)
#       define RADEON_SHADOW_FUNC_NEQUAL    (1 << 21)
#       define RADEON_SHADOW_PASS_1         (0 << 22)
#       define RADEON_SHADOW_PASS_2         (1 << 22)
#       define RADEON_RIGHT_HAND_CUBE_D3D   (0 << 24)
#       define RADEON_RIGHT_HAND_CUBE_OGL   (1 << 24)
#define RADEON_PP_ROT_MATRIX_0              0x1d58
#define RADEON_PP_ROT_MATRIX_1              0x1d5c
#define RADEON_PP_TXFILTER_0                0x1c54
#define RADEON_PP_TXFILTER_1                0x1c6c
#define RADEON_PP_TXFILTER_2                0x1c84
#       define RADEON_MAG_FILTER_NEAREST                   (0  <<  0)
#       define RADEON_MAG_FILTER_LINEAR                    (1  <<  0)
#       define RADEON_MAG_FILTER_MASK                      (1  <<  0)
#       define RADEON_MIN_FILTER_NEAREST                   (0  <<  1)
#       define RADEON_MIN_FILTER_LINEAR                    (1  <<  1)
#       define RADEON_MIN_FILTER_NEAREST_MIP_NEAREST       (2  <<  1)
#       define RADEON_MIN_FILTER_NEAREST_MIP_LINEAR        (3  <<  1)
#       define RADEON_MIN_FILTER_LINEAR_MIP_NEAREST        (6  <<  1)
#       define RADEON_MIN_FILTER_LINEAR_MIP_LINEAR         (7  <<  1)
#       define RADEON_MIN_FILTER_ANISO_NEAREST             (8  <<  1)
#       define RADEON_MIN_FILTER_ANISO_LINEAR              (9  <<  1)
#       define RADEON_MIN_FILTER_ANISO_NEAREST_MIP_NEAREST (10 <<  1)
#       define RADEON_MIN_FILTER_ANISO_NEAREST_MIP_LINEAR  (11 <<  1)
#       define RADEON_MIN_FILTER_MASK                      (15 <<  1)
#       define RADEON_LOD_BIAS_MASK                        (0xffff <<  8)
#       define RADEON_LOD_BIAS_SHIFT                       8
#       define RADEON_MAX_MIP_LEVEL_MASK                   (0x0f << 16)
#       define RADEON_MAX_MIP_LEVEL_SHIFT                  16
#       define RADEON_WRAPEN_S                             (1  << 22)
#       define RADEON_CLAMP_S_WRAP                         (0  << 23)
#       define RADEON_CLAMP_S_MIRROR                       (1  << 23)
#       define RADEON_CLAMP_S_CLAMP_LAST                   (2  << 23)
#       define RADEON_CLAMP_S_MIRROR_CLAMP_LAST            (3  << 23)
#       define RADEON_CLAMP_S_CLAMP_BORDER                 (4  << 23)
#       define RADEON_CLAMP_S_MIRROR_CLAMP_BORDER          (5  << 23)
#       define RADEON_CLAMP_S_MASK                         (7  << 23)
#       define RADEON_WRAPEN_T                             (1  << 26)
#       define RADEON_CLAMP_T_WRAP                         (0  << 27)
#       define RADEON_CLAMP_T_MIRROR                       (1  << 27)
#       define RADEON_CLAMP_T_CLAMP_LAST                   (2  << 27)
#       define RADEON_CLAMP_T_MIRROR_CLAMP_LAST            (3  << 27)
#       define RADEON_CLAMP_T_CLAMP_BORDER                 (4  << 27)
#       define RADEON_CLAMP_T_MIRROR_CLAMP_BORDER          (5  << 27)
#       define RADEON_CLAMP_T_MASK                         (7  << 27)
#       define RADEON_BORDER_MODE_OGL                      (0  << 31)
#       define RADEON_BORDER_MODE_D3D                      (1  << 31)
#define RADEON_PP_TXFORMAT_0                0x1c58
#define RADEON_PP_TXFORMAT_1                0x1c70
#define RADEON_PP_TXFORMAT_2                0x1c88
#       define RADEON_TXFORMAT_I8                 (0  <<  0)
#       define RADEON_TXFORMAT_AI88               (1  <<  0)
#       define RADEON_TXFORMAT_RGB332             (2  <<  0)
#       define RADEON_TXFORMAT_ARGB1555           (3  <<  0)
#       define RADEON_TXFORMAT_RGB565             (4  <<  0)
#       define RADEON_TXFORMAT_ARGB4444           (5  <<  0)
#       define RADEON_TXFORMAT_ARGB8888           (6  <<  0)
#       define RADEON_TXFORMAT_RGBA8888           (7  <<  0)
#       define RADEON_TXFORMAT_Y8                 (8  <<  0)
#       define RADEON_TXFORMAT_FORMAT_MASK        (31 <<  0)
#       define RADEON_TXFORMAT_FORMAT_SHIFT       0
#       define RADEON_TXFORMAT_APPLE_YUV_MODE     (1  <<  5)
#       define RADEON_TXFORMAT_ALPHA_IN_MAP       (1  <<  6)
#       define RADEON_TXFORMAT_NON_POWER2         (1  <<  7)
#       define RADEON_TXFORMAT_WIDTH_MASK         (15 <<  8)
#       define RADEON_TXFORMAT_WIDTH_SHIFT        8
#       define RADEON_TXFORMAT_HEIGHT_MASK        (15 << 12)
#       define RADEON_TXFORMAT_HEIGHT_SHIFT       12
#       define RADEON_TXFORMAT_ST_ROUTE_STQ0      (0  << 24)
#       define RADEON_TXFORMAT_ST_ROUTE_MASK      (3  << 24)
#       define RADEON_TXFORMAT_ST_ROUTE_STQ1      (1  << 24)
#       define RADEON_TXFORMAT_ST_ROUTE_STQ2      (2  << 24)
#       define RADEON_TXFORMAT_ENDIAN_NO_SWAP     (0  << 26)
#       define RADEON_TXFORMAT_ENDIAN_16BPP_SWAP  (1  << 26)
#       define RADEON_TXFORMAT_ENDIAN_32BPP_SWAP  (2  << 26)
#       define RADEON_TXFORMAT_ENDIAN_HALFDW_SWAP (3  << 26)
#       define RADEON_TXFORMAT_ALPHA_MASK_ENABLE  (1  << 28)
#       define RADEON_TXFORMAT_CHROMA_KEY_ENABLE  (1  << 29)
#       define RADEON_TXFORMAT_CUBIC_MAP_ENABLE   (1  << 30)
#       define RADEON_TXFORMAT_PERSPECTIVE_ENABLE (1  << 31)
#define RADEON_PP_TXOFFSET_0                0x1c5c
#define RADEON_PP_TXOFFSET_1                0x1c74
#define RADEON_PP_TXOFFSET_2                0x1c8c
#       define RADEON_TXO_ENDIAN_NO_SWAP     (0 << 0)
#       define RADEON_TXO_ENDIAN_BYTE_SWAP   (1 << 0)
#       define RADEON_TXO_ENDIAN_WORD_SWAP   (2 << 0)
#       define RADEON_TXO_ENDIAN_HALFDW_SWAP (3 << 0)
#       define RADEON_TXO_MACRO_LINEAR       (0 << 2)
#       define RADEON_TXO_MACRO_TILE         (1 << 2)
#       define RADEON_TXO_MICRO_LINEAR       (0 << 3)
#       define RADEON_TXO_MICRO_TILE_X2      (1 << 3)
#       define RADEON_TXO_MICRO_TILE_OPT     (2 << 3)
#       define RADEON_TXO_OFFSET_MASK        0xffffffe0
#       define RADEON_TXO_OFFSET_SHIFT       5
#define RADEON_PP_TXCBLEND_0                0x1c60
#define RADEON_PP_TXCBLEND_1                0x1c78
#define RADEON_PP_TXCBLEND_2                0x1c90
#	define RADEON_COLOR_ARG_A_SHIFT			0
#	define RADEON_COLOR_ARG_A_MASK			(0x1f << 0)
#	define RADEON_COLOR_ARG_A_ZERO			(0 << 0)
#	define RADEON_COLOR_ARG_A_CURRENT_COLOR		(2 << 0)
#	define RADEON_COLOR_ARG_A_CURRENT_ALPHA		(3 << 0)
#	define RADEON_COLOR_ARG_A_DIFFUSE_COLOR		(4 << 0)
#	define RADEON_COLOR_ARG_A_DIFFUSE_ALPHA		(5 << 0)
#	define RADEON_COLOR_ARG_A_SPECULAR_COLOR	(6 << 0)
#	define RADEON_COLOR_ARG_A_SPECULAR_ALPHA	(7 << 0)
#	define RADEON_COLOR_ARG_A_TFACTOR_COLOR		(8 << 0)
#	define RADEON_COLOR_ARG_A_TFACTOR_ALPHA		(9 << 0)
#	define RADEON_COLOR_ARG_A_T0_COLOR		(10 << 0)
#	define RADEON_COLOR_ARG_A_T0_ALPHA		(11 << 0)
#	define RADEON_COLOR_ARG_A_T1_COLOR		(12 << 0)
#	define RADEON_COLOR_ARG_A_T1_ALPHA		(13 << 0)
#	define RADEON_COLOR_ARG_A_T2_COLOR		(14 << 0)
#	define RADEON_COLOR_ARG_A_T2_ALPHA		(15 << 0)
#	define RADEON_COLOR_ARG_A_T3_COLOR		(16 << 0)
#	define RADEON_COLOR_ARG_A_T3_ALPHA		(17 << 0)
#	define RADEON_COLOR_ARG_B_SHIFT			5
#	define RADEON_COLOR_ARG_B_MASK			(0x1f << 5)
#	define RADEON_COLOR_ARG_B_ZERO			(0 << 5)
#	define RADEON_COLOR_ARG_B_CURRENT_COLOR		(2 << 5)
#	define RADEON_COLOR_ARG_B_CURRENT_ALPHA		(3 << 5)
#	define RADEON_COLOR_ARG_B_DIFFUSE_COLOR		(4 << 5)
#	define RADEON_COLOR_ARG_B_DIFFUSE_ALPHA		(5 << 5)
#	define RADEON_COLOR_ARG_B_SPECULAR_COLOR	(6 << 5)
#	define RADEON_COLOR_ARG_B_SPECULAR_ALPHA	(7 << 5)
#	define RADEON_COLOR_ARG_B_TFACTOR_COLOR		(8 << 5)
#	define RADEON_COLOR_ARG_B_TFACTOR_ALPHA		(9 << 5)
#	define RADEON_COLOR_ARG_B_T0_COLOR		(10 << 5)
#	define RADEON_COLOR_ARG_B_T0_ALPHA		(11 << 5)
#	define RADEON_COLOR_ARG_B_T1_COLOR		(12 << 5)
#	define RADEON_COLOR_ARG_B_T1_ALPHA		(13 << 5)
#	define RADEON_COLOR_ARG_B_T2_COLOR		(14 << 5)
#	define RADEON_COLOR_ARG_B_T2_ALPHA		(15 << 5)
#	define RADEON_COLOR_ARG_B_T3_COLOR		(16 << 5)
#	define RADEON_COLOR_ARG_B_T3_ALPHA		(17 << 5)
#	define RADEON_COLOR_ARG_C_SHIFT			10
#	define RADEON_COLOR_ARG_C_MASK			(0x1f << 10)
#	define RADEON_COLOR_ARG_C_ZERO			(0 << 10)
#	define RADEON_COLOR_ARG_C_CURRENT_COLOR		(2 << 10)
#	define RADEON_COLOR_ARG_C_CURRENT_ALPHA		(3 << 10)
#	define RADEON_COLOR_ARG_C_DIFFUSE_COLOR		(4 << 10)
#	define RADEON_COLOR_ARG_C_DIFFUSE_ALPHA		(5 << 10)
#	define RADEON_COLOR_ARG_C_SPECULAR_COLOR	(6 << 10)
#	define RADEON_COLOR_ARG_C_SPECULAR_ALPHA	(7 << 10)
#	define RADEON_COLOR_ARG_C_TFACTOR_COLOR		(8 << 10)
#	define RADEON_COLOR_ARG_C_TFACTOR_ALPHA		(9 << 10)
#	define RADEON_COLOR_ARG_C_T0_COLOR		(10 << 10)
#	define RADEON_COLOR_ARG_C_T0_ALPHA		(11 << 10)
#	define RADEON_COLOR_ARG_C_T1_COLOR		(12 << 10)
#	define RADEON_COLOR_ARG_C_T1_ALPHA		(13 << 10)
#	define RADEON_COLOR_ARG_C_T2_COLOR		(14 << 10)
#	define RADEON_COLOR_ARG_C_T2_ALPHA		(15 << 10)
#	define RADEON_COLOR_ARG_C_T3_COLOR		(16 << 10)
#	define RADEON_COLOR_ARG_C_T3_ALPHA		(17 << 10)
#	define RADEON_COMP_ARG_A			(1 << 15)
#	define RADEON_COMP_ARG_A_SHIFT			15
#	define RADEON_COMP_ARG_B			(1 << 16)
#	define RADEON_COMP_ARG_B_SHIFT			16
#	define RADEON_COMP_ARG_C			(1 << 17)
#	define RADEON_COMP_ARG_C_SHIFT			17
#	define RADEON_BLEND_CTL_MASK			(7 << 18)
#	define RADEON_BLEND_CTL_ADD			(0 << 18)
#	define RADEON_BLEND_CTL_SUBTRACT		(1 << 18)
#	define RADEON_BLEND_CTL_ADDSIGNED		(2 << 18)
#	define RADEON_BLEND_CTL_BLEND			(3 << 18)
#	define RADEON_BLEND_CTL_DOT3			(4 << 18)
#	define RADEON_SCALE_SHIFT			21
#	define RADEON_SCALE_MASK			(3 << 21)
#	define RADEON_SCALE_1X				(0 << 21)
#	define RADEON_SCALE_2X				(1 << 21)
#	define RADEON_SCALE_4X				(2 << 21)
#	define RADEON_CLAMP_TX				(1 << 23)
#	define RADEON_T0_EQ_TCUR			(1 << 24)
#	define RADEON_T1_EQ_TCUR			(1 << 25)
#	define RADEON_T2_EQ_TCUR			(1 << 26)
#	define RADEON_T3_EQ_TCUR			(1 << 27)
#	define RADEON_COLOR_ARG_MASK			0x1f
#	define RADEON_COMP_ARG_SHIFT			15
#define RADEON_PP_TXABLEND_0                0x1c64
#define RADEON_PP_TXABLEND_1                0x1c7c
#define RADEON_PP_TXABLEND_2                0x1c94
#	define RADEON_ALPHA_ARG_A_SHIFT			0
#	define RADEON_ALPHA_ARG_A_MASK			(0xf << 0)
#	define RADEON_ALPHA_ARG_A_ZERO			(0 << 0)
#	define RADEON_ALPHA_ARG_A_CURRENT_ALPHA		(1 << 0)
#	define RADEON_ALPHA_ARG_A_DIFFUSE_ALPHA		(2 << 0)
#	define RADEON_ALPHA_ARG_A_SPECULAR_ALPHA	(3 << 0)
#	define RADEON_ALPHA_ARG_A_TFACTOR_ALPHA		(4 << 0)
#	define RADEON_ALPHA_ARG_A_T0_ALPHA		(5 << 0)
#	define RADEON_ALPHA_ARG_A_T1_ALPHA		(6 << 0)
#	define RADEON_ALPHA_ARG_A_T2_ALPHA		(7 << 0)
#	define RADEON_ALPHA_ARG_A_T3_ALPHA		(8 << 0)
#	define RADEON_ALPHA_ARG_B_SHIFT			4
#	define RADEON_ALPHA_ARG_B_MASK			(0xf << 4)
#	define RADEON_ALPHA_ARG_B_ZERO			(0 << 4)
#	define RADEON_ALPHA_ARG_B_CURRENT_ALPHA		(1 << 4)
#	define RADEON_ALPHA_ARG_B_DIFFUSE_ALPHA		(2 << 4)
#	define RADEON_ALPHA_ARG_B_SPECULAR_ALPHA	(3 << 4)
#	define RADEON_ALPHA_ARG_B_TFACTOR_ALPHA		(4 << 4)
#	define RADEON_ALPHA_ARG_B_T0_ALPHA		(5 << 4)
#	define RADEON_ALPHA_ARG_B_T1_ALPHA		(6 << 4)
#	define RADEON_ALPHA_ARG_B_T2_ALPHA		(7 << 4)
#	define RADEON_ALPHA_ARG_B_T3_ALPHA		(8 << 4)
#	define RADEON_ALPHA_ARG_C_SHIFT			8
#	define RADEON_ALPHA_ARG_C_MASK			(0xf << 8)
#	define RADEON_ALPHA_ARG_C_ZERO			(0 << 8)
#	define RADEON_ALPHA_ARG_C_CURRENT_ALPHA		(1 << 8)
#	define RADEON_ALPHA_ARG_C_DIFFUSE_ALPHA		(2 << 8)
#	define RADEON_ALPHA_ARG_C_SPECULAR_ALPHA	(3 << 8)
#	define RADEON_ALPHA_ARG_C_TFACTOR_ALPHA		(4 << 8)
#	define RADEON_ALPHA_ARG_C_T0_ALPHA		(5 << 8)
#	define RADEON_ALPHA_ARG_C_T1_ALPHA		(6 << 8)
#	define RADEON_ALPHA_ARG_C_T2_ALPHA		(7 << 8)
#	define RADEON_ALPHA_ARG_C_T3_ALPHA		(8 << 8)
#	define RADEON_DOT_ALPHA_DONT_REPLICATE		(1 << 9)
#	define RADEON_ALPHA_ARG_MASK			0xf

#define RADEON_PP_TFACTOR_0                 0x1c68
#define RADEON_PP_TFACTOR_1                 0x1c80
#define RADEON_PP_TFACTOR_2                 0x1c98

#define RADEON_RB3D_BLENDCNTL               0x1c20
#       define RADEON_COMB_FCN_ADD_CLAMP               (0  << 12)
#       define RADEON_COMB_FCN_ADD_NOCLAMP             (1  << 12)
#       define RADEON_COMB_FCN_SUB_CLAMP               (2  << 12)
#       define RADEON_COMB_FCN_SUB_NOCLAMP             (3  << 12)
#       define RADEON_SRC_BLEND_GL_ZERO                (32 << 16)
#       define RADEON_SRC_BLEND_GL_ONE                 (33 << 16)
#       define RADEON_SRC_BLEND_GL_SRC_COLOR           (34 << 16)
#       define RADEON_SRC_BLEND_GL_ONE_MINUS_SRC_COLOR (35 << 16)
#       define RADEON_SRC_BLEND_GL_DST_COLOR           (36 << 16)
#       define RADEON_SRC_BLEND_GL_ONE_MINUS_DST_COLOR (37 << 16)
#       define RADEON_SRC_BLEND_GL_SRC_ALPHA           (38 << 16)
#       define RADEON_SRC_BLEND_GL_ONE_MINUS_SRC_ALPHA (39 << 16)
#       define RADEON_SRC_BLEND_GL_DST_ALPHA           (40 << 16)
#       define RADEON_SRC_BLEND_GL_ONE_MINUS_DST_ALPHA (41 << 16)
#       define RADEON_SRC_BLEND_GL_SRC_ALPHA_SATURATE  (42 << 16)
#       define RADEON_SRC_BLEND_MASK                   (63 << 16)
#       define RADEON_DST_BLEND_GL_ZERO                (32 << 24)
#       define RADEON_DST_BLEND_GL_ONE                 (33 << 24)
#       define RADEON_DST_BLEND_GL_SRC_COLOR           (34 << 24)
#       define RADEON_DST_BLEND_GL_ONE_MINUS_SRC_COLOR (35 << 24)
#       define RADEON_DST_BLEND_GL_DST_COLOR           (36 << 24)
#       define RADEON_DST_BLEND_GL_ONE_MINUS_DST_COLOR (37 << 24)
#       define RADEON_DST_BLEND_GL_SRC_ALPHA           (38 << 24)
#       define RADEON_DST_BLEND_GL_ONE_MINUS_SRC_ALPHA (39 << 24)
#       define RADEON_DST_BLEND_GL_DST_ALPHA           (40 << 24)
#       define RADEON_DST_BLEND_GL_ONE_MINUS_DST_ALPHA (41 << 24)
#       define RADEON_DST_BLEND_MASK                   (63 << 24)
#define RADEON_RB3D_CNTL                    0x1c3c
#       define RADEON_ALPHA_BLEND_ENABLE       (1  <<  0)
#       define RADEON_PLANE_MASK_ENABLE        (1  <<  1)
#       define RADEON_DITHER_ENABLE            (1  <<  2)
#       define RADEON_ROUND_ENABLE             (1  <<  3)
#       define RADEON_SCALE_DITHER_ENABLE      (1  <<  4)
#       define RADEON_DITHER_INIT              (1  <<  5)
#       define RADEON_ROP_ENABLE               (1  <<  6)
#       define RADEON_STENCIL_ENABLE           (1  <<  7)
#       define RADEON_Z_ENABLE                 (1  <<  8)
#       define RADEON_DEPTH_XZ_OFFEST_ENABLE   (1  <<  9)
#       define RADEON_COLOR_FORMAT_ARGB1555    (3  << 10)
#       define RADEON_COLOR_FORMAT_RGB565      (4  << 10)
#       define RADEON_COLOR_FORMAT_ARGB8888    (6  << 10)
#       define RADEON_COLOR_FORMAT_RGB332      (7  << 10)
#       define RADEON_COLOR_FORMAT_Y8          (8  << 10)
#       define RADEON_COLOR_FORMAT_RGB8        (9  << 10)
#       define RADEON_COLOR_FORMAT_YUV422_VYUY (11 << 10)
#       define RADEON_COLOR_FORMAT_YUV422_YVYU (12 << 10)
#       define RADEON_COLOR_FORMAT_aYUV444     (14 << 10)
#       define RADEON_COLOR_FORMAT_ARGB4444    (15 << 10)
#       define RADEON_CLRCMP_FLIP_ENABLE       (1  << 14)
#       define RADEON_ZBLOCK8                  (0  << 15)
#       define RADEON_ZBLOCK16                 (1  << 15)
#define RADEON_RB3D_COLOROFFSET             0x1c40
#       define RADEON_COLOROFFSET_MASK      0xfffffff0
#define RADEON_RB3D_COLORPITCH              0x1c48
#       define RADEON_COLORPITCH_MASK         0x000001ff8
#       define RADEON_COLOR_TILE_ENABLE       (1 << 16)
#       define RADEON_COLOR_MICROTILE_ENABLE  (1 << 17)
#       define RADEON_COLOR_ENDIAN_NO_SWAP    (0 << 18)
#       define RADEON_COLOR_ENDIAN_WORD_SWAP  (1 << 18)
#       define RADEON_COLOR_ENDIAN_DWORD_SWAP (2 << 18)
#define RADEON_RB3D_DEPTHOFFSET             0x1c24
#define RADEON_RB3D_DEPTHPITCH              0x1c28
#       define RADEON_DEPTHPITCH_MASK         0x00001ff8
#       define RADEON_DEPTH_ENDIAN_NO_SWAP    (0 << 18)
#       define RADEON_DEPTH_ENDIAN_WORD_SWAP  (1 << 18)
#       define RADEON_DEPTH_ENDIAN_DWORD_SWAP (2 << 18)
#define RADEON_RB3D_PLANEMASK               0x1d84
#define RADEON_RB3D_ROPCNTL                 0x1d80
#define RADEON_RB3D_STENCILREFMASK          0x1d7c
#       define RADEON_STENCIL_REF_SHIFT       0
#       define RADEON_STENCIL_MASK_SHIFT      16
#       define RADEON_STENCIL_WRITEMASK_SHIFT 24
#define RADEON_RB3D_ZSTENCILCNTL            0x1c2c
#       define RADEON_DEPTH_FORMAT_MASK          (0xf << 0)
#       define RADEON_DEPTH_FORMAT_16BIT_INT_Z   (0  <<  0)
#       define RADEON_DEPTH_FORMAT_24BIT_INT_Z   (2  <<  0)
#       define RADEON_DEPTH_FORMAT_24BIT_FLOAT_Z (3  <<  0)
#       define RADEON_DEPTH_FORMAT_32BIT_INT_Z   (4  <<  0)
#       define RADEON_DEPTH_FORMAT_32BIT_FLOAT_Z (5  <<  0)
#       define RADEON_DEPTH_FORMAT_16BIT_FLOAT_W (7  <<  0)
#       define RADEON_DEPTH_FORMAT_24BIT_FLOAT_W (9  <<  0)
#       define RADEON_DEPTH_FORMAT_32BIT_FLOAT_W (11 <<  0)
#       define RADEON_Z_TEST_NEVER               (0  <<  4)
#       define RADEON_Z_TEST_LESS                (1  <<  4)
#       define RADEON_Z_TEST_LEQUAL              (2  <<  4)
#       define RADEON_Z_TEST_EQUAL               (3  <<  4)
#       define RADEON_Z_TEST_GEQUAL              (4  <<  4)
#       define RADEON_Z_TEST_GREATER             (5  <<  4)
#       define RADEON_Z_TEST_NEQUAL              (6  <<  4)
#       define RADEON_Z_TEST_ALWAYS              (7  <<  4)
#       define RADEON_Z_TEST_MASK                (7  <<  4)
#       define RADEON_HIERARCHICAL_Z_ENABLE      (1  <<  8)
#       define RADEON_STENCIL_TEST_NEVER         (0  << 12)
#       define RADEON_STENCIL_TEST_LESS          (1  << 12)
#       define RADEON_STENCIL_TEST_LEQUAL        (2  << 12)
#       define RADEON_STENCIL_TEST_EQUAL         (3  << 12)
#       define RADEON_STENCIL_TEST_GEQUAL        (4  << 12)
#       define RADEON_STENCIL_TEST_GREATER       (5  << 12)
#       define RADEON_STENCIL_TEST_NEQUAL        (6  << 12)
#       define RADEON_STENCIL_TEST_ALWAYS        (7  << 12)
#       define RADEON_STENCIL_S_FAIL_KEEP        (0  << 16)
#       define RADEON_STENCIL_S_FAIL_ZERO        (1  << 16)
#       define RADEON_STENCIL_S_FAIL_REPLACE     (2  << 16)
#       define RADEON_STENCIL_S_FAIL_INC         (3  << 16)
#       define RADEON_STENCIL_S_FAIL_DEC         (4  << 16)
#       define RADEON_STENCIL_S_FAIL_INVERT      (5  << 16)
#       define RADEON_STENCIL_ZPASS_KEEP         (0  << 20)
#       define RADEON_STENCIL_ZPASS_ZERO         (1  << 20)
#       define RADEON_STENCIL_ZPASS_REPLACE      (2  << 20)
#       define RADEON_STENCIL_ZPASS_INC          (3  << 20)
#       define RADEON_STENCIL_ZPASS_DEC          (4  << 20)
#       define RADEON_STENCIL_ZPASS_INVERT       (5  << 20)
#       define RADEON_STENCIL_ZFAIL_KEEP         (0  << 20)
#       define RADEON_STENCIL_ZFAIL_ZERO         (1  << 20)
#       define RADEON_STENCIL_ZFAIL_REPLACE      (2  << 20)
#       define RADEON_STENCIL_ZFAIL_INC          (3  << 20)
#       define RADEON_STENCIL_ZFAIL_DEC          (4  << 20)
#       define RADEON_STENCIL_ZFAIL_INVERT       (5  << 20)
#       define RADEON_Z_COMPRESSION_ENABLE       (1  << 28)
#       define RADEON_FORCE_Z_DIRTY              (1  << 29)
#       define RADEON_Z_WRITE_ENABLE             (1  << 30)
#       define RADEON_Z_DECOMPRESSION_ENABLE     (1  << 31)
#define RADEON_RE_LINE_PATTERN              0x1cd0
#       define RADEON_LINE_PATTERN_MASK             0x0000ffff
#       define RADEON_LINE_REPEAT_COUNT_SHIFT       16
#       define RADEON_LINE_PATTERN_START_SHIFT      24
#       define RADEON_LINE_PATTERN_LITTLE_BIT_ORDER (0 << 28)
#       define RADEON_LINE_PATTERN_BIG_BIT_ORDER    (1 << 28)
#       define RADEON_LINE_PATTERN_AUTO_RESET       (1 << 29)
#define RADEON_RE_LINE_STATE                0x1cd4
#       define RADEON_LINE_CURRENT_PTR_SHIFT   0
#       define RADEON_LINE_CURRENT_COUNT_SHIFT 8
#define RADEON_RE_MISC                      0x26c4
#       define RADEON_STIPPLE_COORD_MASK       0x1f
#       define RADEON_STIPPLE_X_OFFSET_SHIFT   0
#       define RADEON_STIPPLE_X_OFFSET_MASK    (0x1f << 0)
#       define RADEON_STIPPLE_Y_OFFSET_SHIFT   8
#       define RADEON_STIPPLE_Y_OFFSET_MASK    (0x1f << 8)
#       define RADEON_STIPPLE_LITTLE_BIT_ORDER (0 << 16)
#       define RADEON_STIPPLE_BIG_BIT_ORDER    (1 << 16)
#define RADEON_RE_SOLID_COLOR               0x1c1c
#define RADEON_RE_TOP_LEFT                  0x26c0
#       define RADEON_RE_LEFT_SHIFT         0
#       define RADEON_RE_TOP_SHIFT          16
#define RADEON_RE_WIDTH_HEIGHT              0x1c44
#       define RADEON_RE_WIDTH_SHIFT        0
#       define RADEON_RE_HEIGHT_SHIFT       16

#define RADEON_SE_CNTL                      0x1c4c
#       define RADEON_FFACE_CULL_CW          (0 <<  0)
#       define RADEON_FFACE_CULL_CCW         (1 <<  0)
#       define RADEON_FFACE_CULL_DIR_MASK    (1 <<  0)
#       define RADEON_BFACE_CULL             (0 <<  1)
#       define RADEON_BFACE_SOLID            (3 <<  1)
#       define RADEON_FFACE_CULL             (0 <<  3)
#       define RADEON_FFACE_SOLID            (3 <<  3)
#       define RADEON_FFACE_CULL_MASK        (3 <<  3)
#       define RADEON_BADVTX_CULL_DISABLE    (1 <<  5)
#       define RADEON_FLAT_SHADE_VTX_0       (0 <<  6)
#       define RADEON_FLAT_SHADE_VTX_1       (1 <<  6)
#       define RADEON_FLAT_SHADE_VTX_2       (2 <<  6)
#       define RADEON_FLAT_SHADE_VTX_LAST    (3 <<  6)
#       define RADEON_DIFFUSE_SHADE_SOLID    (0 <<  8)
#       define RADEON_DIFFUSE_SHADE_FLAT     (1 <<  8)
#       define RADEON_DIFFUSE_SHADE_GOURAUD  (2 <<  8)
#       define RADEON_DIFFUSE_SHADE_MASK     (3 <<  8)
#       define RADEON_ALPHA_SHADE_SOLID      (0 << 10)
#       define RADEON_ALPHA_SHADE_FLAT       (1 << 10)
#       define RADEON_ALPHA_SHADE_GOURAUD    (2 << 10)
#       define RADEON_ALPHA_SHADE_MASK       (3 << 10)
#       define RADEON_SPECULAR_SHADE_SOLID   (0 << 12)
#       define RADEON_SPECULAR_SHADE_FLAT    (1 << 12)
#       define RADEON_SPECULAR_SHADE_GOURAUD (2 << 12)
#       define RADEON_SPECULAR_SHADE_MASK    (3 << 12)
#       define RADEON_FOG_SHADE_SOLID        (0 << 14)
#       define RADEON_FOG_SHADE_FLAT         (1 << 14)
#       define RADEON_FOG_SHADE_GOURAUD      (2 << 14)
#       define RADEON_FOG_SHADE_MASK         (3 << 14)
#       define RADEON_ZBIAS_ENABLE_POINT     (1 << 16)
#       define RADEON_ZBIAS_ENABLE_LINE      (1 << 17)
#       define RADEON_ZBIAS_ENABLE_TRI       (1 << 18)
#       define RADEON_WIDELINE_ENABLE        (1 << 20)
#       define RADEON_VPORT_XY_XFORM_ENABLE  (1 << 24)
#       define RADEON_VPORT_Z_XFORM_ENABLE   (1 << 25)
#       define RADEON_VTX_PIX_CENTER_D3D     (0 << 27)
#       define RADEON_VTX_PIX_CENTER_OGL     (1 << 27)
#       define RADEON_ROUND_MODE_TRUNC       (0 << 28)
#       define RADEON_ROUND_MODE_ROUND       (1 << 28)
#       define RADEON_ROUND_MODE_ROUND_EVEN  (2 << 28)
#       define RADEON_ROUND_MODE_ROUND_ODD   (3 << 28)
#       define RADEON_ROUND_PREC_16TH_PIX    (0 << 30)
#       define RADEON_ROUND_PREC_8TH_PIX     (1 << 30)
#       define RADEON_ROUND_PREC_4TH_PIX     (2 << 30)
#       define RADEON_ROUND_PREC_HALF_PIX    (3 << 30)
#define RADEON_SE_CNTL_STATUS               0x2140
#       define RADEON_VC_NO_SWAP            (0 << 0)
#       define RADEON_VC_16BIT_SWAP         (1 << 0)
#       define RADEON_VC_32BIT_SWAP         (2 << 0)
#       define RADEON_VC_HALF_DWORD_SWAP    (3 << 0)
#       define RADEON_TCL_BYPASS            (1 << 8)
#define RADEON_SE_COORD_FMT                 0x15c0
#       define RADEON_VTX_XY_PRE_MULT_1_OVER_W0  (1 <<  0)
#       define RADEON_VTX_Z_PRE_MULT_1_OVER_W0   (1 <<  1)
#       define RADEON_VTX_ST0_NONPARAMETRIC      (1 <<  8)
#       define RADEON_VTX_ST1_NONPARAMETRIC      (1 <<  9)
#       define RADEON_VTX_ST2_NONPARAMETRIC      (1 << 10)
#       define RADEON_VTX_ST3_NONPARAMETRIC      (1 << 11)
#       define RADEON_VTX_W0_NORMALIZE           (1 << 12)
#       define RADEON_VTX_W0_IS_NOT_1_OVER_W0    (1 << 16)
#       define RADEON_VTX_ST0_PRE_MULT_1_OVER_W0 (1 << 17)
#       define RADEON_VTX_ST1_PRE_MULT_1_OVER_W0 (1 << 19)
#       define RADEON_VTX_ST2_PRE_MULT_1_OVER_W0 (1 << 21)
#       define RADEON_VTX_ST3_PRE_MULT_1_OVER_W0 (1 << 23)
#       define RADEON_TEX1_W_ROUTING_USE_W0      (0 << 26)
#       define RADEON_TEX1_W_ROUTING_USE_Q1      (1 << 26)
#define RADEON_SE_LINE_WIDTH                0x1db8
#define RADEON_SE_TCL_LIGHT_MODEL_CTL       0x226c
#define RADEON_SE_TCL_MATERIAL_AMBIENT_RED     0x2220
#define RADEON_SE_TCL_MATERIAL_AMBIENT_GREEN   0x2224
#define RADEON_SE_TCL_MATERIAL_AMBIENT_BLUE    0x2228
#define RADEON_SE_TCL_MATERIAL_AMBIENT_ALPHA   0x222c
#define RADEON_SE_TCL_MATERIAL_DIFFUSE_RED     0x2230
#define RADEON_SE_TCL_MATERIAL_DIFFUSE_GREEN   0x2234
#define RADEON_SE_TCL_MATERIAL_DIFFUSE_BLUE    0x2238
#define RADEON_SE_TCL_MATERIAL_DIFFUSE_ALPHA   0x223c
#define RADEON_SE_TCL_MATERIAL_EMMISSIVE_RED   0x2210
#define RADEON_SE_TCL_MATERIAL_EMMISSIVE_GREEN 0x2214
#define RADEON_SE_TCL_MATERIAL_EMMISSIVE_BLUE  0x2218
#define RADEON_SE_TCL_MATERIAL_EMMISSIVE_ALPHA 0x221c
#define RADEON_SE_TCL_MATERIAL_SPECULAR_RED    0x2240
#define RADEON_SE_TCL_MATERIAL_SPECULAR_GREEN  0x2244
#define RADEON_SE_TCL_MATERIAL_SPECULAR_BLUE   0x2248
#define RADEON_SE_TCL_MATERIAL_SPECULAR_ALPHA  0x224c
#define RADEON_SE_TCL_MATRIX_SELECT_0       0x225c
#define RADEON_SE_TCL_MATRIX_SELECT_1       0x2260
#define RADEON_SE_TCL_OUTPUT_VTX_FMT        0x2254
#define RADEON_SE_TCL_OUTPUT_VTX_SEL        0x2258
#define RADEON_SE_TCL_PER_LIGHT_CTL_0       0x2270
#define RADEON_SE_TCL_PER_LIGHT_CTL_1       0x2274
#define RADEON_SE_TCL_PER_LIGHT_CTL_2       0x2278
#define RADEON_SE_TCL_PER_LIGHT_CTL_3       0x227c
#define RADEON_SE_TCL_SHININESS             0x2250
#define RADEON_SE_TCL_TEXTURE_PROC_CTL      0x2268
#define RADEON_SE_TCL_UCP_VERT_BLEND_CTL    0x2264
#define RADEON_SE_VPORT_XSCALE              0x1d98
#define RADEON_SE_VPORT_XOFFSET             0x1d9c
#define RADEON_SE_VPORT_YSCALE              0x1da0
#define RADEON_SE_VPORT_YOFFSET             0x1da4
#define RADEON_SE_VPORT_ZSCALE              0x1da8
#define RADEON_SE_VPORT_ZOFFSET             0x1dac

#define RADEON_CP_VC_FRMT_XY                        0x00000000
#define RADEON_CP_VC_FRMT_W0                        0x00000001
#define RADEON_CP_VC_FRMT_FPCOLOR                   0x00000002
#define RADEON_CP_VC_FRMT_FPALPHA                   0x00000004
#define RADEON_CP_VC_FRMT_PKCOLOR                   0x00000008
#define RADEON_CP_VC_FRMT_FPSPEC                    0x00000010
#define RADEON_CP_VC_FRMT_FPFOG                     0x00000020
#define RADEON_CP_VC_FRMT_PKSPEC                    0x00000040
#define RADEON_CP_VC_FRMT_ST0                       0x00000080
#define RADEON_CP_VC_FRMT_ST1                       0x00000100
#define RADEON_CP_VC_FRMT_Q1                        0x00000200
#define RADEON_CP_VC_FRMT_ST2                       0x00000400
#define RADEON_CP_VC_FRMT_Q2                        0x00000800
#define RADEON_CP_VC_FRMT_ST3                       0x00001000
#define RADEON_CP_VC_FRMT_Q3                        0x00002000
#define RADEON_CP_VC_FRMT_Q0                        0x00004000
#define RADEON_CP_VC_FRMT_BLND_WEIGHT_CNT_MASK      0x00038000
#define RADEON_CP_VC_FRMT_N0                        0x00040000
#define RADEON_CP_VC_FRMT_XY1                       0x08000000
#define RADEON_CP_VC_FRMT_Z1                        0x10000000
#define RADEON_CP_VC_FRMT_W1                        0x20000000
#define RADEON_CP_VC_FRMT_N1                        0x40000000
#define RADEON_CP_VC_FRMT_Z                         0x80000000

#define RADEON_CP_VC_CNTL_PRIM_TYPE_NONE            0x00000000
#define RADEON_CP_VC_CNTL_PRIM_TYPE_POINT           0x00000001
#define RADEON_CP_VC_CNTL_PRIM_TYPE_LINE            0x00000002
#define RADEON_CP_VC_CNTL_PRIM_TYPE_LINE_STRIP      0x00000003
#define RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST        0x00000004
#define RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_FAN         0x00000005
#define RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_STRIP       0x00000006
#define RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_TYPE2       0x00000007
#define RADEON_CP_VC_CNTL_PRIM_TYPE_RECT_LIST       0x00000008
#define RADEON_CP_VC_CNTL_PRIM_TYPE_3VRT_POINT_LIST 0x00000009
#define RADEON_CP_VC_CNTL_PRIM_TYPE_3VRT_LINE_LIST  0x0000000a
#define RADEON_CP_VC_CNTL_PRIM_WALK_IND             0x00000010
#define RADEON_CP_VC_CNTL_PRIM_WALK_LIST            0x00000020
#define RADEON_CP_VC_CNTL_PRIM_WALK_RING            0x00000030
#define RADEON_CP_VC_CNTL_COLOR_ORDER_BGRA          0x00000000
#define RADEON_CP_VC_CNTL_COLOR_ORDER_RGBA          0x00000040
#define RADEON_CP_VC_CNTL_MAOS_ENABLE               0x00000080
#define RADEON_CP_VC_CNTL_VTX_FMT_NON_RADEON_MODE   0x00000000
#define RADEON_CP_VC_CNTL_VTX_FMT_RADEON_MODE       0x00000100
#define RADEON_CP_VC_CNTL_NUM_SHIFT                 16


#endif
