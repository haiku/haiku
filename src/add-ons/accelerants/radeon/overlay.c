/*
	Copyright (c) 2002-2004, Thomas Kurschel
	
	Part of Radeon accelerant
		
	Hardware access routines for overlays
*/

#include "GlobalData.h"
#include "radeon_interface.h"
#include "mmio.h"
#include "overlay_regs.h"
#include "pll_regs.h"
#include "capture_regs.h"
#include "utils.h"
#include "pll_access.h"
#include <math.h>
#include <string.h>
#include "CP.h"


void Radeon_TempHideOverlay( accelerator_info *ai );

// standard (linear) gamma
static struct {
    uint16 reg;
    bool r200_or_above;
    uint32 slope;
    uint32 offset;
} std_gamma[] = {
    { RADEON_OV0_GAMMA_0_F, false, 0x100, 0x0000 },
    { RADEON_OV0_GAMMA_10_1F, false, 0x100, 0x0020 },
    { RADEON_OV0_GAMMA_20_3F, false, 0x100, 0x0040 },
    { RADEON_OV0_GAMMA_40_7F, false, 0x100, 0x0080 },
    { RADEON_OV0_GAMMA_80_BF, true, 0x100, 0x0100 },
    { RADEON_OV0_GAMMA_C0_FF, true, 0x100, 0x0100 },
    { RADEON_OV0_GAMMA_100_13F, true, 0x100, 0x0200 },
    { RADEON_OV0_GAMMA_140_17F, true, 0x100, 0x0200 },
    { RADEON_OV0_GAMMA_180_1BF, true, 0x100, 0x0300 },
    { RADEON_OV0_GAMMA_1C0_1FF, true, 0x100, 0x0300 },
    { RADEON_OV0_GAMMA_200_23F, true, 0x100, 0x0400 },
    { RADEON_OV0_GAMMA_240_27F, true, 0x100, 0x0400 },
    { RADEON_OV0_GAMMA_280_2BF, true, 0x100, 0x0500 },
    { RADEON_OV0_GAMMA_2C0_2FF, true, 0x100, 0x0500 },
    { RADEON_OV0_GAMMA_300_33F, true, 0x100, 0x0600 },
    { RADEON_OV0_GAMMA_340_37F, true, 0x100, 0x0600 },
    { RADEON_OV0_GAMMA_380_3BF, false, 0x100, 0x0700 },
    { RADEON_OV0_GAMMA_3C0_3FF, false, 0x100, 0x0700 }
};


// setup overlay unit before first use
void Radeon_InitOverlay( 
	accelerator_info *ai, int crtc_idx )
{
	vuint8 *regs = ai->regs;
	shared_info *si = ai->si;
	uint i;
	uint32 ecp_div;
	
	SHOW_FLOW0( 0, "" );
	
	// make sure we really write this value as the "toggle" bit
	// contained in it (which is zero initially) is edge-sensitive!
	// for capturing, we need to select "software" video port
	si->overlay_mgr.auto_flip_reg = RADEON_OV0_VID_PORT_SELECT_SOFTWARE;
	
	OUTREG( regs, RADEON_OV0_SCALE_CNTL, RADEON_SCALER_SOFT_RESET );
	OUTREG( regs, RADEON_OV0_AUTO_FLIP_CNTRL, si->overlay_mgr.auto_flip_reg );
	OUTREG( regs, RADEON_OV0_FILTER_CNTL, 			// use fixed filter coefficients
		RADEON_OV0_HC_COEF_ON_HORZ_Y |
		RADEON_OV0_HC_COEF_ON_HORZ_UV |
		RADEON_OV0_HC_COEF_ON_VERT_Y |
		RADEON_OV0_HC_COEF_ON_VERT_UV );
	OUTREG( regs, RADEON_OV0_KEY_CNTL, RADEON_GRAPHIC_KEY_FN_EQ |
		RADEON_VIDEO_KEY_FN_FALSE |
		RADEON_CMP_MIX_OR );
	OUTREG( regs, RADEON_OV0_TEST, 0 );
//	OUTREG( regs, RADEON_FCP_CNTL, RADEON_FCP_CNTL_GND );	// disable capture clock
//	OUTREG( regs, RADEON_CAP0_TRIG_CNTL, 0 );				// disable capturing
	OUTREG( regs, RADEON_OV0_REG_LOAD_CNTL, 0 );
	// tell deinterlacer to always show recent field
	OUTREG( regs, RADEON_OV0_DEINTERLACE_PATTERN, 
		0xaaaaa | (9 << RADEON_OV0_DEINT_PAT_LEN_M1_SHIFT) );
	
	// set gamma
	for( i = 0; i < sizeof( std_gamma ) / sizeof( std_gamma[0] ); ++i ) {
		if( !std_gamma[i].r200_or_above || si->asic >= rt_r200 ) {
			OUTREG( regs, std_gamma[i].reg,	
				(std_gamma[i].slope << 16) | std_gamma[i].offset );
		}
	}
	
	// overlay unit can only handle up to 175 MHz, if pixel clock is higher,
	// only every second pixel is handled
	if( si->crtc[crtc_idx].mode.timing.pixel_clock < 175000 )
		ecp_div = 0;
	else
		ecp_div = 1;

	Radeon_OUTPLLP( regs, si->asic, RADEON_VCLK_ECP_CNTL, 
		ecp_div << RADEON_ECP_DIV_SHIFT, ~RADEON_ECP_DIV_MASK );

	// Force the overlay clock on for integrated chips
	if ((si->asic == rt_rs100) || 
	(si->asic == rt_rs200) ||
	(si->asic == rt_rs300)) {
		Radeon_OUTPLL( regs, si->asic, RADEON_VCLK_ECP_CNTL,
        	(Radeon_INPLL( regs, si->asic, RADEON_VCLK_ECP_CNTL) | (1<<18)));
    }
    
	si->active_overlay.crtc_idx = si->pending_overlay.crtc_idx;
	
	// invalidate active colour space
	si->active_overlay.ob.space = -1;
	
	// invalidate position/scaling
	si->active_overlay.ob.width = -1;
}

// colour space transformation matrix
typedef struct space_transform
{
    float   RefLuma;	// scaling of luma to use full RGB range
    float   RefRCb;		// b/u -> r
    float   RefRY;		// g/y -> r
    float   RefRCr;		// r/v -> r
    float   RefGCb;
    float   RefGY;
    float   RefGCr;
    float   RefBCb;
    float   RefBY;
    float   RefBCr;
} space_transform;


