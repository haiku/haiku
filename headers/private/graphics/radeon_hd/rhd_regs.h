/*
 * Copyright 2007, 2008  Luc Verhaegen <libv@exsuse.de>
 * Copyright 2007, 2008  Matthias Hopf <mhopf@novell.com>
 * Copyright 2007, 2008  Egbert Eich   <eich@novell.com>
 * Copyright 2007, 2008  Advanced Micro Devices, Inc.
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
#ifndef _RHD_REGS_H
# define _RHD_REGS_H

enum {
    CLOCK_CNTL_INDEX      =       0x8,  /* (RW) */
    CLOCK_CNTL_DATA       =       0xC,  /* (RW) */
    BUS_CNTL		  =       0x4C, /* (RW) */
    MC_IND_INDEX	  =       0x70, /* (RW) */
    MC_IND_DATA           =       0x74, /* (RW) */
    RS600_MC_INDEX	  =	  0x70,
    RS600_MC_DATA	  =	  0x74,
    RS690_MC_INDEX	  =	  0x78,
    RS690_MC_DATA	  =	  0x7c,
    RS780_MC_INDEX	  =	  0x28f8,
    RS780_MC_DATA	  =	  0x28fc,

    RS60_MC_NB_MC_INDEX	  =	  0x78,
    RS60_MC_NB_MC_DATA	  =	  0x7C,
    CONFIG_CNTL		  =	  0xE0,
    PCIE_RS69_MC_INDEX	  =	  0xE8,
    PCIE_RS69_MC_DATA	  =	  0xEC,
    R5XX_CONFIG_MEMSIZE	  =	  0x00F8,

    HDP_FB_LOCATION       =	  0x0134,

    SEPROM_CNTL1	  =       0x1C0,  /* (RW) */

    AGP_BASE              =       0x0170,

    GPIOPAD_MASK          =       0x198,  /* (RW) */
    GPIOPAD_A		  =       0x19C,  /* (RW) */
    GPIOPAD_EN		  =       0x1A0,  /* (RW) */
    VIPH_CONTROL          =       0xC40,  /* (RW) */

    ROM_CNTL                       = 0x1600,
    GENERAL_PWRMGT                 = 0x0618,
    LOW_VID_LOWER_GPIO_CNTL        = 0x0724,
    MEDIUM_VID_LOWER_GPIO_CNTL     = 0x0720,
    HIGH_VID_LOWER_GPIO_CNTL       = 0x071C,
    CTXSW_VID_LOWER_GPIO_CNTL      = 0x0718,
    LOWER_GPIO_ENABLE              = 0x0710,

    /* VGA registers */
    VGA_RENDER_CONTROL             = 0x0300,
    VGA_MODE_CONTROL               = 0x0308,
    VGA_MEMORY_BASE_ADDRESS        = 0x0310,
    VGA_HDP_CONTROL                = 0x0328,
    D1VGA_CONTROL                  = 0x0330,
    D2VGA_CONTROL                  = 0x0338,

    EXT1_PPLL_REF_DIV_SRC          = 0x0400,
    EXT1_PPLL_REF_DIV              = 0x0404,
    EXT1_PPLL_UPDATE_LOCK          = 0x0408,
    EXT1_PPLL_UPDATE_CNTL          = 0x040C,
    EXT2_PPLL_REF_DIV_SRC          = 0x0410,
    EXT2_PPLL_REF_DIV              = 0x0414,
    EXT2_PPLL_UPDATE_LOCK          = 0x0418,
    EXT2_PPLL_UPDATE_CNTL          = 0x041C,

    EXT1_PPLL_FB_DIV               = 0x0430,
    EXT2_PPLL_FB_DIV               = 0x0434,
    EXT1_PPLL_POST_DIV_SRC         = 0x0438,
    EXT1_PPLL_POST_DIV             = 0x043C,
    EXT2_PPLL_POST_DIV_SRC         = 0x0440,
    EXT2_PPLL_POST_DIV             = 0x0444,
    EXT1_PPLL_CNTL                 = 0x0448,
    EXT2_PPLL_CNTL                 = 0x044C,
    P1PLL_CNTL                     = 0x0450,
    P2PLL_CNTL                     = 0x0454,
    P1PLL_INT_SS_CNTL              = 0x0458,
    P2PLL_INT_SS_CNTL              = 0x045C,

    P1PLL_DISP_CLK_CNTL            = 0x0468, /* rv620+ */
    P2PLL_DISP_CLK_CNTL            = 0x046C, /* rv620+ */
    EXT1_SYM_PPLL_POST_DIV         = 0x0470, /* rv620+ */
    EXT2_SYM_PPLL_POST_DIV         = 0x0474, /* rv620+ */

    PCLK_CRTC1_CNTL                = 0x0480,
    PCLK_CRTC2_CNTL                = 0x0484,

    /* these regs were reverse enginered,
     * so the chance is high that the naming is wrong
     * R6xx+ ??? */
    AUDIO_PLL1_MUL		   = 0x0514,
    AUDIO_PLL1_DIV		   = 0x0518,
    AUDIO_PLL2_MUL		   = 0x0524,
    AUDIO_PLL2_DIV		   = 0x0528,
    AUDIO_CLK_SRCSEL		   = 0x0534,

    DCCG_DISP_CLK_SRCSEL           = 0x0538, /* rv620+ */

    AGP_STATUS                     = 0x0F5C,

    R7XX_MC_VM_FB_LOCATION	   = 0x2024,

    R6XX_MC_VM_FB_LOCATION	   = 0x2180,
    R6XX_HDP_NONSURFACE_BASE       = 0x2C04,
    R6XX_CONFIG_MEMSIZE            = 0x5428,
    R6XX_CONFIG_FB_BASE            = 0x542C, /* AKA CONFIG_F0_BASE */
    /* PCI config space */
    PCI_CONFIG_SPACE_BASE          = 0x5000,
    PCI_CAPABILITIES_PTR           = 0x5034,

    /* CRTC1 registers */
    D1CRTC_H_TOTAL                 = 0x6000,
    D1CRTC_H_BLANK_START_END       = 0x6004,
    D1CRTC_H_SYNC_A                = 0x6008,
    D1CRTC_H_SYNC_A_CNTL           = 0x600C,
    D1CRTC_H_SYNC_B                = 0x6010,
    D1CRTC_H_SYNC_B_CNTL           = 0x6014,

    D1CRTC_V_TOTAL                 = 0x6020,
    D1CRTC_V_BLANK_START_END       = 0x6024,
    D1CRTC_V_SYNC_A                = 0x6028,
    D1CRTC_V_SYNC_A_CNTL           = 0x602C,
    D1CRTC_V_SYNC_B                = 0x6030,
    D1CRTC_V_SYNC_B_CNTL           = 0x6034,

    D1CRTC_CONTROL                 = 0x6080,
    D1CRTC_BLANK_CONTROL           = 0x6084,
    D1CRTC_INTERLACE_CONTROL	   = 0x6088,
    D1CRTC_BLACK_COLOR             = 0x6098,
    D1CRTC_STATUS                  = 0x609C,
    D1CRTC_COUNT_CONTROL           = 0x60B4,

    /* D1GRPH registers */
    D1GRPH_ENABLE                  = 0x6100,
    D1GRPH_CONTROL                 = 0x6104,
    D1GRPH_LUT_SEL                 = 0x6108,
    D1GRPH_SWAP_CNTL               = 0x610C,
    D1GRPH_PRIMARY_SURFACE_ADDRESS = 0x6110,
    D1GRPH_SECONDARY_SURFACE_ADDRESS = 0x6118,
    D1GRPH_PITCH                   = 0x6120,
    D1GRPH_SURFACE_OFFSET_X        = 0x6124,
    D1GRPH_SURFACE_OFFSET_Y        = 0x6128,
    D1GRPH_X_START                 = 0x612C,
    D1GRPH_Y_START                 = 0x6130,
    D1GRPH_X_END                   = 0x6134,
    D1GRPH_Y_END                   = 0x6138,
    D1GRPH_UPDATE                  = 0x6144,

    /* LUT */
    DC_LUT_RW_SELECT               = 0x6480,
    DC_LUT_RW_MODE                 = 0x6484,
    DC_LUT_RW_INDEX                = 0x6488,
    DC_LUT_SEQ_COLOR               = 0x648C,
    DC_LUT_PWL_DATA                = 0x6490,
    DC_LUT_30_COLOR                = 0x6494,
    DC_LUT_READ_PIPE_SELECT        = 0x6498,
    DC_LUT_WRITE_EN_MASK           = 0x649C,
    DC_LUT_AUTOFILL                = 0x64A0,

    /* LUTA */
    DC_LUTA_CONTROL                = 0x64C0,
    DC_LUTA_BLACK_OFFSET_BLUE      = 0x64C4,
    DC_LUTA_BLACK_OFFSET_GREEN     = 0x64C8,
    DC_LUTA_BLACK_OFFSET_RED       = 0x64CC,
    DC_LUTA_WHITE_OFFSET_BLUE      = 0x64D0,
    DC_LUTA_WHITE_OFFSET_GREEN     = 0x64D4,
    DC_LUTA_WHITE_OFFSET_RED       = 0x64D8,

    /* D1CUR */
    D1CUR_CONTROL                  = 0x6400,
    D1CUR_SURFACE_ADDRESS          = 0x6408,
    D1CUR_SIZE                     = 0x6410,
    D1CUR_POSITION                 = 0x6414,
    D1CUR_HOT_SPOT                 = 0x6418,
    D1CUR_UPDATE                   = 0x6424,

    /* D1MODE */
    D1MODE_DESKTOP_HEIGHT          = 0x652C,
    D1MODE_VLINE_START_END         = 0x6538,
    D1MODE_VLINE_STATUS            = 0x653C,
    D1MODE_VIEWPORT_START          = 0x6580,
    D1MODE_VIEWPORT_SIZE           = 0x6584,
    D1MODE_EXT_OVERSCAN_LEFT_RIGHT = 0x6588,
    D1MODE_EXT_OVERSCAN_TOP_BOTTOM = 0x658C,
    D1MODE_DATA_FORMAT             = 0x6528,

    /* D1SCL */
    D1SCL_ENABLE                   = 0x6590,
    D1SCL_TAP_CONTROL              = 0x6594,
    D1MODE_CENTER                  = 0x659C, /* guess */
    D1SCL_HVSCALE                  = 0x65A4, /* guess */
    D1SCL_HFILTER                  = 0x65B0, /* guess */
    D1SCL_VFILTER                  = 0x65C0, /* guess */
    D1SCL_UPDATE                   = 0x65CC,
    D1SCL_DITHER                   = 0x65D4, /* guess */
    D1SCL_FLIP_CONTROL             = 0x65D8, /* guess */

    /* CRTC2 registers */
    D2CRTC_H_TOTAL                 = 0x6800,
    D2CRTC_H_BLANK_START_END       = 0x6804,
    D2CRTC_H_SYNC_A                = 0x6808,
    D2CRTC_H_SYNC_A_CNTL           = 0x680C,
    D2CRTC_H_SYNC_B                = 0x6810,
    D2CRTC_H_SYNC_B_CNTL           = 0x6814,

    D2CRTC_V_TOTAL                 = 0x6820,
    D2CRTC_V_BLANK_START_END       = 0x6824,
    D2CRTC_V_SYNC_A                = 0x6828,
    D2CRTC_V_SYNC_A_CNTL           = 0x682C,
    D2CRTC_V_SYNC_B                = 0x6830,
    D2CRTC_V_SYNC_B_CNTL           = 0x6834,

    D2CRTC_CONTROL                 = 0x6880,
    D2CRTC_BLANK_CONTROL           = 0x6884,
    D2CRTC_BLACK_COLOR             = 0x6898,
    D2CRTC_INTERLACE_CONTROL       = 0x6888,
    D2CRTC_STATUS                  = 0x689C,
    D2CRTC_COUNT_CONTROL           = 0x68B4,

    /* D2GRPH registers */
    D2GRPH_ENABLE                  = 0x6900,
    D2GRPH_CONTROL                 = 0x6904,
    D2GRPH_LUT_SEL                 = 0x6908,
    D2GRPH_SWAP_CNTL               = 0x690C,
    D2GRPH_PRIMARY_SURFACE_ADDRESS = 0x6910,
    D2GRPH_SECONDARY_SURFACE_ADDRESS = 0x6918,
    D2GRPH_PITCH                   = 0x6920,
    D2GRPH_SURFACE_OFFSET_X        = 0x6924,
    D2GRPH_SURFACE_OFFSET_Y        = 0x6928,
    D2GRPH_X_START                 = 0x692C,
    D2GRPH_Y_START                 = 0x6930,
    D2GRPH_X_END                   = 0x6934,
    D2GRPH_Y_END                   = 0x6938,
    D2GRPH_UPDATE                  = 0x6944,

    /* LUTB */
    DC_LUTB_CONTROL                = 0x6CC0,
    DC_LUTB_BLACK_OFFSET_BLUE      = 0x6CC4,
    DC_LUTB_BLACK_OFFSET_GREEN     = 0x6CC8,
    DC_LUTB_BLACK_OFFSET_RED       = 0x6CCC,
    DC_LUTB_WHITE_OFFSET_BLUE      = 0x6CD0,
    DC_LUTB_WHITE_OFFSET_GREEN     = 0x6CD4,
    DC_LUTB_WHITE_OFFSET_RED       = 0x6CD8,

    /* D2MODE */
    D2MODE_DESKTOP_HEIGHT          = 0x6D2C,
    D2MODE_VLINE_START_END         = 0x6D38,
    D2MODE_VLINE_STATUS            = 0x6D3C,
    D2MODE_VIEWPORT_START          = 0x6D80,
    D2MODE_VIEWPORT_SIZE           = 0x6D84,
    D2MODE_EXT_OVERSCAN_LEFT_RIGHT = 0x6D88,
    D2MODE_EXT_OVERSCAN_TOP_BOTTOM = 0x6D8C,
    D2MODE_DATA_FORMAT             = 0x6D28,

    /* D2SCL */
    D2SCL_ENABLE                   = 0x6D90,
    D2SCL_TAP_CONTROL              = 0x6D94,
    D2MODE_CENTER                  = 0x6D9C, /* guess */
    D2SCL_HVSCALE                  = 0x6DA4, /* guess */
    D2SCL_HFILTER                  = 0x6DB0, /* guess */
    D2SCL_VFILTER                  = 0x6DC0, /* guess */
    D2SCL_UPDATE                   = 0x6DCC,
    D2SCL_DITHER                   = 0x6DD4, /* guess */
    D2SCL_FLIP_CONTROL             = 0x6DD8, /* guess */

    /* Audio, reverse enginered */
    AUDIO_ENABLE		   = 0x7300, /* RW */
    AUDIO_TIMING		   = 0x7344, /* RW */
    /* Audio params */
    AUDIO_VENDOR_ID                = 0x7380, /* RW */
    AUDIO_REVISION_ID              = 0x7384, /* RW */
    AUDIO_ROOT_NODE_COUNT          = 0x7388, /* RW */
    AUDIO_NID1_NODE_COUNT          = 0x738c, /* RW */
    AUDIO_NID1_TYPE                = 0x7390, /* RW */
    AUDIO_SUPPORTED_SIZE_RATE      = 0x7394, /* RW */
    AUDIO_SUPPORTED_CODEC          = 0x7398, /* RW */
    AUDIO_SUPPORTED_POWER_STATES   = 0x739c, /* RW */
    AUDIO_NID2_CAPS                = 0x73a0, /* RW */
    AUDIO_NID3_CAPS                = 0x73a4, /* RW */
    AUDIO_NID3_PIN_CAPS            = 0x73a8, /* RW */
    /* Audio conn list */
    AUDIO_CONN_LIST_LEN            = 0x73ac, /* RW */
    AUDIO_CONN_LIST                = 0x73b0, /* RW */
    /* Audio verbs */
    AUDIO_RATE_BPS_CHANNEL         = 0x73c0, /* RO */
    AUDIO_PLAYING                  = 0x73c4, /* RO */
    AUDIO_IMPLEMENTATION_ID        = 0x73c8, /* RW */
    AUDIO_CONFIG_DEFAULT           = 0x73cc, /* RW */
    AUDIO_PIN_SENSE                = 0x73d0, /* RW */
    AUDIO_PIN_WIDGET_CNTL          = 0x73d4, /* RO */
    AUDIO_STATUS_BITS              = 0x73d8, /* RO */

    R700_AUDIO_UNKNOWN             = 0x7604,

    /* HDMI */
    HDMI_TMDS			   = 0x7400,
    HDMI_LVTMA			   = 0x7700,
    HDMI_DIG			   = 0x7800,

    /* R500 DAC A */
    DACA_ENABLE                    = 0x7800,
    DACA_SOURCE_SELECT             = 0x7804,
    DACA_SYNC_TRISTATE_CONTROL     = 0x7820,
    DACA_SYNC_SELECT               = 0x7824,
    DACA_AUTODETECT_CONTROL        = 0x7828,
    DACA_AUTODETECT_INT_CONTROL    = 0x7838,
    DACA_FORCE_OUTPUT_CNTL         = 0x783C,
    DACA_FORCE_DATA                = 0x7840,
    DACA_POWERDOWN                 = 0x7850,
    DACA_CONTROL1                  = 0x7854,
    DACA_CONTROL2                  = 0x7858,
    DACA_COMPARATOR_ENABLE         = 0x785C,
    DACA_COMPARATOR_OUTPUT	   = 0x7860,

