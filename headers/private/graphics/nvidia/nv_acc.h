/* 
	definitions for used nVidia acceleration engine commands.

	Written by Rudolf Cornelissen 12/2004
*/

#ifndef NV_ACC_H
#define NV_ACC_H

typedef struct {
	uint32 reserved00[0x0004];
	uint16 FifoFree;	/* little endian (FIFO internal register) */
	uint16 Nop;			/* little endian (FIFO internal register) */
	uint32 reserved01[0x00bb];
	uint32 Rop3;		/* b0-7 is ROP3, b8-31 are not used */
} cmd_nv_rop5_solid;

typedef struct {
} cmd_nv_image_black_rectangle;

typedef struct {
} cmd_nv_image_pattern;

typedef struct {
} cmd_nv_image_blit;

typedef struct {
} cmd_nv3_gdi_rectangle_text;

typedef struct {
} cmd_nv_dx5_texture_triangle;

#endif
