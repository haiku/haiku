/* CTRC functionality */
/* Authors:
   Mark Watson 2/2000,
   Apsed,
   Rudolf Cornelissen 10/2002
*/

#define MODULE_BIT 0x00040000

#include "mga_std.h"

/*Adjust passed parameters to a valid mode line*/
status_t gx00_crtc_validate_timing(
	uint16 *hd_e,uint16 *hs_s,uint16 *hs_e,uint16 *ht,
	uint16 *vd_e,uint16 *vs_s,uint16 *vs_e,uint16 *vt
)
{
/*horizontal*/
	/*make all parameters multiples of 8 and confine to required number of bits*/
	*hd_e&=0x7F8;
	*hs_s&=0xFF8;
	*hs_e&=0xFF8;
	*ht  &=0xFF8;

	/*confine to a reasonable width*/
	if (*hd_e<640) *hd_e=640;
	if (*hd_e>2048) *hd_e=2048;

	/*if horizontal total does not leave room for a sensible sync pulse, increase it!*/
	if (*ht<(*hd_e+80)) *ht=*hd_e+80;

	/*make sure sync pulse is not during display*/
	if (*hs_e>(*ht-0x8)) *hs_e=*ht-0x8;
	if (*hs_s<(*hd_e+0x8)) *hs_e=*hd_e+0x8;

	/*correct sync pulse if it is too long*/
	if (*hs_e>(*hs_s+0xF8)) *hs_e=*hs_s+0xF8;

	/*fail if they are now outside required number of bits*/
	if (
		*hd_e!=(*hd_e&0x7F8) ||
		*hs_s!=(*hs_s&0xFF8) ||
		*hs_e!=(*hs_e&0xFF8) ||
		*ht  !=(*ht  &0xFF8)
	)
	{
		LOG(8,("CRTC:Horizontal timing fell out of bits\n"));
		return B_ERROR;
	}

/*vertical*/
	/*squish to required number of bits*/
	*vd_e&=0x7FF;
	*vs_s&=0xFFF;
	*vs_e&=0xFFF;
	*vt  &=0xFFF;

	/*confine to a reasonable height*/
	if (*vd_e<400) *vd_e=400;
	if (*vd_e>2048) *vd_e=2048;

	/*if vertical total does not leave room for a sync pulse, increase it!*/
	if (*vt<(*vd_e+3)) *vt=*vd_e+3;

	/*make sure sync pulse if not during display*/
	if (*vs_e>(*vt-1)) *vs_e=*vt-1;
	if (*vs_s<(*vd_e+1)) *vs_s=*vd_e+1;

	/*correct sync pulse if it is too long*/
	if (*vs_e>(*vs_s+0xF)) *vs_e=*vs_s+0xF;

	/*fail if now outside required number of bits*/
	if (
		*vd_e!=(*vd_e&0x7FF) ||
		*vs_s!=(*vs_s&0xFFF) ||
		*vs_e!=(*vs_e&0xFFF) ||
		*vt  !=(*vt  &0xFFF)
	)
	{
		LOG(8,("CRTC:Vertical timing fell out of bits\n"));
		return B_ERROR;
	}
	return B_OK;
}
	