/* TMDSA */
    TMDSA_CNTL                     = 0x7880,
    TMDSA_SOURCE_SELECT            = 0x7884,
    TMDSA_COLOR_FORMAT             = 0x7888,
    TMDSA_FORCE_OUTPUT_CNTL        = 0x788C,
    TMDSA_BIT_DEPTH_CONTROL        = 0x7894,
    TMDSA_DCBALANCER_CONTROL       = 0x78D0,
    TMDSA_DATA_SYNCHRONIZATION_R500 = 0x78D8,
    TMDSA_DATA_SYNCHRONIZATION_R600 = 0x78DC,
    TMDSA_TRANSMITTER_ENABLE       = 0x7904,
    TMDSA_LOAD_DETECT              = 0x7908,
    TMDSA_MACRO_CONTROL            = 0x790C, /* r5x0 and r600: 3 for pll and 1 for TX */
    TMDSA_PLL_ADJUST               = 0x790C, /* rv6x0: pll only */
    TMDSA_TRANSMITTER_CONTROL      = 0x7910,
    TMDSA_TRANSMITTER_ADJUST       = 0x7920, /* rv6x0: TX part of macro control */

    /* DAC B */
    DACB_ENABLE                    = 0x7A00,
    DACB_SOURCE_SELECT             = 0x7A04,
    DACB_SYNC_TRISTATE_CONTROL     = 0x7A20,
    DACB_SYNC_SELECT               = 0x7A24,
    DACB_AUTODETECT_CONTROL        = 0x7A28,
    DACB_AUTODETECT_INT_CONTROL    = 0x7A38,
    DACB_FORCE_OUTPUT_CNTL         = 0x7A3C,
    DACB_FORCE_DATA                = 0x7A40,
    DACB_POWERDOWN                 = 0x7A50,
    DACB_CONTROL1                  = 0x7A54,
    DACB_CONTROL2                  = 0x7A58,
    DACB_COMPARATOR_ENABLE         = 0x7A5C,
    DACB_COMPARATOR_OUTPUT         = 0x7A60,

    /* LVTMA */
    LVTMA_CNTL                     = 0x7A80,
    LVTMA_SOURCE_SELECT            = 0x7A84,
    LVTMA_COLOR_FORMAT             = 0x7A88,
    LVTMA_FORCE_OUTPUT_CNTL        = 0x7A8C,
    LVTMA_BIT_DEPTH_CONTROL        = 0x7A94,
    LVTMA_DCBALANCER_CONTROL       = 0x7AD0,

    /* no longer shared between both r5xx and r6xx */
    LVTMA_R500_DATA_SYNCHRONIZATION = 0x7AD8,
    LVTMA_R500_PWRSEQ_REF_DIV       = 0x7AE4,
    LVTMA_R500_PWRSEQ_DELAY1        = 0x7AE8,
    LVTMA_R500_PWRSEQ_DELAY2        = 0x7AEC,
    LVTMA_R500_PWRSEQ_CNTL          = 0x7AF0,
    LVTMA_R500_PWRSEQ_STATE         = 0x7AF4,
    LVTMA_R500_BL_MOD_CNTL          = 0x7AF8,
    LVTMA_R500_LVDS_DATA_CNTL       = 0x7AFC,
    LVTMA_R500_MODE                 = 0x7B00,
    LVTMA_R500_TRANSMITTER_ENABLE   = 0x7B04,
    LVTMA_R500_MACRO_CONTROL        = 0x7B0C,
    LVTMA_R500_TRANSMITTER_CONTROL  = 0x7B10,
    LVTMA_R500_REG_TEST_OUTPUT      = 0x7B14,

    /* R600 adds an undocumented register at 0x7AD8,
     * shifting all subsequent registers by exactly one. */
    LVTMA_R600_DATA_SYNCHRONIZATION = 0x7ADC,
    LVTMA_R600_PWRSEQ_REF_DIV       = 0x7AE8,
    LVTMA_R600_PWRSEQ_DELAY1        = 0x7AEC,
    LVTMA_R600_PWRSEQ_DELAY2        = 0x7AF0,
    LVTMA_R600_PWRSEQ_CNTL          = 0x7AF4,
    LVTMA_R600_PWRSEQ_STATE         = 0x7AF8,
    LVTMA_R600_BL_MOD_CNTL          = 0x7AFC,
    LVTMA_R600_LVDS_DATA_CNTL       = 0x7B00,
    LVTMA_R600_MODE                 = 0x7B04,
    LVTMA_R600_TRANSMITTER_ENABLE   = 0x7B08,
    LVTMA_R600_MACRO_CONTROL        = 0x7B10,
    LVTMA_R600_TRANSMITTER_CONTROL  = 0x7B14,
    LVTMA_R600_REG_TEST_OUTPUT      = 0x7B18,

    LVTMA_TRANSMITTER_ADJUST        = 0x7B24, /* RV630 */
    LVTMA_PREEMPHASIS_CONTROL       = 0x7B28, /* RV630 */

    /* I2C in separate enum */

    /* HPD */
    DC_GPIO_HPD_MASK               = 0x7E90,
    DC_GPIO_HPD_A                  = 0x7E94,
    DC_GPIO_HPD_EN                 = 0x7E98,
    DC_GPIO_HPD_Y                  = 0x7E9C
};

