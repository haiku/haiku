/*
	ServerBitmap.cpp
		Bitmap class which is intended to provide an easy way to package all the
		data associated with a picture. It's very low-level, so there's still
		plenty to do when coding with 'em. Also, they can be used to access draw on the
		frame buffer in a relatively easy way, although it is currently unclear
		whether this functionality will even be necessary.
*/

// These three includes for ServerBitmap(const char *path_to_image_file) only
#include <TranslationUtils.h>
#include <string.h>
#include <Bitmap.h>

#include "ServerBitmap.h"
#include "Desktop.h"
#include "DisplayDriver.h"

//#define DEBUG_BITMAP

#ifdef DEBUG_BITMAP
#include <stdio.h>
#endif

long pow2(int power)
{
	if(power==0)
		return 1;
	return long(2 << (power-1));
}

ServerBitmap::ServerBitmap(BRect rect,color_space space,int32 BytesPerLine=0)
{
	is_initialized=true;
	width=rect.IntegerWidth()+1;
	height=rect.IntegerHeight()+1;
	cspace=space;
	areaid=B_ERROR;

	HandleSpace(space, BytesPerLine);
	buffer=new uint8[bytesperline*height];
}

ServerBitmap::ServerBitmap(int32 w,int32 h,color_space space,int32 BytesPerLine=0)
{
	is_initialized=true;
	width=w;
	height=h;
	cspace=space;
	areaid=B_ERROR;

	HandleSpace(space, BytesPerLine);
	buffer=new uint8[bytesperline*height];
}

// This loads a bitmap from disk, given a path
ServerBitmap::ServerBitmap(const char *path)
{
	is_initialized=true;
	if(!path)
	{
		width=0;
		height=0;
		cspace=B_NO_COLOR_SPACE;
		bytesperline=0;
		is_initialized=false;
		buffer=NULL;
		return;
	}

	BBitmap *bmp=BTranslationUtils::GetBitmap(path);

	if(bmp==NULL)
	{
		width=0;
		height=0;
		cspace=B_NO_COLOR_SPACE;
		bytesperline=0;
		is_initialized=false;
		buffer=NULL;
	}
	else
	{
		width=bmp->Bounds().IntegerWidth();
		height=bmp->Bounds().IntegerHeight();
		cspace=bmp->ColorSpace();
		bytesperline=bmp->BytesPerRow();
		buffer=new uint8[bmp->BitsLength()];
		memcpy(buffer,(void *)bmp->Bits(),bmp->BitsLength());
	}
}

// Load a bitmap from a resource file
ServerBitmap::ServerBitmap(uint32 type,int32 id)
{
	is_initialized=true;
	BBitmap *bmp=BTranslationUtils::GetBitmap(type,id);

	if(bmp==NULL)
	{
		width=0;
		height=0;
		cspace=B_NO_COLOR_SPACE;
		bytesperline=0;
		is_initialized=false;
		buffer=NULL;
	}
	else
	{
		width=bmp->Bounds().IntegerWidth();
		height=bmp->Bounds().IntegerHeight();
		cspace=bmp->ColorSpace();
		bytesperline=bmp->BytesPerRow();
		buffer=new uint8[bmp->BitsLength()];
		memcpy(buffer,(void *)bmp->Bits(),bmp->BitsLength());
	}
}

ServerBitmap::ServerBitmap(uint32 type, const char *name)
{
	is_initialized=true;
	BBitmap *bmp=BTranslationUtils::GetBitmap(type,name);

	if(bmp==NULL)
	{
		width=0;
		height=0;
		cspace=B_NO_COLOR_SPACE;
		bytesperline=0;
		is_initialized=false;
		buffer=NULL;
	}
	else
	{
		width=bmp->Bounds().IntegerWidth();
		height=bmp->Bounds().IntegerHeight();
		cspace=bmp->ColorSpace();
		bytesperline=bmp->BytesPerRow();
		buffer=new uint8[bmp->BitsLength()];
		memcpy(buffer,(void *)bmp->Bits(),bmp->BitsLength());
	}
}

ServerBitmap::ServerBitmap(ServerBitmap *bitmap)
{
	is_initialized=true;
	width=bitmap->width;
	height=bitmap->height;
	cspace=bitmap->cspace;
	areaid=B_ERROR;

	HandleSpace(bitmap->cspace, bitmap->bytesperline);
	buffer=new uint8[bytesperline*height];
	memcpy(buffer, bitmap->buffer, bytesperline * height);
}