/*set a mode line - inputs are in pixels*/
status_t gx00_crtc_set_timing(
	uint16 hd_e,uint16 hs_s,uint16 hs_e,uint16 ht,
	uint16 vd_e,uint16 vs_s,uint16 vs_e,uint16 vt,
	uint8 hsync_pos,uint8 vsync_pos
)
{
	uint32 htotal;		/*total horizontal total VCLKs*/
	uint32 hdisp_e;            /*end of horizontal display (begins at 0)*/
	uint32 hsync_s;            /*begin of horizontal sync pulse*/
	uint32 hsync_e;            /*end of horizontal sync pulse*/
	uint32 hblnk_s;            /*begin horizontal blanking*/
	uint32 hblnk_e;            /*end horizontal blanking*/

	uint32 vtotal;		/*total vertical total scanlines*/
	uint32 vdisp_e;            /*end of vertical display*/
	uint32 vsync_s;            /*begin of vertical sync pulse*/
	uint32 vsync_e;            /*end of vertical sync pulse*/
	uint32 vblnk_s;            /*begin vertical blanking*/
	uint32 vblnk_e;            /*end vertical blanking*/

	uint32 linecomp;	/*split screen and vdisp_e interrupt*/

	LOG(4,("CRTC: setting timing\n"));

	/*Modify parameters as required by the G400/G200*/
	htotal=(ht>>3)-5;
	hdisp_e=(hd_e>>3)-1;
	hsync_s=(hs_s>>3);
	hsync_e=(hs_e>>3);
	hblnk_s=hdisp_e;
	hblnk_e=htotal+4;
	
	vtotal=vt-2;
	vdisp_e=vd_e-1;
	vsync_s=vs_s-1;
	vsync_e=vs_e-1;
	vblnk_s=vdisp_e;
	vblnk_e=vtotal+1;

	linecomp=256; /*should display half the screen!*/
	
	/*log the mode I am setting*/
	LOG(2,("CRTC:\n\tHTOT:%x\n\tHDISPEND:%x\n\tHBLNKS:%x\n\tHBLNKE:%x\n\tHSYNCS:%x\n\tHSYNCE:%x\n\t",htotal,hdisp_e,hblnk_s,hblnk_e,hsync_s,hsync_e));
	LOG(2,("VTOT:%x\n\tVDISPEND:%x\n\tVBLNKS:%x\n\tVBLNKE:%x\n\tVSYNCS:%x\n\tVSYNCE:%x\n",vtotal,vdisp_e,vblnk_s,vblnk_e,vsync_s,vsync_e));

	/*actually program the card! Note linecomp is programmed to vblnk_s for VBI*/
	/*horizontal - VGA regs*/

	VGAW_I(CRTC,0,htotal&0xFF);
	VGAW_I(CRTC,1,hdisp_e&0xFF);
	VGAW_I(CRTC,2,hblnk_s&0xFF);
	VGAW_I(CRTC,3,(hblnk_e&0x1F)|0x80);
	VGAW_I(CRTC,4,hsync_s&0xFF);
	VGAW_I(CRTC,5,(hsync_e&0x1F)|((hblnk_e&0x20)<<2));
	
	/*vertical - VGA regs*/
	VGAW_I(CRTC,6,vtotal&0xFF);
	VGAW_I(CRTC,7,
	(
		((vtotal&0x100)>>(8-0)) |((vtotal&0x200)>>(9-5))|
		((vdisp_e&0x100)>>(8-1))|((vdisp_e&0x200)>>(9-6))|
		((vsync_s&0x100)>>(8-2))|((vsync_s&0x200)>>(9-7))|
		((vblnk_s&0x100)>>(8-3))|((linecomp&0x100)>>(8-4))
	));
	VGAW_I(CRTC,0x9,((vblnk_s&0x200)>>(9-5))|((linecomp&0x200)>>(9-6)));
	VGAW_I(CRTC,0x10,vsync_s&0xFF);
	VGAW_I(CRTC,0x11,((VGAR_I(CRTC,0x11))&0xF0)|(vsync_e&0xF));
	VGAW_I(CRTC,0x12,vdisp_e&0xFF);
	VGAW_I(CRTC,0x15,vblnk_s&0xFF);
	VGAW_I(CRTC,0x16,vblnk_e&0xFF);
	VGAW_I(CRTC,0x18,linecomp&0xFF);

	/*horizontal - extended regs*/
	VGAW_I(CRTCEXT,1,
	(
		((htotal&0x100)>>8)|
		((hblnk_s&0x100)>>7)|
		((hsync_s&0x100)>>6)|
		(hblnk_e&0x40)|
		(VGAR_I(CRTCEXT,1)&0xb8)
	));

	/*vertical - extended regs*/
	VGAW_I(CRTCEXT,2,
	(
	 	((vtotal&0xC00)>>10)|
		((vdisp_e&0x400)>>8)|
		((vblnk_s&0xC00)>>7)|
		((vsync_s&0xC00)>>5)|
		((linecomp&0x400)>>3)
	));

	/*set up HSYNC & VSYNC polarity*/
	VGAW(MISCW,(VGAR(MISCR)&0x3F)|((!vsync_pos)<<7)|((!hsync_pos)<<6));
	LOG(2,("HSYNC/VSYNC pol:%x %x MISC dump:%x\n",hsync_pos,vsync_pos,VGAR(MISCR)));

	return B_OK;
}

