/*
	DriverDriver:
		Replacement class for the ViewDriver which utilizes a BDirectWindow for ease
		of testing without requiring a second video card for SecondDriver and also
		without the limitations (mostly speed) of ViewDriver.
		
		Note that unlike ViewDriver, this windowed graphics module does NOT emulate
		the Input Server. The Router input server filter is required for use.
		
		The concept is a combination of both SecondDriver and ViewDriver: utilize
		PortLink messaging (for better speed than BMessages) and draw directly to the
		frame buffer.
		
		Components:		2 classes, DDriverWin and DirectDriver
		
		DirectDriver - a wrapper class which enqueues draw requests
		DDirectWin - does most of the work.
*/

#define DEBUG_DRIVER_MODULE
#include <View.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "PortLink.h"
#include "ServerProtocol.h"
#include "DirectDriver.h"
#include "ServerBitmap.h"

// Message defines for internal use only
enum
{
DDWIN_SHUTDOWN=100,
DDWIN_CLEAR,
DDWIN_SAFEMODE,
DDWIN_RESET,
DDWIN_SETSCREEN,

DDWIN_FILLARC,
DDWIN_STROKEARC,
DDWIN_FILLRECT,
DDWIN_STROKERECT,
DDWIN_FILLROUNDRECT,
DDWIN_STROKEROUNDRECT,
DDWIN_FILLELLIPSE,
DDWIN_STROKEELLIPSE,
DDWIN_FILLBEZIER,
DDWIN_STROKEBEZIER,
DDWIN_FILLTRIANGLE,
DDWIN_STROKETRIANGLE,
DDWIN_STROKELINE,
DDWIN_DRAWSTRING,

DDWIN_SETPENSIZE,
DDWIN_MOVEPENTO,
DDWIN_SHOWCURSOR,
DDWIN_HIDECURSOR,
DDWIN_OBSCURECURSOR,
DDWIN_MOVECURSOR,
DDWIN_SETCURSOR,
DDWIN_SETHIGHCOLOR,
DDWIN_SETLOWCOLOR
};

DDriverWin::DDriverWin(BRect frame)
	: BDirectWindow(frame,"OBOS App Server, P6",B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	msgport=create_port(100,"DDWinPort");
	fConnected=false;
	fConnectionDisabled=false;
	locker=new BLocker();
	fClipList=NULL;
	fNumClipRects=0;

	AddChild(new BView(Bounds(),"BView",B_FOLLOW_ALL,0));
	if(ChildAt(0))
		ChildAt(0)->SetViewColor(B_TRANSPARENT_32_BIT);
	
	if (!SupportsWindowMode())
		SetFullScreen(true);
	framebuffer=NULL;
	
	fDirty=true;
	fDrawThreadID=spawn_thread(DrawingThread, "drawing_thread",B_NORMAL_PRIORITY, (void *) this);
	resume_thread(fDrawThreadID);
	Show();
}

DDriverWin::~DDriverWin(void)
{
	int32 result;
	
	// In case the update thread is waiting for a message (very likely),
	// give it a message to unblock it
	fConnectionDisabled=true;
	write_port(msgport,DDWIN_SHUTDOWN,NULL,0);
	Hide();
	Sync();
	wait_for_thread(fDrawThreadID, &result);
	free(fClipList);
	if(framebuffer)
		delete framebuffer;
	delete locker;
}

void DDriverWin::DirectConnected(direct_buffer_info *info)
{
	if (!fConnected && fConnectionDisabled)
		return;
	
	locker->Lock();
	
	switch(info->buffer_state & B_DIRECT_MODE_MASK)
	{
		case B_DIRECT_START:
		{
			fConnected=true;
			// fall through on purpose
		}	
		case B_DIRECT_MODIFY:
		{
			// Get clipping information
			if (fClipList)
			{
				free(fClipList);
				fClipList=NULL;
			}
			fNumClipRects=info->clip_list_count;
			fClipList=(clipping_rect *)
			malloc(fNumClipRects*sizeof(clipping_rect));
			if (fClipList)
			{
				memcpy(fClipList, info->clip_list,
				fNumClipRects*sizeof(clipping_rect));
				fBits=(uint8 *) info->bits;
				fRowBytes=info->bytes_per_row;
				fFormat=info->pixel_format;
				fBounds=info->window_bounds;
				fDirty=true;
			}
			framebuffer=new ServerBitmap(Bounds(),info->pixel_format);
			break;
		}
		case B_DIRECT_STOP:
		{
			fConnected=false;
			delete framebuffer;
			framebuffer=NULL;
			break;
		}
	}
	locker->Unlock();
}