ServerBitmap::ServerBitmap(int8 *data)
{
	// 68-byte array used in R5 for holding cursors.
	// This API has serious problems and should be deprecated(but supported) in R2

	// Now that we have all the setup, we're going to map (for now) the cursor
	// to RGBA32. Eventually, there will be support for 16 and 8-bit depths
	is_initialized=true;
	if(data)
	{	
		uint32 black=0xFF000000,
			white=0xFFFFFFFF,
			*bmppos;
		uint16	*cursorpos, *maskpos,cursorflip, maskflip,
				cursorval, maskval;
		uint8 	i,j;
	
		cursorpos=(uint16*)(data+4);
		maskpos=(uint16*)(data+36);
		
		buffer=new uint8[1024];	// 16x16x4 - RGBA32 space
		width=16;
		height=16;
		bytesperline=64;
		
		// for each row in the cursor data
		for(j=0;j<16;j++)
		{
			bmppos=(uint32*)(buffer+ (j*64) );
	
			// On intel, our bytes end up swapped, so we must swap them back
			cursorflip=(cursorpos[j] & 0xFF) << 8;
			cursorflip |= (cursorpos[j] & 0xFF00) >> 8;
			maskflip=(maskpos[j] & 0xFF) << 8;
			maskflip |= (maskpos[j] & 0xFF00) >> 8;
	
			// for each column in each row of cursor data
			for(i=0;i<16;i++)
			{
				// Get the values and dump them to the bitmap
				cursorval=cursorflip & pow2(15-i);
				maskval=maskflip & pow2(15-i);
				bmppos[i]=((cursorval!=0)?black:white) & ((maskval>0)?0xFFFFFFFF:0x00FFFFFF);
			}
		}
	}
	else
	{
		buffer=NULL;
		width=0;
		height=0;
		bytesperline=0;
	}
	
#ifdef DEBUG_SERVER_CURSOR
	PrintToStream();
#endif
}

ServerBitmap::~ServerBitmap(void)
{
	if(buffer)
		delete buffer;
}

void ServerBitmap::UpdateSettings(void)
{
	// This is only used when the object is pointing to the frame buffer
	bpp=driver->GetDepth();
	width=driver->GetWidth();
	height=driver->GetHeight();
}

uint8 *ServerBitmap::Bits(void)
{
	return buffer;
}

uint32 ServerBitmap::BitsLength(void)
{
	return (uint32)(bytesperline*height);
}

void ServerBitmap::HandleSpace(color_space space, int32 BytesPerLine)
{
	// Function written to handle color space setup which is required
	// by the two "normal" constructors

	// Big convoluted mess just to handle every color space and dword align
	// the buffer	
	switch(space)
	{
		// Buffer is dword-aligned, so nothing need be done
		// aside from allocate the memory
		case B_RGB32:
		case B_RGBA32:
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_UVL32:
		case B_UVLA32:
		case B_LAB32:
		case B_LABA32:
		case B_HSI32:
		case B_HSIA32:
		case B_HSV32:
		case B_HSVA32:
		case B_HLS32:
		case B_HLSA32:
		case B_CMY32:
		case B_CMYA32:
		case B_CMYK32:

		// 24-bit = 32-bit with extra 8 bits ignored
		case B_RGB24_BIG:
		case B_RGB24:
		case B_LAB24:
		case B_UVL24:
		case B_HSI24:
		case B_HSV24:
		case B_HLS24:
		case B_CMY24:
		{
			if(BytesPerLine<(width*4))
				bytesperline=width*4;
			else
				bytesperline=BytesPerLine;
			bpp=32;
			break;
		}
		// Calculate size and dword-align

		// 1-bit
		case B_GRAY1:
		{
			int32 numbytes=width>>3;
			if((width % 8) != 0)
				numbytes++;
			if(BytesPerLine<numbytes)
			{
				for(int8 i=0;i<4;i++)
				{
					if( (numbytes+i)%4==0)
					{
						bytesperline=numbytes+i;
						break;
					}
				}
			}
			else
				bytesperline=BytesPerLine;
			bpp=1;
		}		

		// 8-bit
		case B_CMAP8:
		case B_GRAY8:
		case B_YUV411:
		case B_YUV420:
		case B_YCbCr422:
		case B_YCbCr411:
		case B_YCbCr420:
		case B_YUV422:
		{
			if(BytesPerLine<width)
			{
				for(int8 i=0;i<4;i++)
				{
					if( (width+i)%4==0)
					{
						bytesperline=width+i;
						break;
					}
				}
			}
			else
				bytesperline=BytesPerLine;
			bpp=8;
			break;
		}

		// 16-bit
		case B_YUV9:
		case B_YUV12:
		case B_RGB15:
		case B_RGBA15:
		case B_RGB16:
		case B_RGB16_BIG:
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_YCbCr444:
		case B_YUV444:
		{
			if(BytesPerLine<width*2)
			{
				if( (width*2) % 4 !=0)
					bytesperline=(width+1)*2;
				else
					bytesperline=width*2;
			}
			else
				bytesperline=BytesPerLine;
			bpp=16;
			break;
		}
		case B_NO_COLOR_SPACE:
			bpp=0;
			break;
	}

#ifdef DEBUG_BITMAP
printf("ServerBitmap::HandleSpace():\n");
printf("Bytes per line: %ld\n",bytesperline);
printf("Width: %ld  Height: %ld\n",width, height);
#endif
}
