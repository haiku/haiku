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
	struct {
		uint32 LeftTop;			/* b0-15 is top, b16-31 is left */
		uint32 WidthHeight;		/* b0-15 is height, b16-31 is width */
	} UnclippedRectangle[0x40];	/* command can handle upto 64 unclipped rects */
/* fixme: XFree also defines the registers below:
 * (used for the 2D 'ScanlineCPUToScreenColorExpandFill' and 'ColorExpandScanline'
 * functions.)
 * We don't use this currently. */
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

/* This command could (at least) potentially speed-up our fill_span command... :-) */
//fixme: test this 2D command!
typedef struct {
	uint32 reserved00[0x0004];
	uint16 FifoFree;			/* little endian (FIFO internal register) */
	uint16 Nop;					/* little endian (FIFO internal register) */
	uint32 reserved01[0x00bc];	/* fixme? there's more here that's not used apparantly */
	uint32 Color;				/* b0-31 is color */
	uint32 reserved02[0x003e];
	struct {
		uint32 Point0;			/* b0-15 is X, b16-31 is Y: starting coordinate */
		uint32 Point1;			/* b0-15 is X, b16-31 is Y: ending coordinate */
	} Line[0x10];				/* command can handle upto 16 lines */
	struct {
		uint32 Point0X;			/* b0-31 is X: starting coordinate */
		uint32 Point0Y;			/* b0-31 is Y: starting coordinate */
		uint32 Point1X;			/* b0-31 is X: ending coordinate */
		uint32 Point1Y;			/* b0-31 is Y: ending coordinate */
	} Line32[0x08];				/* cmd can handle upto 8 lines with 32-bit coordinates */
	struct {
		uint32 Point;			/* b0-15 is X, b16-31 is Y */
	} Polyline[0x20];			/* cmd can handle upto 32 polylines */
	struct {
		uint32 PointX;			/* b0-31 is X */
		uint32 PointY;			/* b0-31 is Y */
	} Polyline32[0x10];			/* command can handle upto 16 polylines with 32-bit coord's */
	struct {
		uint32 Color;			/* b0-31 is color */
		uint32 Point;			/* b0-15 is X, b16-31 is Y */
	} ColorPolyline[0x10];		/* cmd can handle upto 16 individually colored polylines */
} cmd_nv1_render_solid_lin;

/* Someone defined this in XFree once (as 'RivaRectangle') but never used it:
 * a handle should also be defined in the engine's init code, which was never done AFAIK.
 * The command could be very interesting for us, as it could potentially speed-up our
 * rectangle_fills :-) */
//fixme: test this 2D command!
typedef struct {
	uint32 reserved00[0x0004];
	uint16 FifoFree;			/* little endian (FIFO internal register) */
	uint16 Nop;					/* little endian (FIFO internal register) */
	uint32 reserved01[0x00bc];	/* fixme? there's more here that's not used apparantly */
	uint32 Color;				/* b0-31 is color */
	uint32 reserved02[0x003e];
	struct {
		uint32 TopLeft;			/* b0-15 is left, b16-31 is top */
		uint32 HeightWidth;		/* b0-15 is width, b16-31 is height */
	} Rectangle[0x10];			/* command can handle upto 16 rectangles */
} cmd_nv_render_solid_rectangle;/* nv1_render_solid_rectangle is identical */

//fixme: complete or remove (this 2D cmd is not used currently)
typedef struct {
} cmd_nv1_image_from_cpu;		/* 'pixmap': used in XFree 4.2.0, but not beyond.
								 * (Used for the 2D 'ImageWriteScanline' and
								 *  'ScanlineImageWriteRect' functions.)
								 * Is this command actually usefull? */


/************************
 * 3D specific commands *
 ************************/

typedef struct {
	uint32 reserved00[0x0004];
	uint16 FifoFree;			/* little endian (FIFO internal register) */
	uint16 Nop;					/* little endian (FIFO internal register) */
	uint32 reserved01[0x00bb];
	uint32 Colorkey;			/* texture colorkey */
	uint32 Offset;				/* texture offset */
	uint32 Format;				/* texture colorspace, size, and a lot more */
	uint32 Filter;				/* texture filtering modes (used for scaling) */
	uint32 Blend;				/* triangle blend: shade, perspective, specular.. */
	uint32 Control;				/* triangle control: Z-enable, culling, dither.. */	
	uint32 FogColor;			/* fog colorvalue */
	uint32 reserved02[0x0039];
	struct {
		float ScreenX;			/* X */
		float ScreenY;			/* Y */
		float ScreenZ;			/* depth */
		float RWH;				/* eyeM */
		uint32 Color;			/* b24-31 Alpha, b16-23 Red, b8-15 Green, b0-7 Blue */
		uint32 Specular;		/* b24-31 Fog, b16-23 Red, b8-15 Green, b0-7 Blue */
		float TU;				/* texture S */
		float TV;				/* texture T */
	} TLVertex[0x10];			/* command can handle upto 16 textured, lit(?) vertexes */
	uint32 TLVDrawPrim[0x40];	/* b20-31 is I5, b16-19 is I4, b12-15 is I3,
								 * b8-11 is I2, 4-7 is I1, b0-3 is I0:
								 * Ix is a TLVertex[Ix].
								 * So: define your (single) texture, define your
								 * vertexes, and then program TLVDrawPrim with the
								 * order to draw them.
								 * You can draw primitives consisting of sets of upto
								 * 6 out of 16 defined vertexes this way; and you can
								 * draw 64 sets maximum. */
} cmd_nv4_dx5_texture_triangle;	/* nv10_dx5_texture_triangle is identical */

typedef struct {
	uint32 reserved00[0x0004];
	uint16 FifoFree;			/* little endian (FIFO internal register) */
	uint16 Nop;					/* little endian (FIFO internal register) */
	uint32 reserved01[0x00bd];	/* fixme? there's more here that's not used apparantly */
	uint32 Pitch;				/* b16-31 is Z-buffer, b0-15 is colorbuffer pitch */
	uint32 SetOffsetColor;		/* b0-31 is colorbuffer (renderbuffer) offset */
	uint32 SetOffsetZeta;		/* b0-31 is Z-buffer (zeta buffer) offset */
} cmd_nv4_context_surfaces_argb_zs; /* nv10_context_surfaces_argb_zs is identical */

//fixme: let's loose this one if possible as we don't support NV3 anyway ...
//		 (checkout if we can re-setup 2D surfaces (after using 3D) without this command)
typedef struct {
	uint32 reserved00[0x0004];
	uint16 FifoFree;			/* little endian (FIFO internal register) */
	uint16 Nop;					/* little endian (FIFO internal register) */
	uint32 reserved01[0x00bd];	/* fixme? there's more here that's not used apparantly */
	uint32 Pitch;				/* b0-15 is buffer pitch */
	uint32 SetOffset;			/* b0-22 is buffer offset (so just 1st 8Mb adressable!) */
} cmd_nv3_surface_x;			/* x = 0, 1, 2 or 3: selects buffer to be setup;
								 * 0,1 is 2D surfaces, 2,3 is 3D surfaces */

//fixme: fill these out...
typedef struct {
} cmd_nv_render_d3d0_triangle_zeta;

typedef struct {
} cmd_nv4_dx6_multi_texture_triangle;/* nv10_dx6_multi_texture_triangle is identical? */

#endif
