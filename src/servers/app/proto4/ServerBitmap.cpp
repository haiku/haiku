/*
	ServerBitmap.cpp
		Bitmap class which is intended to provide an easy way to package all the
		data associated with a picture. It's very low-level, so there's still
		plenty to do when coding with 'em. Also, they're used to access draw on the
		frame buffer in a relatively easy way, although it is currently unclear
		whether this functionality will even be necessary.
*/

// These three includes for ServerBitmap(const char *path_to_image_file) only
#include <TranslationUtils.h>
#include <string.h>
#include <Bitmap.h>

#include <assert.h>
#include "ServerBitmap.h"
#include "Desktop.h"

ServerBitmap::ServerBitmap(BRect rect,color_space space,int32 BytesPerLine=0)
{
	width=rect.IntegerWidth()+1;
	height=rect.IntegerHeight()+1;
	cspace=space;
	is_vram=false;
	is_area=false;
	areaid=B_ERROR;

	HandleSpace(space, BytesPerLine);
	buffer=new uint8[bytesperline*height];
	driver=get_gfxdriver();
}

ServerBitmap::ServerBitmap(int32 w,int32 h,color_space space,int32 BytesPerLine=0)
{
	width=w;
	height=h;
	cspace=space;
	is_vram=false;
	is_area=false;
	areaid=B_ERROR;

	HandleSpace(space, BytesPerLine);
	buffer=new uint8[bytesperline*height];
	driver=get_gfxdriver();
}

ServerBitmap::ServerBitmap(int32 w,int32 h,color_space space, area_id areaID, int32 BytesPerLine=0)
{
	// This version is used for holding BBitmaps as one of the undocumented BBitmap
	// constructors allows - it creates an area on the server's side.
	width=w;
	height=h;
	cspace=space;
	is_vram=false;
	is_area=true;

	HandleSpace(space, BytesPerLine);
	area_info ainfo;
	areaid=areaID;
	get_area_info(areaid, &ainfo);
	buffer=(uint8*)ainfo.address;
	driver=get_gfxdriver();
}

ServerBitmap::ServerBitmap(void)
{
	// This version is obviously intended for use as an easy-access class for the
	// frame buffer. AtheOS does it and it's small, so why not? Worst case, this
	// can disappear if not necessary.
	is_vram=true;
	is_area=false;
	areaid=B_ERROR;
	UpdateSettings();
}

// This particular version is not intended for use except for testing purposes.
// It loads a BBitmap and copies it to the ServerBitmap. Probably will disappear
// when the *real* server is written
ServerBitmap::ServerBitmap(const char *path)
{
	BBitmap *bmp=BTranslationUtils::GetBitmap(path);
	assert(bmp!=NULL);

	if(bmp==NULL)
	{
		width=0;
		height=0;
		cspace=B_NO_COLOR_SPACE;
		bytesperline=0;
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

ServerBitmap::~ServerBitmap(void)
{
	if(!is_vram)
		delete buffer;
}

void ServerBitmap::UpdateSettings(void)
{
	// This is only used when the object is pointing to the frame buffer
	driver=get_gfxdriver();
	bpp=driver->GetDepth();
	width=driver->GetWidth();
	height=driver->GetHeight();
}

void ServerBitmap::SetBuffer(void *buffer)
{
	buffer=(uint8 *)buffer;
}

uint8 *ServerBitmap::Buffer(void)
{
	return buffer;
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
}