enum DXSCL_UPDATE_bits {
    DXSCL_UPDATE_LOCK = (1 << 16)
};

enum CONFIG_CNTL_BITS {
    RS69_CFG_ATI_REV_ID_SHIFT      = 8,
    RS69_CFG_ATI_REV_ID_MASK       = 0xF << RS69_CFG_ATI_REV_ID_SHIFT
};

enum rv620Regs {
    /* DAC common */
    RV620_DAC_COMPARATOR_MISC       = 0x7da4,
    RV620_DAC_COMPARATOR_OUTPUT     = 0x7da8,

    /* RV620 DAC A */
    RV620_DACA_ENABLE              = 0x7000,
    RV620_DACA_SOURCE_SELECT       = 0x7004,
    RV620_DACA_SYNC_TRISTATE_CONTROL     = 0x7020,
    /* RV620_DACA_SYNC_SELECT         = 0x7024, ?? */
    RV620_DACA_AUTODETECT_CONTROL  = 0x7028,
    RV620_DACA_AUTODETECT_STATUS   = 0x7034,
    RV620_DACA_AUTODETECT_INT_CONTROL  = 0x7038,
    RV620_DACA_FORCE_OUTPUT_CNTL   = 0x703C,
    RV620_DACA_FORCE_DATA          = 0x7040,
    RV620_DACA_POWERDOWN           = 0x7050,
    /* RV620_DACA_CONTROL1         moved */
    RV620_DACA_CONTROL2            = 0x7058,
    RV620_DACA_COMPARATOR_ENABLE   = 0x705C,
    /* RV620_DACA_COMPARATOR_OUTPUT  changed */
    RV620_DACA_BGADJ_SRC           = 0x7ef0,
    RV620_DACA_MACRO_CNTL          = 0x7ef4,
    RV620_DACA_AUTO_CALIB_CONTROL  = 0x7ef8,