status_t gx00_crtc_depth(int mode)
{
	uint8 viddelay = 0; // in CRTCEXT3, reserved if >= G100

	if (si->ps.card_type < G100) do { // apsed TODO in caller
		if (si->ps.memory_size <= 2) { viddelay = 1<<3; break;}
		if (si->ps.memory_size <= 4) { viddelay = 0<<3; break;}
		viddelay = 2<<3; // for 8 to 16Mb of memory
	} while (0);

	/*set VCLK scaling*/
	switch(mode)
	{
	case BPP8:
		VGAW_I(CRTCEXT,3,viddelay|0x80);
		break;
	case BPP15:case BPP16:
		VGAW_I(CRTCEXT,3,viddelay|0x81);
		break;
	case BPP24:
		VGAW_I(CRTCEXT,3,viddelay|0x82);
		break;
	case BPP32:case BPP32DIR:
		VGAW_I(CRTCEXT,3,viddelay|0x83);
		break;
	}
	return B_OK;
}

status_t gx00_crtc_dpms(uint8 display,uint8 h,uint8 v) // MIL2
{
	LOG(4,("gx00_crtc_dpms (%d,%d,%d)\n", display,h,v));
	VGAW_I(SEQ,1,(!display)<<5);
	VGAW_I(CRTCEXT,1,(VGAR_I(CRTCEXT,1)&0xCF)|((!v)<<5))|((!h)<<4);
	
	VGAW_I(CRTC,0x17,0xC3);/*do not force disable all syncs and other stuff*/
	VGAW_I(CRTC,0x14,0x00);

	return B_OK;
}

status_t gx00_crtc_dpms_fetch(uint8 * display,uint8 * h,uint8 * v) // MIL2
{
	*display=!((VGAR_I(SEQ,1)&0x20)>>5);
	*h=!((VGAR_I(CRTCEXT,1)&0x10)>>4);
	*v=!((VGAR_I(CRTCEXT,1)&0x20)>>5);

	LOG(4,("gx00_crtc_dpms_fetch (%d,%d,%d)\n", *display,*h,*v));
	return B_OK;
}

status_t gx00_crtc_set_display_pitch(uint32 pitch,uint8 bpp) 
{
	uint32 offset;

	LOG(4,("CRTC: setting card pitch (offset between lines)\n"));

	/*figure out offset value hardware needs*/
	offset = (pitch*bpp)/128;

	LOG(2,("CRTC: offset: 0x%04x\n",offset));
		
	/*program the card!*/
	VGAW_I(CRTC,0x13,(offset&0xFF));
	VGAW_I(CRTCEXT,0,(VGAR_I(CRTCEXT,0)&0xCF)|((offset&0x300)>>4));
	return B_OK;
}