// Parameters for ITU-R BT.601 and ITU-R BT.709 colour spaces
space_transform trans_yuv[2] =
{
    { 1.1678, 0.0, 1, 1.6007, -0.3929, 1, -0.8154, 2.0232, 1, 0.0 }, /* BT.601 */
    { 1.1678, 0.0, 1, 1.7980, -0.2139, 1, -0.5345, 2.1186, 1, 0.0 }  /* BT.709 */
};


// RGB is a pass through
space_transform trans_rgb =
	{ 1, 0, 0, 1, 0, 1, 0, 1, 0, 0 };


// set overlay colour space transformation matrix
static void Radeon_SetTransform( 
	accelerator_info *ai,
	float	    bright,
	float	    cont,
	float	    sat, 
	float	    hue,
	float	    red_intensity, 
	float	    green_intensity, 
	float	    blue_intensity,
	uint	    ref)
{
	vuint8 *regs = ai->regs;
	shared_info *si = ai->si;
	float	    OvHueSin, OvHueCos;
	float	    CAdjOff;
	float		CAdjRY, CAdjGY, CAdjBY;
	float	    CAdjRCb, CAdjRCr;
	float	    CAdjGCb, CAdjGCr;
	float	    CAdjBCb, CAdjBCr;
	float	    RedAdj,GreenAdj,BlueAdj;
	float	    OvROff, OvGOff, OvBOff;
	float		OvRY, OvGY, OvBY;
	float	    OvRCb, OvRCr;
	float	    OvGCb, OvGCr;
	float	    OvBCb, OvBCr;
	float	    Loff;
	float	    Coff;
	
	uint32	    dwOvROff, dwOvGOff, dwOvBOff;
	uint32		dwOvRY, dwOvGY, dwOvBY;
	uint32	    dwOvRCb, dwOvRCr;
	uint32	    dwOvGCb, dwOvGCr;
	uint32	    dwOvBCb, dwOvBCr;
	
	space_transform	*trans;
	
	SHOW_FLOW0( 0, "" );

	// get proper conversion formula
	switch( si->pending_overlay.ob.space ) {
	case B_YCbCr422:
	case B_YUV12:
		Loff = 16 * 4;		// internal representation is 10 Bits
		Coff = 128 * 4;
		
		if (ref >= 2) 
			ref = 0;
		
		trans = &trans_yuv[ref];
		break;
		
	case B_RGB15:
	case B_RGB16:
	case B_RGB32:
	default:
		Loff = 0;
		Coff = 0;
		trans = &trans_rgb;
	}
	
	OvHueSin = sin(hue);
	OvHueCos = cos(hue);
	
	// get matrix values to convert overlay colour space to RGB
	// applying colour adjustment, saturation and luma scaling
	// (saturation doesn't work with RGB input, perhaps it did with some
	//  maths; this is left to the reader :)
	CAdjRY = cont * trans->RefLuma * trans->RefRY;
	CAdjGY = cont * trans->RefLuma * trans->RefGY;
	CAdjBY = cont * trans->RefLuma * trans->RefBY;
	
	CAdjRCb = sat * -OvHueSin * trans->RefRCr;
	CAdjRCr = sat * OvHueCos * trans->RefRCr;
	CAdjGCb = sat * (OvHueCos * trans->RefGCb - OvHueSin * trans->RefGCr);
	CAdjGCr = sat * (OvHueSin * trans->RefGCb + OvHueCos * trans->RefGCr);
	CAdjBCb = sat * OvHueCos * trans->RefBCb;
	CAdjBCr = sat * OvHueSin * trans->RefBCb;
	
	// adjust black level
	CAdjOff = cont * trans[ref].RefLuma * bright * 1023.0;
	RedAdj = cont * trans[ref].RefLuma * red_intensity * 1023.0;
	GreenAdj = cont * trans[ref].RefLuma * green_intensity * 1023.0;
	BlueAdj = cont * trans[ref].RefLuma * blue_intensity * 1023.0;
	
	OvRY = CAdjRY;
	OvGY = CAdjGY;
	OvBY = CAdjBY;
	OvRCb = CAdjRCb;
	OvRCr = CAdjRCr;
	OvGCb = CAdjGCb;
	OvGCr = CAdjGCr;
	OvBCb = CAdjBCb;
	OvBCr = CAdjBCr;
	// apply offsets
	OvROff = RedAdj + CAdjOff -	CAdjRY * Loff - (OvRCb + OvRCr) * Coff;
	OvGOff = GreenAdj + CAdjOff - CAdjGY * Loff - (OvGCb + OvGCr) * Coff;
	OvBOff = BlueAdj + CAdjOff - CAdjBY * Loff - (OvBCb + OvBCr) * Coff;
	
	dwOvROff = ((int32)(OvROff * 2.0)) & 0x1fff;
	dwOvGOff = ((int32)(OvGOff * 2.0)) & 0x1fff;
	dwOvBOff = ((int32)(OvBOff * 2.0)) & 0x1fff;

	dwOvRY = (((int32)(OvRY * 2048.0))&0x7fff)<<17;
	dwOvGY = (((int32)(OvGY * 2048.0))&0x7fff)<<17;
	dwOvBY = (((int32)(OvBY * 2048.0))&0x7fff)<<17;
	dwOvRCb = (((int32)(OvRCb * 2048.0))&0x7fff)<<1;
	dwOvRCr = (((int32)(OvRCr * 2048.0))&0x7fff)<<17;
	dwOvGCb = (((int32)(OvGCb * 2048.0))&0x7fff)<<1;
	dwOvGCr = (((int32)(OvGCr * 2048.0))&0x7fff)<<17;
	dwOvBCb = (((int32)(OvBCb * 2048.0))&0x7fff)<<1;
	dwOvBCr = (((int32)(OvBCr * 2048.0))&0x7fff)<<17;

	OUTREG( regs, RADEON_OV0_LIN_TRANS_A, dwOvRCb | dwOvRY );
	OUTREG( regs, RADEON_OV0_LIN_TRANS_B, dwOvROff | dwOvRCr );
	OUTREG( regs, RADEON_OV0_LIN_TRANS_C, dwOvGCb | dwOvGY );
	OUTREG( regs, RADEON_OV0_LIN_TRANS_D, dwOvGOff | dwOvGCr );
	OUTREG( regs, RADEON_OV0_LIN_TRANS_E, dwOvBCb | dwOvBY );
	OUTREG( regs, RADEON_OV0_LIN_TRANS_F, dwOvBOff | dwOvBCr );
	
	si->active_overlay.ob.space = si->pending_overlay.ob.space;
}