    /* DAC B */
    RV620_DACB_ENABLE              = 0x7100,
    RV620_DACB_SOURCE_SELECT       = 0x7104,
    RV620_DACB_SYNC_TRISTATE_CONTROL     = 0x7120,
    /* RV620_DACB_SYNC_SELECT         = 0x7124, ?? */
    RV620_DACB_AUTODETECT_CONTROL  = 0x7128,
    RV620_DACB_AUTODETECT_STATUS   = 0x7134,
    RV620_DACB_AUTODETECT_INT_CONTROL  = 0x7138,
    RV620_DACB_FORCE_OUTPUT_CNTL   = 0x713C,
    RV620_DACB_FORCE_DATA          = 0x7140,
    RV620_DACB_POWERDOWN           = 0x7150,
    /* RV620_DACB_CONTROL1         moved */
    RV620_DACB_CONTROL2            = 0x7158,
    RV620_DACB_COMPARATOR_ENABLE   = 0x715C,
    RV620_DACB_BGADJ_SRC           = 0x7ef0,
    RV620_DACB_MACRO_CNTL          = 0x7ff4,
    RV620_DACB_AUTO_CALIB_CONTROL  = 0x7ef8,
    /* DIG1 */
    RV620_DIG1_CNTL		   = 0x75A0,
    RV620_DIG1_CLOCK_PATTERN	   = 0x75AC,
    RV620_LVDS1_DATA_CNTL	   = 0x75BC,
    RV620_TMDS1_CNTL		   = 0x75C0,
    /* DIG2 */
    RV620_DIG2_CNTL		   = 0x79A0,
    RV620_DIG2_CLOCK_PATTERN	   = 0x79AC,
    RV620_LVDS2_DATA_CNTL	   = 0x79BC,
    RV620_TMDS2_CNTL		   = 0x79C0,

    /* RV62x I2C */
    RV62_GENERIC_I2C_CONTROL         =       0x7d80,  /* (RW) */
    RV62_GENERIC_I2C_INTERRUPT_CONTROL       =       0x7d84,  /* (RW) */
    RV62_GENERIC_I2C_STATUS  =       0x7d88,  /* (RW) */
    RV62_GENERIC_I2C_SPEED   =       0x7d8c,  /* (RW) */
    RV62_GENERIC_I2C_SETUP   =       0x7d90,  /* (RW) */
    RV62_GENERIC_I2C_TRANSACTION     =       0x7d94,  /* (RW) */
    RV62_GENERIC_I2C_DATA    =       0x7d98,  /* (RW) */
    RV62_GENERIC_I2C_PIN_SELECTION   =       0x7d9c,  /* (RW) */
    RV62_DC_GPIO_DDC4_MASK   =       0x7e20,  /* (RW) */
    RV62_DC_GPIO_DDC1_MASK   =       0x7e40,  /* (RW) */
    RV62_DC_GPIO_DDC2_MASK   =       0x7e50,  /* (RW) */
    RV62_DC_GPIO_DDC3_MASK   =       0x7e60,  /* (RW) */

    /* ?? */
    RV620_DCIO_LINK_STEER_CNTL		   = 0x7FA4,

    RV620_LVTMA_TRANSMITTER_CONTROL= 0x7F00,
    RV620_LVTMA_TRANSMITTER_ENABLE = 0x7F04,
    RV620_LVTMA_TRANSMITTER_ADJUST = 0x7F18,
    RV620_LVTMA_PREEMPHASIS_CONTROL= 0x7F1C,
    RV620_LVTMA_MACRO_CONTROL      = 0x7F0C,
    RV620_LVTMA_PWRSEQ_CNTL        = 0x7F80,
    RV620_LVTMA_PWRSEQ_STATE       = 0x7f84,
    RV620_LVTMA_PWRSEQ_REF_DIV     = 0x7f88,
    RV620_LVTMA_PWRSEQ_DELAY1      = 0x7f8C,
    RV620_LVTMA_PWRSEQ_DELAY2      = 0x7f90,
    RV620_LVTMA_BL_MOD_CNTL	   = 0x7F94,
    RV620_LVTMA_DATA_SYNCHRONIZATION = 0x7F98,
    RV620_FMT1_CONTROL		= 0x6700,
    RV620_FMT1_BIT_DEPTH_CONTROL= 0x6710,
    RV620_FMT1_CLAMP_CNTL	= 0x672C,
    RV620_FMT2_CONTROL		= 0x6F00,
    RV620_FMT2_CNTL		= 0x6F10,
    RV620_FMT2_CLAMP_CNTL	= 0x6F2C,

    RV620_EXT1_DIFF_POST_DIV_CNTL= 0x0420,
    RV620_EXT2_DIFF_POST_DIV_CNTL= 0x0424,
    RV620_DCCG_PCLK_DIGA_CNTL   = 0x04b0,
    RV620_DCCG_PCLK_DIGB_CNTL   = 0x04b4,
    RV620_DCCG_SYMCLK_CNTL	= 0x04b8
};

enum RV620_EXT1_DIFF_POST_DIV_CNTL_BITS {
    RV62_EXT1_DIFF_POST_DIV_RESET  = 1 << 0,
    RV62_EXT1_DIFF_POST_DIV_SELECT = 1 << 4,
    RV62_EXT1_DIFF_DRIVER_ENABLE   = 1 << 8
};

enum RV620_EXT2_DIFF_POST_DIV_CNTL_BITS {
    RV62_EXT2_DIFF_POST_DIV_RESET = 1 << 0,
    RV62_EXT2_DIFF_POST_DIV_SELECT = 1 << 4,
    RV62_EXT2_DIFF_DRIVER_ENABLE = 3 << 8
};

enum RV620_LVTMA_PWRSEQ_CNTL_BITS {
    RV62_LVTMA_PWRSEQ_EN = 1 << 0,
    RV62_LVTMA_PWRSEQ_DISABLE_SYNCEN_CONTROL_OF_TX_EN = 1 << 1,
    RV62_LVTMA_PLL_ENABLE_PWRSEQ_MASK = 1 << 2,
    RV62_LVTMA_PLL_RESET_PWRSEQ_MASK = 1 << 3,
    RV62_LVTMA_PWRSEQ_TARGET_STATE = 1 << 4,
    RV62_LVTMA_SYNCEN = 1 << 8,
    RV62_LVTMA_SYNCEN_OVRD = 1 << 9,
    RV62_LVTMA_SYNCEN_POL = 1 << 10,
    RV62_LVTMA_DIGON = 1 << 16,
    RV62_LVTMA_DIGON_OVRD = 1 << 17,
    RV62_LVTMA_DIGON_POL = 1 << 18,
    RV62_LVTMA_BLON = 1 << 24,
    RV62_LVTMA_BLON_OVRD = 1 << 25,
    RV62_LVTMA_BLON_POL = 1 << 26
};

enum RV620_LVTMA_PWRSEQ_STATE_BITS {
    RV62_LVTMA_PWRSEQ_STATE_SHIFT = 8
};

enum RV620_LVTMA_PWRSEQ_STATE_VAL {
    RV62_POWERUP_DONE = 4,
    RV62_POWERDOWN_DONE = 9
};

enum RV620_LVTMA_TRANSMITTER_CONTROL_BITS {
    RV62_LVTMA_PLL_ENABLE  = 1 << 0,
    RV62_LVTMA_PLL_RESET   = 1 << 1,
    RV62_LVTMA_IDSCKSEL    = 1 << 4,
    RV62_LVTMA_BGSLEEP     = 1 << 5,
    RV62_LVTMA_IDCLK_SEL   = 1 << 6,
    RV62_LVTMA_TMCLK       = 1 << 8,
    RV62_LVTMA_TMCLK_FROM_PADS = 1 << 13,
    RV62_LVTMA_TDCLK           = 1 << 14,
    RV62_LVTMA_TDCLK_FROM_PADS = 1 << 15,
    RV62_LVTMA_BYPASS_PLL      = 1 << 28,
    RV62_LVTMA_USE_CLK_DATA    = 1 << 29,
    RV62_LVTMA_MODE            = 1 << 30,
    RV62_LVTMA_INPUT_TEST_CLK_SEL = 1 << 31
};

enum RV620_DCCG_SYMCLK_CNTL {
    RV62_SYMCLKA_SRC_SHIFT = 8,
    RV62_SYMCLKB_SRC_SHIFT = 12
};

enum RV620_DCCG_DIG_CNTL {
    RV62_PCLK_DIGA_ON = 0x1
};

