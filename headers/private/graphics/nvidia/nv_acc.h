/* 
	definitions for used nVidia acceleration engine commands.

	Written by Rudolf Cornelissen 12/2004
*/

#ifndef NV_ACC_H
#define NV_ACC_H

typedef struct {
	uint32 reserved00[0x0004];
	uint16 FifoFree;			/* little endian (FIFO internal register) */
	uint16 Nop;					/* little endian (FIFO internal register) */
	uint32 reserved01[0x00bb];
	uint32 SetRop5;				/* b0-7 is ROP5 */
} cmd_nv_rop5_solid;

typedef struct {
	uint32 reserved00[0x0004];
	uint16 FifoFree;			/* little endian (FIFO internal register) */
	uint16 Nop;					/* little endian (FIFO internal register) */
	uint32 reserved01[0x00bb];
	uint32 TopLeft;				/* b0-15 is left, b16-31 is top */
	uint32 HeightWidth;			/* b0-15 is width, b16-31 is height */
} cmd_nv_image_black_rectangle;

typedef struct {
	uint32 reserved00[0x0004];
	uint16 FifoFree;			/* little endian (FIFO internal register) */
	uint16 Nop;					/* little endian (FIFO internal register) */
	uint32 reserved01[0x00bd];
	uint32 SetShape;			/* b0-1: %00 = 8X_8Y; %01 = 64X_1Y; %10 = 1X_64Y */
	uint32 reserved02[0x0001];
	uint32 SetColor0;			/* b0-31 is color */
	uint32 SetColor1;			/* b0-31 is color */
	uint32 SetPattern[0x0002];	/* b0-31 is bitmap */
} cmd_nv_image_pattern;

typedef struct {
	uint32 reserved00[0x0004];
	uint16 FifoFree;			/* little endian (FIFO internal register) */
	uint16 Nop;					/* little endian (FIFO internal register) */
	uint32 reserved01[0x00bb];
	uint32 SourceOrg;			/* b0-15 is X, b16-31 is Y */
	uint32 DestOrg;				/* b0-15 is X, b16-31 is Y */
	uint32 HeightWidth;			/* b0-15 is width, b16-31 is height */
} cmd_nv_image_blit;

typedef struct {
} cmd_nv3_gdi_rectangle_text;

typedef struct {
} cmd_nv_dx5_texture_triangle;

#endif