// convert Be colour key to rgb value
static uint32 colourKey2RGB32( 
	uint32 space, uint8 red, uint8 green, uint8 blue ) 
{
	uint32 res;
	
	SHOW_FLOW0( 3, "" );
	
	// the way Be defines colour keys may be convinient to some driver developers,
	// but it's not well defined - took me some time to find out the format used
	// and still I have no idea how alpha is defined; Rudolf told me that alpha is
	// never used
	switch( space ) {
	case B_RGB15:
		res = 
			((uint32)(red >> 0) << (16+3)) | 
			((uint32)(green >> 0) << (8+3)) | 
			((blue >> 0) << 3);
		break;
	case B_RGB16:
		res = 
			((uint32)(red >> 0) << (16+3)) | 
			((uint32)(green >> 0) << (8+2)) | 
			((blue >> 0) << 3);
		break;
	case B_RGB32:
	case B_CMAP8:
		res = ((uint32)(red) << 16) | ((uint32)(green) << 8) | blue;
		break;
	default:
		res = 0;
	}
	
	SHOW_FLOW( 3, "key=%lx", res );
	return res;
}


// set colour key of overlay
static void Radeon_SetColourKey( 
	accelerator_info *ai, const overlay_window *ow )
{
	virtual_card *vc = ai->vc;
	vuint8 *regs = ai->regs;
	uint32 rgb32, mask32, min32, max32;
	
	/*SHOW_FLOW( 0, "value=%02x %02x %02x, mask=%02x %02x %02x",
		ow->red.value, ow->green.value, ow->blue.value,
		ow->red.mask, ow->green.mask, ow->blue.mask );*/
	
	// Radeons don't support value and mask as colour key but colour range
	rgb32 = colourKey2RGB32( vc->mode.space, 
		ow->red.value, ow->green.value, ow->blue.value );
	mask32 = colourKey2RGB32( vc->mode.space,
		ow->red.mask, ow->green.mask, ow->blue.mask );

	// ~mask32 are all unimportant (usually low order) bits	
	// oring this to the colour should give us the highest valid colour value
	// (add would be more precise but may lead to overflows)
	min32 = rgb32;
	max32 = rgb32 | ~mask32;
	
	OUTREG( regs, RADEON_OV0_GRAPHICS_KEY_CLR_LOW, min32 );
	OUTREG( regs, RADEON_OV0_GRAPHICS_KEY_CLR_HIGH, max32 );
	OUTREG( regs, RADEON_OV0_KEY_CNTL, 
		RADEON_GRAPHIC_KEY_FN_EQ |
		RADEON_VIDEO_KEY_FN_FALSE |
		RADEON_CMP_MIX_OR );
}

typedef struct {
	uint max_scale;					// maximum src_width/dest_width, 
									// i.e. source increment per screen pixel
	uint8 group_size; 				// size of one filter group in pixels
	uint8 p1_step_by, p23_step_by;	// > 0: log(source pixel increment)+1, 2-tap filter
									// = 0: source pixel increment = 1, 4-tap filter
} hscale_factor;

#define count_of( a ) (sizeof( a ) / sizeof( a[0] ))

// scaling/filter tables depending on overlay colour space:
// magnifying pixels is no problem, but minifying can lead to overload,
// so we have to skip pixels and/or use 2-tap filters
static hscale_factor scale_RGB16[] = {
	{ (2 << 12), 		2, 1, 1 },
	{ (4 << 12), 		2, 2, 2 },
	{ (8 << 12), 		2, 3, 3 },
	{ (16 << 12), 		2, 4, 4 },
	{ (32 << 12), 		2, 5, 5 }
};

static hscale_factor scale_RGB32[] = {
	{ (2 << 12) / 3,	2, 0, 0 },
	{ (4 << 12) / 3,	4, 1, 1 },
	{ (8 << 12) / 3,	4, 2, 2 },
	{ (4 << 12), 		4, 2, 3 },
	{ (16 << 12) / 3,	4, 3, 3 },
	{ (8 << 12), 		4, 3, 4 },
	{ (32 << 12) / 3,	4, 4, 4 },
	{ (16 << 12),		4, 5, 5 }
};

static hscale_factor scale_YUV[] = {
	{ (16 << 12) / 16,	2, 0, 0 },
	{ (16 << 12) / 12,	2, 0, 1 },	// mode 4, 1, 0 (as used by YUV12) is impossible
	{ (16 << 12) / 8,	4, 1, 1 },
	{ (16 << 12) / 6,	4, 1, 2 },
	{ (16 << 12) / 4,	4, 2, 2 },
	{ (16 << 12) / 3,	4, 2, 3 },
	{ (16 << 12) / 2,	4, 3, 3 },
	{ (16 << 12) / 1,	4, 4, 4 }
};

static hscale_factor scale_YUV12[] = {
	{ (16 << 12) / 16,			2, 0, 0 },
	{ (16 << 12) / 12,			4, 1, 0 },	
	{ (16 << 12) / 12,			2, 0, 1 },	
	{ (16 << 12) / 8,			4, 1, 1 },
	{ (16 << 12) / 6,			4, 1, 2 },
	{ (16 << 12) / 4,			4, 2, 2 },
	{ (16 << 12) / 3,			4, 2, 3 },
	{ (16 << 12) / 2,			4, 3, 3 },
	{ (int)((16 << 12) / 1.5),	4, 3, 4 },
	{ (int)((16 << 12) / 1.0),	4, 4, 4 },
	{ (int)((16 << 12) / 0.75),	4, 4, 5 },
	{ (int)((16 << 12) / 0.5),	4, 5, 5 }
};

#define min3( a, b, c ) (min( (a), min( (b), (c) )))

static hscale_factor scale_YUV9[] = {
	{ min3( (16 << 12) / 12,	(3 << 12) * 1,	(2 << 12) * 4 * 1 ),	2, 0, 0 },
	{ min3( (16 << 12) / 8, 	(3 << 12) * 1,	(2 << 12) * 4 * 1 ),	4, 1, 0 },
	{ min3( (16 << 12) / 10,	(3 << 12) * 1,	(2 << 12) * 4 * 1 ),	2, 0, 1 },
	{ min3( (16 << 12) / 6, 	(3 << 12) * 1,	(2 << 12) * 4 * 1 ),	4, 1, 1 },
	{ min3( (16 << 12) / 5, 	(3 << 12) * 1,	(2 << 12) * 4 * 2 ),	4, 1, 2 },
	{ min3( (16 << 12) / 3, 	(3 << 12) * 2,	(2 << 12) * 4 * 2 ),	4, 2, 2 },
	{ min3( (int)((16 << 12) / 2.5), 	(3 << 12) * 1,	(2 << 12) * 4 * 4 ),	4, 2, 3 },	// probably, it should be (3 << 12) * 2
	{ min3( (int)((16 << 12) / 1.5), 	(3 << 12) * 4,	(2 << 12) * 4 * 4 ),	4, 3, 3 },
	{ min3( (int)((16 << 12) / 0.75), 	(3 << 12) * 8,	(2 << 12) * 4 * 8 ),	4, 4, 4 },
	{ min3( (int)((16 << 12) / 0.625), 	(3 << 12) * 8,	(2 << 12) * 4 * 16 ),	4, 4, 5 },
	{ min3( (int)((16 << 12) / 0.375), 	(3 << 12) * 16,	(2 << 12) * 4 * 16 ),	4, 5, 5 }
};