enum RV620_DCIO_LINK_STEER_CNTL {
    RV62_LINK_STEER_SWAP = 1 << 0,
    RV62_LINK_STEER_PLLSEL_OVERWRITE_EN = 1 << 16,
    RV62_LINK_STEER_PLLSELA = 1 << 17,
    RV62_LINK_STEER_PLLSELB = 1 << 18
};

enum R620_LVTMA_TRANSMITTER_ENABLE_BITS {
    RV62_LVTMA_LNK0EN = 1 << 0,
    RV62_LVTMA_LNK1EN = 1 << 1,
    RV62_LVTMA_LNK2EN = 1 << 2,
    RV62_LVTMA_LNK3EN = 1 << 3,
    RV62_LVTMA_LNK4EN = 1 << 4,
    RV62_LVTMA_LNK5EN = 1 << 5,
    RV62_LVTMA_LNK6EN = 1 << 6,
    RV62_LVTMA_LNK7EN = 1 << 7,
    RV62_LVTMA_LNK8EN = 1 << 8,
    RV62_LVTMA_LNK9EN = 1 << 9,
    RV62_LVTMA_LNKL = RV62_LVTMA_LNK0EN | RV62_LVTMA_LNK1EN
    | RV62_LVTMA_LNK2EN | RV62_LVTMA_LNK3EN,
    RV62_LVTMA_LNKU = RV62_LVTMA_LNK4EN | RV62_LVTMA_LNK5EN
    | RV62_LVTMA_LNK6EN | RV62_LVTMA_LNK7EN,
    RV62_LVTMA_LNK_ALL =  RV62_LVTMA_LNKL | RV62_LVTMA_LNKU
    | RV62_LVTMA_LNK8EN | RV62_LVTMA_LNK9EN,
    RV62_LVTMA_LNKEN_HPD_MASK = 1 << 16
};

enum RV620_LVTMA_DATA_SYNCHRONIZATION {
    RV62_LVTMA_DSYNSEL  = (1 << 0),
    RV62_LVTMA_PFREQCHG = (1 << 8)
};

enum RV620_LVTMA_PWRSEQ_REF_DIV_BITS {
    LVTMA_PWRSEQ_REF_DI_SHIFT  = 0,
    LVTMA_BL_MOD_REF_DI_SHIFT  = 16
};

enum RV620_LVTMA_BL_MOD_CNTL_BITS {
    LVTMA_BL_MOD_EN          = 1 << 0,
    LVTMA_BL_MOD_LEVEL_SHIFT = 8,
    LVTMA_BL_MOD_RES_SHIFT   = 16
};

enum RV620_DIG_CNTL_BITS {
    /* 0x75A0 */
    RV62_DIG_SWAP		   = (0x1 << 16),
    RV62_DIG_DUAL_LINK_ENABLE     = (0x1 << 12),
    RV62_DIG_START			   = (0x1 << 6),
    RV62_DIG_MODE			= (0x7 << 8),
    RV62_DIG_STEREOSYNC_SELECT   = (1 << 2),
    RV62_DIG_SOURCE_SELECT       = (1 << 0),
    RV62_DIG_SOURCE_SELECT_FMT1  = (0 << 0),
    RV62_DIG_SOURCE_SELECT_FMT2  = (1 << 0)
};

enum RV620_DIG_LVDS_DATA_CNTL_BITS {
    /* 0x75BC */
    RV62_LVDS_24BIT_ENABLE	   = (0x1 << 0),
    RV62_LVDS_24BIT_FORMAT	   = (0x1 << 4)
};

enum RV620_TMDS_CNTL_BITS {
    /* 0x75C0 */
    RV62_TMDS_PIXEL_ENCODING      = (0x1 << 4),
    RV62_TMDS_COLOR_FORMAT        = (0x3 << 8)
};

enum RV620_FMT_BIT_DEPTH_CONTROL {
    RV62_FMT_TRUNCATE_EN = 1 << 0,
    RV62_FMT_TRUNCATE_DEPTH = 1 << 4,
    RV62_FMT_SPATIAL_DITHER_EN = 1 << 8,
    RV62_FMT_SPATIAL_DITHER_MODE = 1 << 9,
    RV62_FMT_SPATIAL_DITHER_DEPTH = 1 << 12,
    RV62_FMT_FRAME_RANDOM_ENABLE = 1 << 13,
    RV62_FMT_RGB_RANDOM_ENABLE = 1 << 14,
    RV62_FMT_HIGHPASS_RANDOM_ENABLE = 1 << 15,
    RV62_FMT_TEMPORAL_DITHER_EN = 1 << 16,
    RV62_FMT_TEMPORAL_DITHER_DEPTH = 1 << 20,
    RV62_FMT_TEMPORAL_DITHER_OFFSET = 3 << 21,
    RV62_FMT_TEMPORAL_LEVEL = 1 << 24,
    RV62_FMT_TEMPORAL_DITHER_RESET = 1 << 25,
    RV62_FMT_25FRC_SEL = 3 << 26,
    RV62_FMT_50FRC_SEL = 3 << 28,
    RV62_FMT_75FRC_SEL = 3 << 30
};

enum RV620_FMT_CONTROL {
    RV62_FMT_PIXEL_ENCODING = 1 << 16
};

enum _r5xxMCRegs {
    R5XX_MC_STATUS                 = 0x0000,
    RV515_MC_FB_LOCATION	   = 0x0001,
    R5XX_MC_FB_LOCATION		   = 0x0004,
    RV515_MC_STATUS                = 0x0008,
    RV515_MC_MISC_LAT_TIMER        = 0x0009
};

enum _r5xxRegs {
    /* I2C */
    R5_DC_I2C_STATUS1 	=	0x7D30,  /* (RW) */
    R5_DC_I2C_RESET 	=	0x7D34,  /* (RW) */
    R5_DC_I2C_CONTROL1 	=	0x7D38,  /* (RW) */
    R5_DC_I2C_CONTROL2 	=	0x7D3C,  /* (RW) */
    R5_DC_I2C_CONTROL3 	=	0x7D40,  /* (RW) */
    R5_DC_I2C_DATA 	=	0x7D44,  /* (RW) */
    R5_DC_I2C_INTERRUPT_CONTROL 	=	0x7D48,  /* (RW) */
    R5_DC_I2C_ARBITRATION 	=	0x7D50,  /* (RW) */

    R5_DC_GPIO_DDC1_MASK              = 0x7E40,  /* (RW) */
    R5_DC_GPIO_DDC1_A                 = 0x7E44,  /* (RW) */
    R5_DC_GPIO_DDC1_EN                = 0x7E48,  /* (RW) */
    R5_DC_GPIO_DDC2_MASK              = 0x7E50,  /* (RW) */
    R5_DC_GPIO_DDC2_A                 = 0x7E54,  /* (RW) */
    R5_DC_GPIO_DDC2_EN                = 0x7E58,  /* (RW) */
    R5_DC_GPIO_DDC3_MASK              = 0x7E60,  /* (RW) */
    R5_DC_GPIO_DDC3_A                 = 0x7E64,  /* (RW) */
    R5_DC_GPIO_DDC3_EN                = 0x7E68  /* (RW) */
};

enum _r5xxSPLLRegs {
    SPLL_FUNC_CNTL      =       0x0  /* (RW) */
};