int32 DDriverWin::DrawingThread(void *data)
{
	DDriverWin *w=(DDriverWin *)data;
//	port_id dport=w->msgport;
	
	while (!w->fConnectionDisabled)
	{
		w->locker->Lock();
		if (w->fConnected)
		{
			if ( w->framebuffer && (w->fFormat==B_RGB32 || w->fFormat==B_RGBA32) && w->fDirty)
//			if ( w->fFormat==B_CMAP8 && w->fDirty)
			{
				int32 y;
				int32 width, height;
				int32 adder,bmpadder;
				uint8 *p_fbuffer, *p_bitmap;
				clipping_rect *clip;
				int32 i;
				
				adder=w->fRowBytes;
				bmpadder=w->framebuffer->bytesperline;
				
				for (i=0; i<w->fNumClipRects; i++)
				{
					clip=&(w->fClipList[i]);
					width=((clip->right-clip->left)+1)*4;
					height=(clip->bottom-clip->top)+1;
					p_fbuffer=w->fBits+(clip->top*w->fRowBytes)+(clip->left*4);
					p_bitmap=w->framebuffer->Buffer()+(clip->top*w->framebuffer->bytesperline)+(clip->left*4);
					y=0;
					while (y < height)
					{
printf("Copy %ld bytes from %p to %p\n",width,p_bitmap, p_fbuffer);
						memcpy(p_fbuffer, p_bitmap, width);
						p_fbuffer+=adder;
						p_bitmap+=bmpadder;
						y++;
					}
				}
			}
			w->fDirty=false;
		}
		w->locker->Unlock();

		// Use BWindow or BView APIs here if you want
		
		// process messages from port here by calling member function DispatchMessage
		
		snooze(16000);
	}
	return B_OK;
}

bool DDriverWin::QuitRequested(void)
{
	port_id serverport=find_port(SERVER_PORT_NAME);
	if(serverport!=B_NAME_NOT_FOUND)
	{
		write_port(serverport,B_QUIT_REQUESTED,NULL,0);
	}
	return true;
}

void DDriverWin::WindowActivated(bool active)
{
	// This is just to hide the regular system cursor so we can see our own
	if(active)
		be_app->HideCursor();
	else
		be_app->ShowCursor();
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

void DDriverWin::SafeMode(void)
{
}

void DDriverWin::Reset(void)
{
}

void DDriverWin::SetScreen(uint32 space)
{
}

void DDriverWin::Clear(clipping_rect *rect,uint8 red,uint8 green,uint8 blue)
{
	int32 width=((rect->right-rect->left)+1)*4;
	int32 height=(rect->bottom-rect->top)+1;
	uint8 *p=fBits+(rect->top*fRowBytes)+(rect->left*4);
	int32 y=0;

	// Loop to clear each row
	while (y < height)
	{
		// loop to clear 1 row belongs here
		memset(p, 0x00, width);

		y++;
		p+=fRowBytes;
	}
}

DirectDriver::DirectDriver(void)
{
	driverwin=new DDriverWin(BRect(100,60,740,540));
	hide_cursor=0;
	drawlink=new PortLink(driverwin->msgport);
}

DirectDriver::~DirectDriver(void)
{
	if(is_initialized)
	{
		driverwin->Lock();
		driverwin->Quit();
	}
	delete drawlink;
}

void DirectDriver::Initialize(void)
{
	locker->Lock();
	is_initialized=true;
//	SetPenSize(1.0);
//	MovePenTo(BPoint(0,0));
	locker->Unlock();
}

bool DirectDriver::IsInitialized(void)
{
	return is_initialized;
}

void DirectDriver::Shutdown(void)
{
	locker->Lock();
	is_initialized=false;
	locker->Unlock();
}

void DirectDriver::SafeMode(void)
{
}

void DirectDriver::Reset(void)
{
}

void DirectDriver::Clear(uint8 red,uint8 green,uint8 blue)
{
}

void DirectDriver::Clear(rgb_color col)
{
}