// parameters of an overlay colour space
typedef struct {
	uint8 bpp_shift;				// log2( bytes per pixel (main plain) )
	uint8 bpuv_shift;				// log2( bytes per pixel (uv-plane) ); 
									// if there is one plane only: bpp=bpuv
	uint8 num_planes;				// number of planes
	uint8 h_uv_sub_sample_shift;	// log2( horizontal pixels per uv pair )
	uint8 v_uv_sub_sample_shift;	// log2( vertical pixels per uv pair )
	hscale_factor *factors;			// scaling/filter table
	uint8 num_factors;
} space_params;

static space_params space_params_table[16] = {
	{ 0, 0, 0, 0, 0, NULL, 0 },	// reserved
	{ 0, 0, 0, 0, 0, NULL, 0 },	// reserved
	{ 0, 0, 0, 0, 0, NULL, 0 },	// reserved
	{ 1, 1, 1, 0, 0, scale_RGB16, count_of( scale_RGB16 ) },	// RGB15
	{ 1, 1, 1, 0, 0, scale_RGB16, count_of( scale_RGB16 ) },	// RGB16
	{ 0, 0, 0, 0, 0, NULL, 0 },	// reserved
	{ 2, 2, 1, 0, 0, scale_RGB32, count_of( scale_RGB32 ) },	// RGB32
	{ 0, 0, 0, 0, 0, NULL, 0 },	// reserved
	{ 0, 0, 0, 0, 0, NULL, 0 },	// reserved
	{ 0, 0, 3, 2, 2, scale_YUV9, count_of( scale_YUV9 ) },		// YUV9
	{ 0, 0, 3, 1, 1, scale_YUV12, count_of( scale_YUV12 ) },	// YUV12, three-plane
	{ 1, 1, 1, 1, 0, scale_YUV, count_of( scale_YUV ) },		// VYUY422
	{ 1, 1, 1, 1, 0, scale_YUV, count_of( scale_YUV ) },		// YVYU422
	{ 0, 1, 2, 1, 1, scale_YUV12, count_of( scale_YUV12 ) },	// YUV12, two-plane
	{ 0, 1, 2, 1, 1, NULL, 0 },	// ???
	{ 0, 0, 0, 0, 0, NULL, 0 }	// reserved
};

// get appropriate scaling/filter parameters
static hscale_factor *getHScaleFactor( 
	accelerator_info *ai,
	space_params *params, 
	uint32 src_left, uint32 src_right, uint32 *h_inc )
{
	uint words_per_p1_line, words_per_p23_line, max_words_per_line;
	bool p1_4tap_allowed, p23_4tap_allowed;
	uint i;
	uint num_factors;
	hscale_factor *factors;

	SHOW_FLOW0( 3, "" );

	// check whether fifo is large enough to feed vertical 4-tap-filter

	words_per_p1_line = 
		ceilShiftDiv( (src_right - 1) << params->bpp_shift, 4 ) - 
		((src_left << params->bpp_shift) >> 4) + 1;
	words_per_p23_line = 
		ceilShiftDiv( (src_right - 1) << params->bpuv_shift, 4 ) - 
		((src_left << params->bpuv_shift) >> 4) + 1;

	// overlay scaler line length differs for different revisions 
	// this needs to be maintained by hand 
	if (ai->si->asic == rt_r200 || ai->si->asic >= rt_r300)
		max_words_per_line = 1920 / 16;
	else
		max_words_per_line = 1536 / 16;

	switch (params->num_planes) {
		case 3:
			p1_4tap_allowed = words_per_p1_line < max_words_per_line / 2;
			p23_4tap_allowed = words_per_p23_line < max_words_per_line / 4;
			break;
		case 2:
			p1_4tap_allowed = words_per_p1_line < max_words_per_line / 2;
			p23_4tap_allowed = words_per_p23_line < max_words_per_line / 2;
			break;
		case 1:
		default:
			p1_4tap_allowed = p23_4tap_allowed = words_per_p1_line < max_words_per_line;
			break;
	}

	SHOW_FLOW( 3, "p1_4tap_allowed=%d, p23_4t_allowed=%d", 
		(int)p1_4tap_allowed, (int)p23_4tap_allowed );

	// search for proper scaling/filter entry
	factors = params->factors;
	num_factors = params->num_factors;

	if (factors == NULL || num_factors == 0)
		return NULL;

	for (i = 0; i < num_factors; ++i, ++factors) {
		if (*h_inc <= factors->max_scale
			&& (factors->p1_step_by > 0 || p1_4tap_allowed)
			&& (factors->p23_step_by > 0 || p23_4tap_allowed))
			break;
	}

	if (i == num_factors) {
		// overlay is asked to be scaled down more than allowed,
		// so use least scaling factor supported
		--factors;
		*h_inc = factors->max_scale;
	}

	SHOW_FLOW( 3, "group_size=%d, p1_step_by=%d, p23_step_by=%d", 
		factors->group_size, factors->p1_step_by, factors->p23_step_by );

	return factors;
}			


#define I2FF( a, shift ) ((uint32)((a) * (1 << (shift))))