enum _r6xxRegs {
    /* MCLK */
    R6_MCLK_PWRMGT_CNTL		   = 0x620,
    /* I2C */
    R6_DC_I2C_CONTROL		   = 0x7D30,  /* (RW) */
    R6_DC_I2C_ARBITRATION		   = 0x7D34,  /* (RW) */
    R6_DC_I2C_INTERRUPT_CONTROL	   = 0x7D38,  /* (RW) */
    R6_DC_I2C_SW_STATUS	           = 0x7d3c,  /* (RW) */
    R6_DC_I2C_DDC1_SPEED              = 0x7D4C,  /* (RW) */
    R6_DC_I2C_DDC1_SETUP              = 0x7D50,  /* (RW) */
    R6_DC_I2C_DDC2_SPEED              = 0x7D54,  /* (RW) */
    R6_DC_I2C_DDC2_SETUP              = 0x7D58,  /* (RW) */
    R6_DC_I2C_DDC3_SPEED              = 0x7D5C,  /* (RW) */
    R6_DC_I2C_DDC3_SETUP              = 0x7D60,  /* (RW) */
    R6_DC_I2C_TRANSACTION0            = 0x7D64,  /* (RW) */
    R6_DC_I2C_TRANSACTION1            = 0x7D68,  /* (RW) */
    R6_DC_I2C_DATA			   = 0x7D74,  /* (RW) */
    R6_DC_I2C_DDC4_SPEED              = 0x7DB4,  /* (RW) */
    R6_DC_I2C_DDC4_SETUP              = 0x7DBC,  /* (RW) */
    R6_DC_GPIO_DDC4_MASK              = 0x7E00,  /* (RW) */
    R6_DC_GPIO_DDC4_A                 = 0x7E04,  /* (RW) */
    R6_DC_GPIO_DDC4_EN                = 0x7E08,  /* (RW) */
    R6_DC_GPIO_DDC1_MASK              = 0x7E40,  /* (RW) */
    R6_DC_GPIO_DDC1_A                 = 0x7E44,  /* (RW) */
    R6_DC_GPIO_DDC1_EN                = 0x7E48,  /* (RW) */
    R6_DC_GPIO_DDC1_Y                 = 0x7E4C,  /* (RW) */
    R6_DC_GPIO_DDC2_MASK              = 0x7E50,  /* (RW) */
    R6_DC_GPIO_DDC2_A                 = 0x7E54,  /* (RW) */
    R6_DC_GPIO_DDC2_EN                = 0x7E58,  /* (RW) */
    R6_DC_GPIO_DDC2_Y                 = 0x7E5C,  /* (RW) */
    R6_DC_GPIO_DDC3_MASK              = 0x7E60,  /* (RW) */
    R6_DC_GPIO_DDC3_A                 = 0x7E64,  /* (RW) */
    R6_DC_GPIO_DDC3_EN                = 0x7E68,  /* (RW) */
    R6_DC_GPIO_DDC3_Y                 = 0x7E6C  /* (RW) */
};

enum R6_MCLK_PWRMGT_CNTL {
    R6_MC_BUSY = (1 << 5)
};


/* *_Q: questionbable */
enum _rs69xRegs {
    /* I2C */
    RS69_DC_I2C_CONTROL		   = 0x7D30,  /* (RW) *//* */
    RS69_DC_I2C_UNKNOWN_2		   = 0x7D34,  /* (RW) */
    RS69_DC_I2C_INTERRUPT_CONTROL	   = 0x7D38,  /* (RW) */
    RS69_DC_I2C_SW_STATUS	           = 0x7d3c,  /* (RW) *//**/
    RS69_DC_I2C_UNKNOWN_1                = 0x7d40,
    RS69_DC_I2C_DDC_SETUP_Q              = 0x7D44,  /* (RW) */
    RS69_DC_I2C_DATA			   = 0x7D58,  /* (RW) *//**/
    RS69_DC_I2C_TRANSACTION0            = 0x7D48,  /* (RW) *//**/
    RS69_DC_I2C_TRANSACTION1            = 0x7D4C,  /* (RW) *//**/
    /* DDIA */
    RS69_DDIA_CNTL			= 0x7200,
    RS69_DDIA_SOURCE_SELECT             = 0x7204,
    RS69_DDIA_BIT_DEPTH_CONTROL		= 0x7214,
    RS69_DDIA_DCBALANCER_CONTROL	= 0x7250,
    RS69_DDIA_PATH_CONTROL		= 0x7264,
    RS69_DDIA_PCIE_LINK_CONTROL2	= 0x7278,
    RS69_DDIA_PCIE_LINK_CONTROL3	= 0x727c,
    RS69_DDIA_PCIE_PHY_CONTROL1		= 0x728c,
    RS69_DDIA_PCIE_PHY_CONTROL2		= 0x7290
};

enum RS69_DDIA_CNTL_BITS {
    RS69_DDIA_ENABLE			= 1 << 0,
    RS69_DDIA_HDMI_EN			= 1 << 2,
    RS69_DDIA_ENABLE_HPD_MASK		= 1 << 4,
    RS69_DDIA_HPD_SELECT		= 1 << 8,
    RS69_DDIA_SYNC_PHASE		= 1 << 12,
    RS69_DDIA_PIXEL_ENCODING		= 1 << 16,
    RS69_DDIA_DUAL_LINK_ENABLE		= 1 << 24,
    RS69_DDIA_SWAP			= 1 << 28
};

enum RS69_DDIA_SOURCE_SELECT_BITS {
    RS69_DDIA_SOURCE_SELECT_BIT        = 1 << 0,
    RS69_DDIA_SYNC_SELECT              = 1 << 8,
    RS69_DDIA_STEREOSYNC_SELECT        = 1 << 16
};

enum RS69_DDIA_LINK_CONTROL2_SHIFT {
    RS69_DDIA_PCIE_OUTPUT_MUX_SEL0	= 0,
    RS69_DDIA_PCIE_OUTPUT_MUX_SEL1	= 4,
    RS69_DDIA_PCIE_OUTPUT_MUX_SEL2	= 8,
    RS69_DDIA_PCIE_OUTPUT_MUX_SEL3	= 12
};

enum RS69_DDIA_BIT_DEPTH_CONTROL_BITS {
    RS69_DDIA_TRUNCATE_EN		= 1 << 0,
    RS69_DDIA_TRUNCATE_DEPTH 		= 1 << 4,
    RS69_DDIA_SPATIAL_DITHER_EN		= 1 << 8,
    RS69_DDIA_SPATIAL_DITHER_DEPTH	= 1 << 12,
    RS69_DDIA_TEMPORAL_DITHER_EN	= 1 << 16,
    RS69_DDIA_TEMPORAL_DITHER_DEPTH	= 1 << 20,
    RS69_DDIA_TEMPORAL_LEVEL		= 1 << 24,
    RS69_DDIA_TEMPORAL_DITHER_RESET	= 1 << 25
};

enum RS69_DDIA_DCBALANCER_CONTROL_BITS {
    RS69_DDIA_DCBALANCER_EN		= 1 << 0,
    RS69_DDIA_SYNC_DCBAL_EN_SHIFT       = 4,
    RS69_DDIA_SYNC_DCBAL_EN_MASK        = 7 <<  RS69_DDIA_SYNC_DCBAL_EN_SHIFT,
    RS69_DDIA_DCBALANCER_TEST_EN	= 1 << 8,
    RS69_DDIA_DCBALANCER_TEST_IN_SHIFT  = 16,
    RS69_DDIA_DCBALANCER_FORCE		= 1 << 24
};

enum RS69_DDIA_PATH_CONTROL_BITS {
    RS69_DDIA_PATH_SELECT_SHIFT		= 0,
    RS69_DDIA_DDPII_DE_ALIGN_EN		= 1 <<  4,
    RS69_DDIA_DDPII_TRAIN_EN		= 1 <<  8,
    RS69_DDIA_DDPII_TRAIN_SELECT	= 1 << 12,
    RS69_DDIA_DDPII_SCRAMBLE_EN		= 1 << 16,
    RS69_DDIA_REPL_MODE_SELECT		= 1 << 20,
    RS69_DDIA_RB_30b_SWAP_EN		= 1 << 24,
    RS69_DDIA_PIXVLD_RESET		= 1 << 28,
    RS69_DDIA_REARRANGER_EN		= 1 << 30
};

enum RS69_DDIA_PCIE_LINK_CONTROL3_BITS {
    RS69_DDIA_PCIE_MIRROR_EN		= 1 << 0,
    RS69_DDIA_PCIE_CFGDUALLINK		= 1 << 4,
    RS69_DDIA_PCIE_NCHG3EN		= 1 << 8,
    RS69_DDIA_PCIE_RX_PDNB_SHIFT	= 12
};

enum RS69_MC_INDEX_BITS {
    PCIE_RS69_MC_IND_ADDR = (0x1 << 0),
    PCIE_RS69_MC_IND_WR_EN = (0x1 << 9)
};

enum RS60_MC_NB_MC_INDEX_BITS {
    RS60_NB_MC_IND_ADDR = (0x1 << 0),
    RS60_NB_MC_IND_WR_EN = (0x1 << 8)
};

enum _rs690MCRegs {
    RS69_K8_FB_LOCATION		=	0x1E,
    RS69_MC_MISC_UMA_CNTL	=	0x5f,
    RS69_MC_SYSTEM_STATUS 	=	0x90,  /* (RW) */
    RS69_MCCFG_FB_LOCATION		=	0x100,
    RS69MCCFG_AGP_LOCATION		=	0x101,
    RS69_MC_INIT_MISC_LAT_TIMER         =       0x104
};

