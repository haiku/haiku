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
	uint32 reserved00[0x0004];
	uint16 FifoFree;			/* little endian (FIFO internal register) */
	uint16 Nop;					/* little endian (FIFO internal register) */
	uint32 reserved01[0x00fa];
	uint32 Color1A;				/* b0-31 is color */
	struct
	{
		uint32 LeftTop;			/* b0-15 is top, b16-31 is left */
		uint32 WidthHeight;		/* b0-15 is height, b16-31 is width */
	} UnclippedRectangle[0x40];	/* command can handle upto 64 unclipped rects */
//XFree also defines: (not used AFAIK ATM)
/*
    U032 reserved04[(0x080)-3];
    struct
    {
        U032 TopLeft;
        U032 BottomRight;
    } ClipB;
    U032 Color1B;
    struct
    {
        U032 TopLeft;
        U032 BottomRight;
    } ClippedRectangle[64];
    U032 reserved05[(0x080)-5];
    struct
    {
        U032 TopLeft;
        U032 BottomRight;
    } ClipC;
    U032 Color1C;
    U032 WidthHeightC;
    U032 PointC;
    U032 MonochromeData1C;
    U032 reserved06[(0x080)+121];
    struct
    {
        U032 TopLeft;
        U032 BottomRight;
    } ClipD;
    U032 Color1D;
    U032 WidthHeightInD;
    U032 WidthHeightOutD;
    U032 PointD;
    U032 MonochromeData1D;
    U032 reserved07[(0x080)+120];
    struct
    {
        U032 TopLeft;
        U032 BottomRight;
    } ClipE;
    U032 Color0E;
    U032 Color1E;
    U032 WidthHeightInE;
    U032 WidthHeightOutE;
    U032 PointE;
    U032 MonochromeData01E;
*/
} cmd_nv3_gdi_rectangle_text;

typedef struct {
} cmd_nv_dx5_texture_triangle;

#endif