// show overlay on screen
static status_t Radeon_ShowOverlay( 
	accelerator_info *ai, int crtc_idx )
{
	virtual_card *vc = ai->vc;
	shared_info *si = ai->si;
	vuint8 *regs = ai->regs;
	overlay_info *overlay = &si->pending_overlay;
	overlay_buffer_node *node = overlay->on;
	crtc_info *crtc = &si->crtc[crtc_idx];

	uint32 ecp_div;
	uint32 v_inc, h_inc;
	uint32 src_v_inc, src_h_inc;
	uint32 src_left, src_top, src_right, src_bottom;
	int32 dest_left, dest_top, dest_right, dest_bottom;
	uint32 offset;
	uint32 tmp;
	uint32 p1_h_accum_init, p23_h_accum_init, p1_v_accum_init, p23_v_accum_init;
	uint32 p1_active_lines, p23_active_lines;
	hscale_factor *factors;
	space_params *params;
	
	uint32 p1_h_inc, p23_h_inc;
	uint32 p1_x_start, p1_x_end;
	uint32 p23_x_start, p23_x_end;
	
	uint scale_ctrl;
		
	/*uint32 buffer[20*2];
	uint idx = 0;*/
	
	SHOW_FLOW0( 0, "" );
	
	Radeon_SetColourKey( ai, &overlay->ow );
	
	// overlay unit can only handle up to 175 MHz; if pixel clock is higher,
	// only every second pixel is handled
	// (this devider is gets written into PLL by InitOverlay,
	//  so we don't need to do it ourself)
	if( crtc->mode.timing.pixel_clock < 175000 )
		ecp_div = 0;
	else
		ecp_div = 1;


	// scaling is independant of clipping, get this first
	{
		uint32 src_width, src_height;

		src_width = overlay->ov.width;
		src_height = overlay->ov.height;
	
		// this is for graphics card
		v_inc = (src_height << 20) / overlay->ow.height;
		h_inc = (src_width << (12 + ecp_div)) / overlay->ow.width;
		
	
		// this is for us	
		src_v_inc = (src_height << 16) / overlay->ow.height;
		src_h_inc = (src_width << 16) / overlay->ow.width;
	}
	
	// calculate unclipped position/size
	// TBD: I assume that overlay_window.offset_xyz is only a hint where 
	//      no overlay is visible; another interpretation were to zoom 
	//      the overlay so it fits into remaining space
	src_left = (overlay->ov.h_start << 16) + overlay->ow.offset_left * src_h_inc;
	src_top = (overlay->ov.v_start << 16) + overlay->ow.offset_top * src_v_inc;
	src_right = ((overlay->ov.h_start + overlay->ov.width) << 16) - 
		overlay->ow.offset_right * src_h_inc;
	src_bottom = ((overlay->ov.v_start + overlay->ov.height) << 16) - 
		overlay->ow.offset_top * src_v_inc;
	dest_left = overlay->ow.h_start + overlay->ow.offset_left;
	dest_top = overlay->ow.v_start + overlay->ow.offset_top;
	dest_right = overlay->ow.h_start + overlay->ow.width - overlay->ow.offset_right;
	dest_bottom = overlay->ow.v_start + overlay->ow.height - overlay->ow.offset_bottom;
	
	SHOW_FLOW( 3, "ow: h=%d, v=%d, width=%d, height=%d",
		overlay->ow.h_start, overlay->ow.v_start, 
		overlay->ow.width, overlay->ow.height );
		
	SHOW_FLOW( 3, "offset_left=%d, offset_right=%d, offset_top=%d, offset_bottom=%d", 
		overlay->ow.offset_left, overlay->ow.offset_right, 
		overlay->ow.offset_top, overlay->ow.offset_bottom );

	
	// apply virtual screen
	dest_left -= vc->mode.h_display_start + crtc->rel_x;
	dest_top -= vc->mode.v_display_start + crtc->rel_y;
	dest_right -= vc->mode.h_display_start + crtc->rel_x;
	dest_bottom -= vc->mode.v_display_start + crtc->rel_y;

	// clip to visible area
	if( dest_left < 0 ) {
		src_left += -dest_left * src_h_inc;
		dest_left = 0;
	}
	if( dest_top < 0 ) {
		src_top += -dest_top * src_v_inc;
		dest_top = 0;
	}
	
	SHOW_FLOW( 3, "mode: w=%d, h=%d", 
		crtc->mode.timing.h_display, crtc->mode.timing.v_display );
	
	if( dest_right > crtc->mode.timing.h_display )
		dest_right = crtc->mode.timing.h_display;
	if( dest_bottom > crtc->mode.timing.v_display )
		dest_bottom = crtc->mode.timing.v_display;

	SHOW_FLOW( 3, "src=(%d, %d, %d, %d)", 
		src_left, src_top, src_right, src_bottom );
	SHOW_FLOW( 3, "dest=(%d, %d, %d, %d)", 
		dest_left, dest_top, dest_right, dest_bottom );


	// especially with multi-screen modes the overlay may not be on screen at all
	if( dest_left >= dest_right || dest_top >= dest_bottom ||
		src_left >= src_right || src_top >= src_bottom )
	{
		Radeon_TempHideOverlay( ai );
		goto done;
	}
	

	// let's calculate all those nice register values
	SHOW_FLOW( 3, "ati_space=%d", node->ati_space );
	params = &space_params_table[node->ati_space];

	// choose proper scaler
	{
		factors = getHScaleFactor( ai, params, src_left >> 16, src_right >> 16, &h_inc );
		if( factors == NULL )
			return B_ERROR;
			
		p1_h_inc = factors->p1_step_by > 0 ? 
			h_inc >> (factors->p1_step_by - 1) : h_inc;
		p23_h_inc = 
			(factors->p23_step_by > 0 ? h_inc >> (factors->p23_step_by - 1) : h_inc) 
			>> params->h_uv_sub_sample_shift;
		
		SHOW_FLOW( 3, "p1_h_inc=%x, p23_h_inc=%x", p1_h_inc, p23_h_inc );
	}

	// get register value for start/end position of overlay image (pixel-precise only)
	{
		uint32 p1_step_size, p23_step_size;
		uint32 p1_left, p1_right, p1_width;
		uint32 p23_left, p23_right, p23_width;
		
		p1_left = src_left >> 16;
		p1_right = src_right >> 16;
		p1_width = p1_right - p1_left;
		
		p1_step_size = factors->p1_step_by > 0 ? (1 << (factors->p1_step_by - 1)) : 1;
		p1_x_start = p1_left % (16 >> params->bpp_shift);
		p1_x_end = ((p1_x_start + p1_width - 1) / p1_step_size) * p1_step_size;
		
		SHOW_FLOW( 3, "p1_x_start=%d, p1_x_end=%d", p1_x_start, p1_x_end );
	
		p23_left = (src_left >> 16) >> params->h_uv_sub_sample_shift;
		p23_right = (src_right >> 16) >> params->h_uv_sub_sample_shift;
		p23_width = p23_right - p23_left;
	
		p23_step_size = factors->p23_step_by > 0 ? (1 << (factors->p23_step_by - 1)) : 1;
		// if resolution of Y and U/V differs but YUV are stored in one 
		// plane then UV alignment depends on Y data, therefore the hack
		// (you are welcome to replace this with some cleaner code ;)
		p23_x_start = p23_left % 
			((16 >> params->bpuv_shift) / 
			 (node->ati_space == 11 || node->ati_space == 12 ? 2 : 1));
		p23_x_end = (int)((p23_x_start + p23_width - 1) / p23_step_size) * p23_step_size;
		
		SHOW_FLOW( 3, "p23_x_start=%d, p23_x_end=%d", p23_x_start, p23_x_end );
		
		// get memory location of first word to be read by scaler
		// (save relative offset for fast update)
		si->active_overlay.rel_offset = (src_top >> 16) * node->buffer.bytes_per_row + 
			((p1_left << params->bpp_shift) & ~0xf);
		offset = node->mem_offset + si->active_overlay.rel_offset;
		
		SHOW_FLOW( 3, "rel_offset=%x", si->active_overlay.rel_offset );
	}
	
	// get active lines for scaler
	// (we could add additional blank lines for DVD letter box mode,
	//  but this is not supported by API; additionally, this only makes
	//  sense if want to put subtitles onto the black border, which is
	//  supported neither)
	{
		uint16 int_top, int_bottom;
		
		int_top = src_top >> 16;
		int_bottom = (src_bottom >> 16);
		
		p1_active_lines = int_bottom - int_top - 1;
		p23_active_lines = 
			ceilShiftDiv( int_bottom - 1, params->v_uv_sub_sample_shift ) - 
			(int_top >> params->v_uv_sub_sample_shift);
			
		SHOW_FLOW( 3, "p1_active_lines=%d, p23_active_lines=%d", 
			p1_active_lines, p23_active_lines );
	}
	
	// if picture is stretched for flat panel, we need to scale all
	// vertical values accordingly
	// TBD: there is no description at all concerning this, so v_accum_init may
	//      need to be initialized based on original value
	{
		if( (crtc->active_displays & (dd_lvds | dd_dvi)) != 0 ) {
			uint64 v_ratio;
			
			// convert 32.32 format to 16.16 format; else we 
			// cannot multiply two fixed point values without
			// overflow
			v_ratio = si->flatpanels[crtc->flatpanel_port].v_ratio >> (FIX_SHIFT - 16);
			
			v_inc = (v_inc * v_ratio) >> 16;
		}
		
		SHOW_FLOW( 3, "v_inc=%x", v_inc );
	}
	
	// get initial horizontal scaler values, taking care of precharge
	// don't ask questions about formulas - take them as is
	// (TBD: home-brewed sub-pixel source clipping may be wrong, 
	//       especially for uv-planes)
	{
		uint32 p23_group_size;

	    tmp = ((src_left & 0xffff) >> 11) + (
	    	(
		    	I2FF( p1_x_start % factors->group_size, 12 ) + 
		    	I2FF( 2.5, 12 ) + 
		    	p1_h_inc / 2 +
		    	I2FF( 0.5, 12-5 )	// rounding
	        ) >> (12 - 5));	// scaled by 1 << 5
	        
	    SHOW_FLOW( 3, "p1_h_accum_init=%x", tmp );
	
		p1_h_accum_init = 
			((tmp << 15) & RADEON_OV0_P1_H_ACCUM_INIT_MASK) |
			((tmp << 23) & RADEON_OV0_P1_PRESHIFT_MASK);
		
		
		p23_group_size = 2;
		
		tmp = ((src_left & 0xffff) >> 11) + (
			(
				I2FF( p23_x_start % p23_group_size, 12 ) + 
				I2FF( 2.5, 12 ) +
				p23_h_inc / 2 +
				I2FF( 0.5, 12-5 )	// rounding 
			) >> (12 - 5)); // scaled by 1 << 5
	
		SHOW_FLOW( 3, "p23_h_accum_init=%x", tmp );
	
		p23_h_accum_init = 
			((tmp << 15) & RADEON_OV0_P23_H_ACCUM_INIT_MASK) |
			((tmp << 23) & RADEON_OV0_P23_PRESHIFT_MASK);
	}

	// get initial vertical scaler values, taking care of precharge
	{
		uint extra_full_line;

		extra_full_line = factors->p1_step_by == 0 ? 1 : 0;
	
	    tmp = ((src_top & 0x0000ffff) >> 11) + (
	    	(min( 
		    	I2FF( 1.5, 20 ) + I2FF( extra_full_line, 20 ) + v_inc / 2, 
	    		I2FF( 2.5, 20 ) + 2 * I2FF( extra_full_line, 20 )
	    	 ) + I2FF( 0.5, 20-5 )) // rounding
	    	>> (20 - 5)); // scaled by 1 << 5
	    	
	    SHOW_FLOW( 3, "p1_v_accum_init=%x", tmp );
	
		p1_v_accum_init = 
			((tmp << 15) & RADEON_OV0_P1_V_ACCUM_INIT_MASK) | 0x00000001;

	
		extra_full_line = factors->p23_step_by == 0 ? 1 : 0;
	
		if( params->v_uv_sub_sample_shift > 0 ) {
			tmp = ((src_top & 0x0000ffff) >> 11) + (
				(min( 
					I2FF( 1.5, 20 ) + 
						I2FF( extra_full_line, 20 ) + 
						((v_inc / 2) >> params->v_uv_sub_sample_shift), 
					I2FF( 2.5, 20 ) + 
						2 * I2FF( extra_full_line, 20 )
				 ) + I2FF( 0.5, 20-5 )) // rounding
				>> (20 - 5)); // scaled by 1 << 5
		} else {
			tmp = ((src_top & 0x0000ffff) >> 11) + (
				(
					I2FF( 2.5, 20 ) + 
					2 * I2FF( extra_full_line, 20 ) +
					I2FF( 0.5, 20-5 )	// rounding
				) >> (20 - 5)); // scaled by 1 << 5
		}
		
		SHOW_FLOW( 3, "p23_v_accum_init=%x", tmp );
	
		p23_v_accum_init = 
			((tmp << 15) & RADEON_OV0_P23_V_ACCUM_INIT_MASK) | 0x00000001;		
	}

	// show me what you've got!
	// we could lock double buffering of overlay unit during update
	// (new values are copied during vertical blank, so if we've updated
	// only some of them, you get a whole frame of mismatched values)
	// but during tests I couldn't get the artifacts go away, so
	// we use the dangerous way which has the pro to not require any
	// waiting
	
	// let's try to lock overlay unit
	// we had to wait now until the lock takes effect, but this is
	// impossible with CCE; perhaps we have to convert this code to 
	// direct register access; did that - let's see what happens...
	OUTREG( regs, RADEON_OV0_REG_LOAD_CNTL, RADEON_REG_LD_CTL_LOCK );
	
	// wait until register access is locked
	while( (INREG( regs, RADEON_OV0_REG_LOAD_CNTL) 
		& RADEON_REG_LD_CTL_LOCK_READBACK) == 0 )
		;
	
	OUTREG( regs, RADEON_OV0_VID_BUF0_BASE_ADRS, offset );
	OUTREG( regs, RADEON_OV0_VID_BUF_PITCH0_VALUE, node->buffer.bytes_per_row );
	OUTREG( regs, RADEON_OV0_H_INC, p1_h_inc | (p23_h_inc << 16) );
	OUTREG( regs, RADEON_OV0_STEP_BY, factors->p1_step_by | (factors->p23_step_by << 8) );
	OUTREG( regs, RADEON_OV0_V_INC, v_inc );
	
	OUTREG( regs,
		crtc->crtc_idx == 0 ? RADEON_OV0_Y_X_START : RADEON_OV1_Y_X_START, 
		(dest_left) | (dest_top << 16) );
	OUTREG( regs, 
		crtc->crtc_idx == 0 ? RADEON_OV0_Y_X_END : RADEON_OV1_Y_X_END,
		(dest_right - 1) | ((dest_bottom - 1) << 16) );

	OUTREG( regs, RADEON_OV0_P1_BLANK_LINES_AT_TOP, 
		RADEON_P1_BLNK_LN_AT_TOP_M1_MASK | (p1_active_lines << 16) );
	OUTREG( regs, RADEON_OV0_P1_X_START_END, p1_x_end | (p1_x_start << 16) );
	OUTREG( regs, RADEON_OV0_P1_H_ACCUM_INIT, p1_h_accum_init );
	OUTREG( regs, RADEON_OV0_P1_V_ACCUM_INIT, p1_v_accum_init );
	
	OUTREG( regs, RADEON_OV0_P23_BLANK_LINES_AT_TOP, 
		RADEON_P23_BLNK_LN_AT_TOP_M1_MASK | (p23_active_lines << 16) );
	OUTREG( regs, RADEON_OV0_P2_X_START_END, 
		p23_x_end | (p23_x_start << 16) );
	OUTREG( regs, RADEON_OV0_P3_X_START_END, 
		p23_x_end | (p23_x_start << 16) );
	OUTREG( regs, RADEON_OV0_P23_H_ACCUM_INIT, p23_h_accum_init );
	OUTREG( regs, RADEON_OV0_P23_V_ACCUM_INIT, p23_v_accum_init );
	
	OUTREG( regs, RADEON_OV0_TEST, node->test_reg );
	
	scale_ctrl = RADEON_SCALER_ENABLE | 
		RADEON_SCALER_DOUBLE_BUFFER | 
		(node->ati_space << 8) | 
		/* RADEON_SCALER_ADAPTIVE_DEINT | */
		RADEON_SCALER_BURST_PER_PLANE |
		(crtc->crtc_idx == 0 ? 0 : RADEON_SCALER_CRTC_SEL );
		
	switch (node->ati_space << 8) {
		case RADEON_SCALER_SOURCE_15BPP: // RGB15
		case RADEON_SCALER_SOURCE_16BPP:
		case RADEON_SCALER_SOURCE_32BPP:
			OUTREG( regs, RADEON_OV0_SCALE_CNTL, scale_ctrl | 
							RADEON_SCALER_LIN_TRANS_BYPASS);
			break;
		case RADEON_SCALER_SOURCE_VYUY422: // VYUY422
		case RADEON_SCALER_SOURCE_YVYU422: // YVYU422
			OUTREG( regs, RADEON_OV0_SCALE_CNTL, scale_ctrl);
			break;
		default:
			SHOW_FLOW(4, "What overlay format is this??? %d", node->ati_space);
			OUTREG( regs, RADEON_OV0_SCALE_CNTL, scale_ctrl |
			 (( ai->si->asic >= rt_r200) ? R200_SCALER_TEMPORAL_DEINT : 0));
		
	}
	
	si->overlay_mgr.auto_flip_reg ^= RADEON_OV0_SOFT_EOF_TOGGLE;
	
	OUTREG( regs, RADEON_OV0_AUTO_FLIP_CNTRL, 
		si->overlay_mgr.auto_flip_reg );
	
	OUTREG( regs, RADEON_OV0_REG_LOAD_CNTL, 0 );
	
done:
	ai->si->active_overlay.on = ai->si->pending_overlay.on;
	ai->si->active_overlay.ow = ai->si->pending_overlay.ow;
	ai->si->active_overlay.ov = ai->si->pending_overlay.ov;
	ai->si->active_overlay.ob = ai->si->pending_overlay.ob;
	ai->si->active_overlay.h_display_start = vc->mode.h_display_start;
	ai->si->active_overlay.v_display_start = vc->mode.v_display_start;

	return B_OK;
}