enum MC_MISC_LAT_TIMER_BITS {
    MC_CPR_INIT_LAT_SHIFT    =  0,
    MC_VF_INIT_LAT           =  4,
    MC_DISP0R_INIT_LAT_SHIFT =  8,
    MC_DISP1R_INIT_LAT_SHIFT = 12,
    MC_FIXED_INIT_LAT_SHIFT  = 16,
    MC_E2R_INIT_LAT_SHIFT    = 20,
    SAME_PAGE_PRIO_SHIFT     = 24,
    MC_GLOBW_INIT_LAT_SHIFT  = 28
};

enum RS69_MC_MISC_UMA_CNTL_BITS {
    RS69_K8_40BIT_ADDR_EXTENSION = (0x1 << 0),
    RS69_GART_BYPASS             = (0x1 << 8),
    RS69_GFX_64BYTE_MODE         = (0x1 << 9),
    RS69_GFX_64BYTE_LAT          = (0x1 << 10),
    RS69_GTW_COHERENCY           = (0x1 << 15),
    RS69_READ_BUFFER_SIZE        = (0x1 << 16),
    RS69_HDR_ROUTE_TO_DSP        = (0x1 << 24),
    RS69_GTW_ROUTE_TO_DSP        = (0x1 << 25),
    RS69_DSP_ROUTE_TO_GFX        = (0x1 << 26),
    RS69_USE_HDPW_LAT_INIT       = (0x1 << 27),
    RS69_USE_GFXW_LAT_INIT       = (0x1 << 28),
    RS69_MCIFR_COHERENT          = (0x1 << 29),
    RS69_NON_SNOOP_AZR_AIC_BP    = (0x1 << 30),
    RS69_SIDE_PORT_PRESENT_R     = (0x1 << 31)
};

enum _rs600MCRegs {
    RS60_MC_SYSTEM_STATUS	=       0x0,
    RS60_NB_FB_LOCATION		=	0xa
};

enum RS600_MC_INDEX_BITS {
    RS600_MC_INDEX_ADDR_MASK	 = 0xffff,
    RS600_MC_INDEX_SEQ_RBS_0	 = (1 << 16),
    RS600_MC_INDEX_SEQ_RBS_1	 = (1 << 17),
    RS600_MC_INDEX_SEQ_RBS_2	 = (1 << 18),
    RS600_MC_INDEX_SEQ_RBS_3	 = (1 << 19),
    RS600_MC_INDEX_AIC_RBS	 = (1 << 20),
    RS600_MC_INDEX_CITF_ARB0	 = (1 << 21),
    RS600_MC_INDEX_CITF_ARB1	 = (1 << 22),
    RS600_MC_INDEX_WR_EN	 = (1 << 23)
};

enum RS690_MC_INDEX_BITS {
    RS690_MC_INDEX_ADDR_MASK	 = 0x1ff,
    RS690_MC_INDEX_WR_EN	 = (1 << 9),
    RS690_MC_INDEX_WR_ACK	 = 0x7f
};

enum RS780_MC_INDEX_BITS {
    RS780_MC_INDEX_ADDR_MASK	 = 0x1ff,
    RS780_MC_INDEX_WR_EN	 = (1 << 9)
};

enum _rs780NBRegs {
    PCIE_RS78_NB_MC_IND_INDEX	= 0x70,
    PCIE_RS78_NB_MC_IND_DATA         = 0x74
};

enum RS78_NB_IND_INDEX_BITS {
    PCIE_RS78_NB_MC_IND_INDEX_MASK = (0xffff << 0),
    PCIE_RS78_MC_IND_SEQ_RBS_0     = (0x1 << 16),
    PCIE_RS78_MC_IND_SEQ_RBS_1     = (0x1 << 17),
    PCIE_RS78_MC_IND_SEQ_RBS_2     = (0x1 << 18),
    PCIE_RS78_MC_IND_SEQ_RBS_3     = (0x1 << 19),
    PCIE_RS78_MC_IND_AIC_RBS       = (0x1 << 20),
    PCIE_RS78_MC_IND_CITF_ARB0     = (0x1 << 21),
    PCIE_RS78_MC_IND_CITF_ARB1     = (0x1 << 22),
    PCIE_RS78_MC_IND_WR_EN         = (0x1 << 23),
    PCIE_RS78_MC_IND_RD_INV        = (0x1 << 24)
};

enum _rs780MCRegs {
    RS78_MC_SYSTEM_STATUS	=	0x0,
    RS78_MC_FB_LOCATION		=	0x10,
    RS78_K8_FB_LOCATION		=	0x11,
    RS78_MC_MISC_UMA_CNTL       =       0x12
};

enum RS6X_MC_SYSTEM_STATUS_BITS {
        RS6X_MC_SYSTEM_IDLE	 = (0x1 << 0),
	RS6X_MC_SEQUENCER_IDLE	 = (0x1 << 1),
	RS6X_MC_ARBITER_IDLE	 = (0x1 << 2),
	RS6X_MC_SELECT_PM	 = (0x1 << 3),
	RS6X_RESERVED4	 = (0xf << 4),
	RS6X_RESERVED8	 = (0xf << 8),
	RS6X_RESERVED12_SYSTEM_STATUS	 = (0xf << 12),
	RS6X_MCA_INIT_EXECUTED	 = (0x1 << 16),
	RS6X_MCA_IDLE	 = (0x1 << 17),
	RS6X_MCA_SEQ_IDLE	 = (0x1 << 18),
	RS6X_MCA_ARB_IDLE	 = (0x1 << 19),
	RS6X_RESERVED20_SYSTEM_STATUS	 = (0xfff << 20)
};

enum RS78_MC_MISC_UMA_CNTL_BITS {
    RS78_K8_40BIT_ADDR_EXTENSION = ( 0x1 << 0),
    RS78_BANKGROUP_SEL           = ( 0x1 << 8),
    RS78_CNTL_SPARE              = ( 0x1 << 15),
    RS78_SIDE_PORT_PRESENT_R     = ( 0x1 << 31)
};

enum R5XX_MC_STATUS_BITS {
    R5XX_MEM_PWRUP_COMPL = (0x1 << 0),
    R5XX_MC_IDLE	    = (0x1 << 1)
};

enum RV515_MC_STATUS_BITS {
    RV515_MC_IDLE        = (0x1 << 4)
};

enum RS78_MC_SYSTEM_STATUS_BITS {
    RS78_MC_SYSTEM_IDLE        =  1 << 0,
    RS78_MC_SEQUENCER_IDLE     =  1 << 1,
    RS78_MC_ARBITER_IDLE       = 1 << 2,
    RS78_MC_SELECT_PM          = 1 << 3,
    RS78_MC_STATUS_15_4_SHIFT  = 4,
    RS78_MCA_INIT_EXECUTED     = 1 << 16,
    RS78_MCA_IDLE              = 1 << 17,
    RS78_MCA_SEQ_IDLE          = 1 << 18,
    RS78_MCA_ARB_IDLE          = 1 << 19,
    RS78_MC_STATUS_31_20_SHIFT = 20
};

enum BUS_CNTL_BITS {
    /* BUS_CNTL */
    BUS_DBL_RESYNC       = (0x1 << 0),
    BIOS_ROM_WRT_EN      = (0x1 << 1),
    BIOS_ROM_DIS         = (0x1 << 2),
    PMI_IO_DIS   = (0x1 << 3),
    PMI_MEM_DIS  = (0x1 << 4),
    PMI_BM_DIS   = (0x1 << 5),
    PMI_INT_DIS  = (0x1 << 6)
};

enum SEPROM_SNTL1_BITS {
    /* SEPROM_CNTL1 */
    WRITE_ENABLE         = (0x1 << 0),
    WRITE_DISABLE        = (0x1 << 1),
    READ_CONFIG  = (0x1 << 2),
    WRITE_CONFIG         = (0x1 << 3),
    READ_STATUS  = (0x1 << 4),
    SECT_TO_SRAM         = (0x1 << 5),
    READY_BUSY   = (0x1 << 7),
    SEPROM_BUSY  = (0x1 << 8),
    BCNT_OVER_WTE_EN     = (0x1 << 9),
    RB_MASKB     = (0x1 << 10),
    SOFT_RESET   = (0x1 << 11),
    STATE_IDLEb  = (0x1 << 12),
    SECTOR_ERASE         = (0x1 << 13),
    BYTE_CNT     = (0xff << 16),
    SCK_PRESCALE         = (0xff << 24)
};

