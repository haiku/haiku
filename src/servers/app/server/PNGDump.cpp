#include <Rect.h>
#include <Bitmap.h>
#include <stdio.h>
#include <unistd.h>
#include <png.h>
#include <ColorUtils.h>
#include "PNGDump.h"

//#define DEBUG_PNGDUMP

void SaveToPNG(const char *filename, const BRect &bounds, color_space space, 
	const void *bits, const int32 &bitslength, const int32 bytesperrow)
{
#ifdef DEBUG_PNGDUMP
printf("SaveToPNG: %s ",filename);
bounds.PrintToStream();
#endif
	FILE *fp;
	png_structp png_ptr;
	png_infop info_ptr;
	
	fp=fopen(filename, "wb");
	if(fp==NULL)
	{
#ifdef DEBUG_PNGDUMP
printf("Couldn't open file\n");
#endif
		return;
	}
	
	png_ptr=png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	
	if(png_ptr==NULL)
	{
#ifdef DEBUG_PNGDUMP
printf("Couldn't create write struct\n");
#endif
		fclose(fp);
		return;
	}
	
	info_ptr=png_create_info_struct(png_ptr);
	if(info_ptr==NULL)
	{
#ifdef DEBUG_PNGDUMP
printf("Couldn't create info struct\n");
#endif
		fclose(fp);
		png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
		return;
	}
	
	if(setjmp(png_ptr->jmpbuf))
	{
#ifdef DEBUG_PNGDUMP
printf("Couldn't set jump\n");
#endif
		fclose(fp);
		png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
		return;
	}
	
	png_init_io(png_ptr, fp);
	
	png_set_compression_level(png_ptr,Z_NO_COMPRESSION);
	
   	png_set_IHDR(png_ptr, info_ptr, bounds.IntegerWidth(), bounds.IntegerHeight(), 8, PNG_COLOR_TYPE_RGB,
	PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	
	png_write_info(png_ptr, info_ptr);
	
	png_byte *row_pointers[bounds.IntegerHeight()];
	png_byte *index=(png_byte*)bits;
	for(int32 i=0;i<bounds.IntegerHeight();i++)
	{
		row_pointers[i]=index;
		index+=bytesperrow;
	}
	png_write_image(png_ptr, row_pointers);
	png_write_end(png_ptr, info_ptr);
	
	png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	fclose(fp);
}