// hide overlay, but not permanently
void Radeon_TempHideOverlay( 
	accelerator_info *ai )
{
	SHOW_FLOW0( 3, "" );

	OUTREG( ai->regs, RADEON_OV0_SCALE_CNTL, 0 );
}


// hide overlay (can be called even if there is none visible)
void Radeon_HideOverlay( 
	accelerator_info *ai )
{
	shared_info *si = ai->si;
	
	Radeon_TempHideOverlay( ai );

	// remember that there is no overlay to be shown	
	si->active_overlay.on = NULL;
	si->active_overlay.prev_on = NULL;
	si->pending_overlay.on = NULL;
	
	// invalidate active head so it will be setup again once
	// a new overlay is shown
	si->active_overlay.crtc_idx = -1;
}


// show new overlay buffer with same parameters as last one
static void Radeon_ReplaceOverlayBuffer( 
	accelerator_info *ai )
{
#if 0
	shared_info *si = ai->si;
	vuint8 *regs = ai->regs;
	uint32 offset;
	int /*old_buf, */new_buf;
	
	offset = si->pending_overlay.on->mem_offset + si->active_overlay.rel_offset;

	/*old_buf = si->overlay_mgr.auto_flip_reg & RADEON_OV0_SOFT_BUF_NUM_MASK;
	new_buf = old_buf == 0 ? 3 : 0;
	si->overlay_mgr.auto_flip_reg &= ~RADEON_OV0_SOFT_BUF_NUM_MASK;
	si->overlay_mgr.auto_flip_reg |= new_buf;*/
	new_buf = 0;
	
	// lock overlay registers
/*	OUTREG( regs, RADEON_OV0_REG_LOAD_CNTL, RADEON_REG_LD_CTL_LOCK );
	
	// wait until register access is locked
	while( (INREG( regs, RADEON_OV0_REG_LOAD_CNTL) 
		& RADEON_REG_LD_CTL_LOCK_READBACK) == 0 )
		;*/
	
	// setup new buffer
	/*OUTREG( regs, 
		new_buf == 0 ? RADEON_OV0_VID_BUF_PITCH0_VALUE : RADEON_OV0_VID_BUF_PITCH1_VALUE, 
		si->pending_overlay.on->buffer.bytes_per_row );*/
	OUTREG( regs, 
		new_buf == 0 ? RADEON_OV0_VID_BUF0_BASE_ADRS : RADEON_OV0_VID_BUF3_BASE_ADRS, 
		offset | (new_buf == 0 ? 0 : RADEON_VIF_BUF0_PITCH_SEL));
	
	// make changes visible	
	si->overlay_mgr.auto_flip_reg ^= RADEON_OV0_SOFT_EOF_TOGGLE;
	
	OUTREG( regs, RADEON_OV0_AUTO_FLIP_CNTRL, si->overlay_mgr.auto_flip_reg );
	
	// unlock overlay registers
//	OUTREG( regs, RADEON_OV0_REG_LOAD_CNTL, 0 );

	ai->si->active_overlay.on = ai->si->pending_overlay.on;
#else
	shared_info *si = ai->si;
	uint32 offset;
	
	if ( ai->si->acc_dma )
	{
		START_IB();
	
		offset = si->pending_overlay.on->mem_offset + si->active_overlay.rel_offset;
		
		WRITE_IB_REG( RADEON_OV0_VID_BUF0_BASE_ADRS, offset);
		
		si->overlay_mgr.auto_flip_reg ^= RADEON_OV0_SOFT_EOF_TOGGLE;
		WRITE_IB_REG( RADEON_OV0_AUTO_FLIP_CNTRL, si->overlay_mgr.auto_flip_reg );
		
		SUBMIT_IB();
	} else {
		Radeon_WaitForFifo( ai, 2 );
		offset = si->pending_overlay.on->mem_offset + si->active_overlay.rel_offset;
		
		OUTREG( ai->regs, RADEON_OV0_VID_BUF0_BASE_ADRS, offset);
		
		si->overlay_mgr.auto_flip_reg ^= RADEON_OV0_SOFT_EOF_TOGGLE;
		OUTREG( ai->regs, RADEON_OV0_AUTO_FLIP_CNTRL, si->overlay_mgr.auto_flip_reg );
	}	
	ai->si->active_overlay.on = ai->si->pending_overlay.on;
#endif
}