enum VIPH_CONTROL_BITS {
    /* VIPH_CONTROL */
    VIPH_CLK_SEL         = (0xff << 0),
    VIPH_REG_RDY         = (0x1 << 13),
    VIPH_MAX_WAIT        = (0xf << 16),
    VIPH_DMA_MODE        = (0x1 << 20),
    VIPH_EN      = (0x1 << 21),
    VIPH_DV0_WID         = (0x1 << 24),
    VIPH_DV1_WID         = (0x1 << 25),
    VIPH_DV2_WID         = (0x1 << 26),
    VIPH_DV3_WID         = (0x1 << 27),
    VIPH_PWR_DOWN        = (0x1 << 28),
    VIPH_PWR_DOWN_AK     = (0x1 << 28),
    VIPH_VIPCLK_DIS      = (0x1 << 29)
};

enum ROM_CNTL_BITS {
    SCK_OVERWRITE            = 1 << 1,
    CLOCK_GATING_EN          = 1 << 2,
    CSB_ACTIVE_TO_SCK_SETUP_TIME_SHIFT = 8,
    CSB_ACTIVE_TO_SCK_HOLD_TIME_SHIFT = 16,
    SCK_PRESCALE_REFCLK_SHIFT      = 24,
    SCK_PRESCALE_CRYSTAL_CLK_SHIFT = 28
};

enum GENERAL_PWRMGT_BITS {
    GLOBAL_PWRMGT_EN           = 1 << 0,
    STATIC_PM_EN               = 1 << 1,
    MOBILE_SU                  = 1 << 2,
    THERMAL_PROTECTION_DIS     = 1 << 3,
    THERMAL_PROTECTION_TYPE    = 1 << 4,
    ENABLE_GEN2PCIE            = 1 << 5,
    SW_GPIO_INDEX_SHIFT        = 1 << 6,
    LOW_VOLT_D2_ACPI           = 1 << 8,
    LOW_VOLT_D3_ACPI           = 1 << 9,
    VOLT_PWRMGT_EN             = 1 << 10,
    OPEN_DRAIN_PADS            = 1 << 11,
    AVP_SCLK_EN                = 1 << 12,
    IDCT_SCLK_EN               = 1 << 13,
    GPU_COUNTER_ACPI           = 1 << 14,
    GPU_COUNTER_CLK            = 1 << 15,
    BACKBIAS_PAD_EN            = 1 << 16,
    BACKBIAS_VALUE             = 1 << 17,
    BACKBIAS_DPM_CNTL          = 1 << 18,
    SPREAD_SPECTRUM_INDEX_SHIFT = 19,
    DYN_SPREAD_SPECTRUM_EN     = 1 << 2
};

enum VGA_RENDER_CONTROL_BITS {
    /* VGA_RENDER_CONTROL */
    VGA_BLINK_RATE		= (0x1f << 0),
    VGA_BLINK_MODE		= (0x3 << 5),
    VGA_CURSOR_BLINK_INVERT      = (0x1 << 7),
    VGA_EXTD_ADDR_COUNT_ENABLE   = (0x1 << 8),
    VGA_VSTATUS_CNTL		= (0x3 << 16),
    VGA_LOCK_8DOT		= (0x1 << 24),
    VGAREG_LINECMP_COMPATIBILITY_SEL     = (0x1 << 25)
};

enum D1VGA_CONTROL_BITS {
    /* D1VGA_CONTROL */
    D1VGA_MODE_ENABLE		= (0x1 << 0),
    D1VGA_TIMING_SELECT		= (0x1 << 8),
    D1VGA_SYNC_POLARITY_SELECT   = (0x1 << 9),
    D1VGA_OVERSCAN_TIMING_SELECT         = (0x1 << 10),
    D1VGA_OVERSCAN_COLOR_EN      = (0x1 << 16),
    D1VGA_ROTATE		= (0x3 << 24)
};

enum D2VGA_CONTROL_BITS {
    /* D2VGA_CONTROL */
    D2VGA_MODE_ENABLE    = (0x1 << 0),
    D2VGA_TIMING_SELECT  = (0x1 << 8),
    D2VGA_SYNC_POLARITY_SELECT   = (0x1 << 9),
    D2VGA_OVERSCAN_TIMING_SELECT         = (0x1 << 10),
    D2VGA_OVERSCAN_COLOR_EN      = (0x1 << 16),
    D2VGA_ROTATE         = (0x3 << 24)
};

enum {
    /* CLOCK_CNTL_INDEX */
    PLL_ADDR     = (0x3f << 0),
    PLL_WR_EN    = (0x1 << 7),
    PPLL_DIV_SEL         = (0x3 << 8),

    /* CLOCK_CNTL_DATA */
#define PLL_DATA         0xffffffff

    /* SPLL_FUNC_CNTL */
    SPLL_CHG_STATUS      = (0x1 << 29),
    SPLL_BYPASS_EN       = (0x1 << 25),

    /* MC_IND_INDEX */
    MC_IND_ADDR		 = (0xffff << 0),
    MC_IND_SEQ_RBS_0     = (0x1 << 16),
    MC_IND_SEQ_RBS_1     = (0x1 << 17),
    MC_IND_SEQ_RBS_2     = (0x1 << 18),
    MC_IND_SEQ_RBS_3     = (0x1 << 19),
    MC_IND_AIC_RBS       = (0x1 << 20),
    MC_IND_CITF_ARB0     = (0x1 << 21),
    MC_IND_CITF_ARB1     = (0x1 << 22),
    MC_IND_WR_EN         = (0x1 << 23),
    MC_IND_RD_INV        = (0x1 << 24)
#define MC_IND_ALL (MC_IND_SEQ_RBS_0 |  MC_IND_SEQ_RBS_1 \
                    |  MC_IND_SEQ_RBS_2 |  MC_IND_SEQ_RBS_3 \
                    |  MC_IND_AIC_RBS | MC_IND_CITF_ARB0 | MC_IND_CITF_ARB1)

    /* MC_IND_DATA */
#define MC_IND_DATA_BIT  0xffffffff
};

enum AGP_STATUS_BITS {
    AGP_1X_MODE          = 0x01,
    AGP_2X_MODE          = 0x02,
    AGP_4X_MODE          = 0x04,
    AGP_FW_MODE          = 0x10,
    AGP_MODE_MASK        = 0x17,
    AGPv3_MODE           = 0x08,
    AGPv3_4X_MODE        = 0x01,
    AGPv3_8X_MODE        = 0x02
};

enum {
    /* HDMI registers */
    HDMI_ENABLE           = 0x00,
    HDMI_STATUS           = 0x04,
    HDMI_CNTL             = 0x08,
    HDMI_UNKNOWN_0        = 0x0C,
    HDMI_AUDIOCNTL        = 0x10,
    HDMI_VIDEOCNTL        = 0x14,
    HDMI_VERSION          = 0x18,
    HDMI_UNKNOWN_1        = 0x28,
    HDMI_VIDEOINFOFRAME_0 = 0x54,
    HDMI_VIDEOINFOFRAME_1 = 0x58,
    HDMI_VIDEOINFOFRAME_2 = 0x5c,
    HDMI_VIDEOINFOFRAME_3 = 0x60,
    HDMI_32kHz_CTS        = 0xac,
    HDMI_32kHz_N          = 0xb0,
    HDMI_44_1kHz_CTS      = 0xb4,
    HDMI_44_1kHz_N        = 0xb8,
    HDMI_48kHz_CTS        = 0xbc,
    HDMI_48kHz_N          = 0xc0,
    HDMI_AUDIOINFOFRAME_0 = 0xcc,
    HDMI_AUDIOINFOFRAME_1 = 0xd0,
    HDMI_IEC60958_1       = 0xd4,
    HDMI_IEC60958_2       = 0xd8,
    HDMI_UNKNOWN_2        = 0xdc,
    HDMI_AUDIO_DEBUG_0    = 0xe0,
    HDMI_AUDIO_DEBUG_1    = 0xe4,
    HDMI_AUDIO_DEBUG_2    = 0xe8,
    HDMI_AUDIO_DEBUG_3    = 0xec
};

#endif /* _RHD_REGS_H */