status_t gx00_crtc_set_display_start(uint32 startadd,uint8 bpp) 
{
	uint32 ext0;

	LOG(4,("CRTC: setting card RAM to be displayed bpp %d\n", bpp));

	/*figure out startadd value hardware needs*/
	/*switch(bpp) 
	{
		case 8:case 24:
			startadd>>=1;
		case 16:
			startadd>>=1;
		case 32:
			startadd>>=1;
			break;
	}*/
	startadd>>=3; // apsed, TODO doc Matrox g200 g400 4.6.5 is false?

	LOG(2,("CRTC: startadd: %x\n",startadd));
	LOG(2,("CRTC: frameRAM: %x\n",si->framebuffer));
	LOG(2,("CRTC: framebuffer: %x\n",si->fbc.frame_buffer));

	/*set standard registers*/
	VGAW_I(CRTC,0xD,startadd&0xFF);
	VGAW_I(CRTC,0xC,(startadd&0xFF00)>>8);

	//calculate extra bits that are standard over Gx00 series
	ext0 = VGAR_I(CRTCEXT,0)&0xB0;
	ext0|= (startadd&0xF0000)>>16;

	//if card is a G200 or G400 then do first extension bit
	if (si->ps.card_type>=G200)
		ext0|=(startadd&0x100000)>>14;
	
	//if card is a G400 then do write to its extension register
	if (si->ps.card_type>=G400)
		VGAW_I(CRTCEXT,8,((startadd&0x200000)>>21));

	//write the extension bits
	VGAW_I(CRTCEXT,0,ext0);

	return B_OK;
}

status_t gx00_crtc_mem_priority(uint8 HIPRILVL)
{	
	if (si->ps.card_type<G100) return B_ERROR; // apsed TODO, not used, see after SetDisplayMode.c/interrupt_enable()

	LOG(4,("CRTC: Setting memory priority level: %x\n",HIPRILVL));
	
	switch (HIPRILVL)
	{
	case 0:
		VGAW_I(CRTCEXT,6,0x00);
		break;
	case 1:case 2:case 3:
		VGAW_I(CRTCEXT,6,0x10|HIPRILVL);
		break;
	case 4:case 5:case 6:case 7:
		VGAW_I(CRTCEXT,6,0x20|HIPRILVL);
		break;
	default:
		LOG(8,("CRTC: Memory priority level violation: %x\n",HIPRILVL));
		return B_ERROR;
	}

	return B_OK;
}

status_t gx00_crtc_cursor_init()
{
	int i;
	uint32 * fb;
	const uint32 curadd = 0; // apsed, TODO with ramaddr -> taken care off.

	/*store cursor at the start of the framebuffer*/
	DXIW(CURADDL,curadd >> 10); /*data at curadd in framebuffer*/
	DXIW(CURADDH,curadd >> 18);
	DXIW(CURCTRL,1);

	/*set cursor colour*/
	DXIW(CURCOL0RED,0XFF);
	DXIW(CURCOL0GREEN,0xFF);
	DXIW(CURCOL0BLUE,0xFF);
	DXIW(CURCOL1RED,0);
	DXIW(CURCOL1GREEN,0);
	DXIW(CURCOL1BLUE,0);
	DXIW(CURCOL2RED,0);
	DXIW(CURCOL2GREEN,0);
	DXIW(CURCOL2BLUE,0);

	/*clear cursor*/
	fb = (uint32 *) si->framebuffer + curadd;
	for (i=0;i<(1024/4);i++)
	{
		fb[i]=0;
	}
	return B_OK;
}

status_t gx00_crtc_cursor_show()
{
	DXIW(CURCTRL,1);
	return B_OK;
}

status_t gx00_crtc_cursor_hide()
{
	DXIW(CURCTRL,0);
	return B_OK;
}

/*set up cursor shape*/
status_t gx00_crtc_cursor_define(uint8* andMask,uint8* xorMask)
{
	uint8 * cursor;
	int y;

	/*get a pointer to the cursor*/
	cursor = (uint8*) si->framebuffer;

	/*draw the cursor*/
	for(y=0;y<16;y++)
	{
		cursor[y*16+7]=~*andMask++;
		cursor[y*16+15]=*xorMask++;
		cursor[y*16+6]=~*andMask++;
		cursor[y*16+14]=*xorMask++;
	}

	return B_OK;
}

/*position the cursor*/
status_t gx00_crtc_cursor_position(uint16 x ,uint16 y)
{
	int i=64;
//	LOG(4,("DAC: cursor-> %d %d\n",x,y));

	x+=i;
	y+=i;

	DACW(CURSPOSXL,x&0xFF);
	DACW(CURSPOSXH,x>>8);
	DACW(CURSPOSYL,y&0xFF);
	DACW(CURSPOSYH,y>>8);

	return B_OK;
}