// get number of pixels of overlay shown on virtual port
static int getIntersectArea( 
	accelerator_info *ai, overlay_window *ow, crtc_info *crtc )
{
	virtual_card *vc = ai->vc;
	int left, top, right, bottom;
	
	left = ow->h_start - (vc->mode.h_display_start + crtc->rel_x);
	top = ow->v_start - (vc->mode.v_display_start + crtc->rel_y);
	right = left + ow->width;
	bottom = top + ow->height;
	
	if( left < 0 )
		left = 0;
	if( top < 0 )
		top = 0;
	if( right > crtc->mode.timing.h_display )
		right = crtc->mode.timing.h_display;
	if( bottom > crtc->mode.timing.v_display )
		bottom = crtc->mode.timing.v_display;
		
	if( right < left || bottom < top )
		return 0;
		
	return (right - left) * (bottom - top);
}


// update overlay, to be called whenever something in terms of 
// overlay have or can have been changed
status_t Radeon_UpdateOverlay( 
	accelerator_info *ai )
{
	virtual_card *vc = ai->vc;
	shared_info *si = ai->si;
	int crtc_idx;
	
	float brightness = 0.0f;
	float contrast = 1.0f;
	float saturation = 1.0f;
	float hue = 0.0f;
    int32 ref = 0;
    
    SHOW_FLOW0( 3, "" );

	// don't mess around with overlay of someone else    
    if( !vc->uses_overlay )
    	return B_OK;

	// make sure there really is an overlay
	if( si->pending_overlay.on == NULL )
		return B_OK;

	// verify that the overlay is still valid
	if( (uint32)si->pending_overlay.ot != si->overlay_mgr.token )
		return B_BAD_VALUE;
		
	if( vc->different_heads > 1 ) {
		int area0, area1;

		// determine on which port most of the overlay is shown
		area0 = getIntersectArea( ai, &si->pending_overlay.ow, &si->crtc[0] );
		area1 = getIntersectArea( ai, &si->pending_overlay.ow, &si->crtc[0] );
		
		SHOW_FLOW( 3, "area0=%d, area1=%d", area0, area1 );
		
		if( area0 >= area1 )
			crtc_idx = 0;
		else
			crtc_idx = 1;
			
	} else if( vc->independant_heads > 1 ) {
		// both ports show the same, use "swap displays" to decide
		// where to show the overlay (to be improved as this flag isn't
		// really designed for that)
		if( vc->swap_displays )
			crtc_idx = 1;
		else
			crtc_idx = 0;
			
	} else {
	
		// one crtc used only - pick the one that we use
		crtc_idx = vc->used_crtc[0] ? 0 : 1;
	}
	
	si->pending_overlay.crtc_idx = crtc_idx;

	// only update registers that have been changed to minimize work
	if( si->active_overlay.crtc_idx != si->pending_overlay.crtc_idx ) {
		Radeon_InitOverlay( ai, crtc_idx );
	} 
	
	if( si->active_overlay.ob.space != si->pending_overlay.ob.space ) {
		Radeon_SetTransform( ai, brightness, contrast, saturation, hue, 0, 0, 0, ref );
	}

	if( memcmp( &si->active_overlay.ow, &si->pending_overlay.ow, sizeof( si->active_overlay.ow )) != 0 || 
		memcmp( &si->active_overlay.ov, &si->pending_overlay.ov, sizeof( si->active_overlay.ov )) != 0 ||
		si->active_overlay.h_display_start != vc->mode.h_display_start ||
		si->active_overlay.v_display_start != vc->mode.v_display_start ||
		si->active_overlay.ob.width != si->pending_overlay.ob.width ||
		si->active_overlay.ob.height != si->pending_overlay.ob.height ||
		si->active_overlay.ob.bytes_per_row != si->pending_overlay.ob.bytes_per_row )
		Radeon_ShowOverlay( ai, crtc_idx );
		
	else if( si->active_overlay.on != si->pending_overlay.on )
		Radeon_ReplaceOverlayBuffer( ai );
		
	SHOW_FLOW0( 3, "success" );
	
	return B_OK;
}
