#include <stdio.h>
#include "DirectDriver.h"
#include <string.h>

#ifndef ABS
	#define ABS(a)	(a<0)?a*-1:a
#endif

OBAppServerScreen::OBAppServerScreen(status_t *error)
	: BDirectWindow(BRect(100,60,740,540),"OBOS App Server, P3",B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	redraw=true;
}

OBAppServerScreen::~OBAppServerScreen(void)
{
}

void OBAppServerScreen::MessageReceived(BMessage *msg)
{
	msg->PrintToStream();
	switch(msg->what)
	{
		case B_KEY_DOWN:
//			Disconnect();
			QuitRequested();
			break;
		default:
			BDirectWindow::MessageReceived(msg);
			break;
	}
}

bool OBAppServerScreen::QuitRequested(void)
{
	redraw=false;
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

int32 OBAppServerScreen::Draw(void *data)
{
	OBAppServerScreen *screen=(OBAppServerScreen *)data;
	for(;;)
	{
		if(screen->connected && screen->redraw)
		{
			// Draw a sample background
			screen->redraw=false;
		}
	}
	return 1;
}

void OBAppServerScreen::DirectConnected(direct_buffer_info *dbinfo)
{
/*	if(ok==true)
	{
		connected=true;
		gcinfo=CardInfo();
		for(int8 i=0;i<48;i++)
			gchooks[i]=CardHookAt(i);
	}
	else
		connected=false;
*/
}

DirectDriver::DirectDriver(void)
{
//	printf("DirectDriver()\n");
//	old_space=B_32_BIT_800x600;
}

DirectDriver::~DirectDriver(void)
{
	printf("~DirectDriver()\n");
	winscreen->Lock();
	winscreen->Close();
}

void DirectDriver::Initialize(void)
{
/*	printf("DirectDriver::Initialize()\n");
	status_t stat;
	winscreen=new OBAppServerScreen(&stat);
	if(stat==B_OK)
		winscreen->Show();
	else
		delete winscreen;
*/
}

void DirectDriver::SafeMode(void)
{
/*	printf("DirectDriver::SafeMode()\n");
//	SetScreen(B_8_BIT_640x480);
	SetScreen(B_32_BIT_800x600);
	Reset();
*/
}

void DirectDriver::Reset(void)
{
/*	printf("DirectDriver::Reset()\n");
	if(winscreen->connected==true)
		Clear(51,102,160);
*/
}

void DirectDriver::SetScreen(uint32 space)
{
//	printf("DirectDriver::SetScreen(%lu)\n",space);
//	winscreen->SetSpace(space);
}

void DirectDriver::Clear(uint8 red, uint8 green, uint8 blue)
{
/*	// This simply clears the entire screen, utilizing a string and
	// memcpy to speed things up in a minor way
	printf("DirectDriver::Clear(%d,%d,%d)\n",red,green,blue);

	if(winscreen->connected==false)
		return;

	int i,imax,
		length=winscreen->gcinfo->bytes_per_row;
	int8 fillstring[length];


	switch(winscreen->gcinfo->bits_per_pixel)
	{
		case 32:
		{
			int32 color,*index32;
			
			// order is going to be BGRA or ARGB
			if(winscreen->gcinfo->rgba_order[0]=='B')
			{
				// Create the number to quickly create the string

				// BBGG RRAA in RAM => 0xRRAA BBGG
				color= (int32(red) << 24) + 0xFF0000L + (int32(blue) << 8) + green;
			}
			else
			{
				// Create the number to quickly create the string

				// AARR GGBB in RAM
				color= 0xFF000000L + (int32(red) << 16) + (int32(green) << 8) + blue;
			}
			
			// fill the string with the appropriate values
			index32=(int32 *)fillstring;
			imax=winscreen->gcinfo->width;

			for(i=0;i<imax;i++)
				index32[i]=color;

			break;
		}
*/
/*		case 16:
		{
			int16 color, *index16;

			if(winscreen->gcinfo->rgba_order[0]=='B')
			{
				// Little-endian			
				// G[2:0],B[4:0]  R[4:0],G[5:3]
			
				// Ick. Gotta love all that shifting and masking. :(
				color=	(int16((green & 7)) << 13) +
						(int16((blue & 31)) << 8) +
						(int16((red & 31)) << 4) +
						(int16(green & 56));
			}
			else
			{
				// Big-endian
				// R[4:0],G[5:3]  G[2:0],B[4:0]
				color=	(int16(red & 31) << 11) +
						(int16(green & 56) << 8) +
						(int16(green & 3) << 5) +
						(blue & 31);
			}
			break;
		}
		case 15:
		{

			//	RGBA_15:
			// G[2:0],B[4:0]   A[0],R[4:0],G[4:3]

			//	RGBA_15_BIG:
			//	A[0],R[4:0],G[4:3]  G[2:0],B[4:0]
			break;
		}
*/
/*		case 8:
		{
			// This is both the easiest and hardest:
			// We fill using memset(), but we have to find the closest match to the
			// color we've been given. Yuck.
			
			// Find the closest match by comparing the differences in values for each
			// color. This function was not intended to be called rapidly, so speed is
			// not much of an issue at this point.
			int8 closest=0;
			int16 closest_delta=765, delta;	//	(255 * 3)

			// Get the current palette
			rgb_color *list=winscreen->ColorList();
			for(i=0;i<256;i++)
			{
				delta=ABS(list->red-red)+ABS(list->green-green)+ABS(list->blue-blue);
				if(delta<closest_delta)
				{
					closest=i;
					closest_delta=delta;
				}
			}
			
			// Theoretically, we now have the closest match, so simply fill the
			// frame buffer with the index
			memset(winscreen->gcinfo->frame_buffer,closest,
					winscreen->gcinfo->bytes_per_row * winscreen->gcinfo->height);

			// Nothing left to do!
			return;
			break;
		}
		default:
		{
			printf("Unsupported bit depth %d in DirectDriver::Clear(r,g,b)!!\n",winscreen->gcinfo->bits_per_pixel);
			return;
		}
	}
	imax=winscreen->gcinfo->height;
	
	// a pointer to increment for the screen copy. Faster than doing a multiply
	// each iteration
	int8 *pbuffer=(int8*)winscreen->gcinfo->frame_buffer;
	for(i=0;i<imax;i++)
	{
		memcpy(pbuffer,fillstring,length);
		pbuffer+=length;
	}
*/
}