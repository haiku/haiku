#include "ServerBitmap.h"
#include "Desktop.h"

ServerBitmap::ServerBitmap(BRect rect,color_space cspace,int32 BytesPerLine=0)
{
	width=rect.IntegerWidth()+1;
	height=rect.IntegerHeight()+1;
	colorspace=cspace;
	is_vram=false;

	// Big convoluted mess just to handle every color space and dword align
	// the buffer	
	switch(cspace)
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

		// 24-bit=32bit with extra 8 bits ignored
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
			break;
		}
		case B_NO_COLOR_SPACE:
			break;
	}
	buffer=new uint8[bytesperline*height];
	driver=get_gfxdriver();;
}

ServerBitmap::~ServerBitmap(void)
{
	delete buffer;
}
