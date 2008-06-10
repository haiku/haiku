/* 
	definitions for used nVidia acceleration engine commands.

	Written by Rudolf Cornelissen 12/2004-6/2008
*/

#ifndef NV_ACC_H
#define NV_ACC_H

/************ DMA command defines ***********/

/* FIFO channels */
#define NV_GENERAL_FIFO_CH0		0x0000
#define NV_GENERAL_FIFO_CH1		0x2000
#define NV_GENERAL_FIFO_CH2		0x4000
#define NV_GENERAL_FIFO_CH3		0x6000
#define NV_GENERAL_FIFO_CH4		0x8000
#define NV_GENERAL_FIFO_CH5		0xa000
#define NV_GENERAL_FIFO_CH6		0xc000
#define NV_GENERAL_FIFO_CH7		0xe000

/* sub-command offsets within FIFO channels */
#define NV_GENERAL_DMAPUT							0x0040
#define NV_GENERAL_DMAGET							0x0044
#define NV_ROP5_SOLID_SETROP5						0x0300
#define NV_IMAGE_BLACK_RECTANGLE_TOPLEFT			0x0300
#define NV_IMAGE_PATTERN_SETCOLORFORMAT				0x0300
#define NV_IMAGE_PATTERN_SETSHAPE					0x0308
#define NV_IMAGE_PATTERN_SETCOLOR0					0x0310
#define NV_IMAGE_BLIT_SOURCEORG						0x0300
//fixme note: non-DMA acc is still using NV3_GDI_RECTANGLE_TEXT...
//which is just as fast as NV4_GDI_RECTANGLE_TEXT, but has a hardware fault for DMA!
#define NV4_GDI_RECTANGLE_TEXT_SETCOLORFORMAT		0x0300
#define NV4_GDI_RECTANGLE_TEXT_COLOR1A				0x03fc
#define NV4_GDI_RECTANGLE_TEXT_UCR0_LEFTTOP			0x0400
#define NV4_SURFACE_FORMAT							0x0300
#define NV_SCALED_IMAGE_FROM_MEMORY_SETCOLORFORMAT	0x0300
#define NV_SCALED_IMAGE_FROM_MEMORY_SOURCEORG		0x0308
#define NV_SCALED_IMAGE_FROM_MEMORY_SOURCESIZE		0x0400


/************************
 * 3D specific commands *
 ************************/

#define NV4_DX5_TEXTURE_TRIANGLE_COLORKEY			0x0300
#define NV4_DX5_TEXTURE_TRIANGLE_TLVERTEX(i)		0x0400 + (i << 5)
#define NV4_DX5_TEXTURE_TRIANGLE_TLVDRAWPRIM(i)		0x0600 + (i << 2)
#define NV4_CONTEXT_SURFACES_ARGB_ZS_PITCH			0x0308

#endif
