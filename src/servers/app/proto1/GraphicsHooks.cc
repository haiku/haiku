#include <Rect.h>
#include <Point.h>
#include "GraphicsHooks.h"
//#include "AppServer.h"		// for the framebuffer pointer

// Structs necessary for our hook functions
#include <GraphicsDefs.h>

int32 define_cursor(uchar *xormask, uchar *andmask, int32 width,
		int32 height, int32 hotx, int32 hoty)
{
	return 0;
}

int32 move_cursor(int32 screenx, int32 screeny)
{
	return 0;
}

int32 show_cursor(bool flag)
{
	return 0;
}

int32 draw_line_with_8_bit_depth(int32 startx, int32 endx,
		int32 starty, int32 endy, uint8 colorindex, bool cliptorect,
		int16 clipleft, int16 cliptop, int16 clipright, int16 clipbottom)
{
	if(cliptorect)
	{	// Do clipping before we start if we're told to
		BRect rect(clipleft,cliptop,clipright,clipbottom);
		BPoint start(startx,starty),end(endx,endy);
		start.ConstrainTo(rect);
		end.ConstrainTo(rect);
	}
	
	return B_OK;
}

int32 draw_line_with_32_bit_depth(int32 startx, int32 endx,
		int32 starty, int32 endy, uint32 color, bool cliptorect,
		int16 clipleft, int16 cliptop, int16 clipright, int16 clipbottom)
{
	if(cliptorect)
	{	// Do clipping before we start if we're told to
		BRect rect(clipleft,cliptop,clipright,clipbottom);
		BPoint start(startx,starty),end(endx,endy);
		start.ConstrainTo(rect);
		end.ConstrainTo(rect);
	}
	
	return 0;
}

int32 draw_rect_with_8_bit_depth(int32 left, int32 top, int32 right,
		int32 bottom, uint8 colorindex)
{
	return 0;
}

int32 draw_rect_with_32_bit_depth(int32 left, int32 top, int32 right,
		int32 bottom, uint32 color)
{
	return 0;
}

int32 blit(int32 sourcex, int32 sourcey, int32 destinationx,
		int32 destinationy, int32 width, int32 height)
{
	return 0;
}

int32 draw_array_with_8_bit_depth(indexed_color_line *array, int32 numitems,
		bool cliptorect, int16 clipleft, int16 cliptop, 
		int16 clipright, int16 clipbottom)
{
	return 0;
}

int32 draw_array_with_32_bit_depth(rgb_color_line *array, int32 numitems,
		bool cliptorect, int16 clipleft, int16 cliptop, 
		int16 clipright, int16 clipbottom)
{
	return 0;
}

int32 sync(void)
{
	return 0;
}

int32 invert_rect(int32 left, int32 top, int32 right, int32 bottom)
{
	return 0;
}

int32 draw_line_with_16_bit_depth(int32 startx, int32 endx,
		int32 starty, int32 endy, uint16 color, bool cliptorect,
		int16 clipleft, int16 cliptop, int16 clipright, int16 clipbottom)
{
	return 0;
}

int32 draw_rect_with_16_bit_depth(int32 left, int32 top, int32 right,
		int32 bottom, uint16 color)
{
	return 0;
}

// hook functions not officially-defined, but which should exist
int32 draw_array_with_16_bit_depth(high_color_line *array, int32 numitems,
		bool cliptorect, int16 clipleft, int16 cliptop, 
		int16 clipright, int16 clipbottom)
{
	return 0;
}

int32 draw_frect_with_8_bit_depth(int32 left, int32 top, int32 right,
		int32 bottom, uint8 colorindex)
{
	return 0;
}

int32 draw_frect_with_16_bit_depth(int32 left, int32 top, int32 right,
		int32 bottom, uint16 color)
{
	return 0;
}

int32 draw_frect_with_32_bit_depth(int32 left, int32 top, int32 right,
		int32 bottom, uint32 color)
{
	return 0;
}

/*
rgb_color GetPixel_32(BPoint pt)
{
//	pt.x = floor(pt.x / zoom);
//	pt.y = floor(pt.y / zoom);
	rgb_color color;

	uint32 pos_bits=uint32 ( (p_active_layer->bitmap->BytesPerRow()*pt.y) + (pt.x*4) );
	uint8	*s_bits=(uint8*) p_active_layer->bitmap->Bits() + pos_bits;
	
	color.blue=*s_bits;
	s_bits++;
	color.green=*s_bits;
	s_bits++;
	color.red=*s_bits;
	s_bits++;
	color.alpha=*s_bits;
	return color;
}

void SetPixel_32(BPoint where, rgb_color color)
{	
	// Make some quick-access pointers
	BBitmap *work_bmp;
	work_bmp=p_active_layer->bitmap;

	uint8 *bmpdata=(uint8 *)work_bmp->Bits();

	// Check bounds
	where.ConstrainTo(work_bmp->Bounds());
	
	// Calculate offset & jump
	uint32 bitoffset;
	bitoffset= uint32 ((where.x*4)+(work_bmp->BytesPerRow()*where.y)); //only counts there - it is similar on all lines
	bmpdata += bitoffset;
	
	// Assign color data
	*bmpdata=color.blue;
		bmpdata++;
	*bmpdata=color.green;
		bmpdata++;
	*bmpdata=color.red;
		bmpdata++;
	*bmpdata=color.alpha;  
}
*/