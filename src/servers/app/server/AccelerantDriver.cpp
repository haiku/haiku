//------------------------------------------------------------------------------
//	Copyright (c) 2001-2003, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		AccelerantDriver.cpp
//	Author:			Gabe Yoder <gyoder@stny.rr.com>
//	Description:		A display driver which works directly with the
//				accelerant for the graphics card.
//  
//------------------------------------------------------------------------------
#include "AccelerantDriver.h"
#include "Angle.h"
#include "FontFamily.h"
#include "ServerCursor.h"
#include "ServerBitmap.h"
#include "LayerData.h"
#include <FindDirectory.h>
#include <graphic_driver.h>
#include <malloc.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <unistd.h>


#define RUN_UNDER_R5

#define CLIP_X(a) ( (a < 0) ? 0 : ((a > mDisplayMode.virtual_width-1) ? \
			mDisplayMode.virtual_width-1 : a) )
#define CLIP_Y(a) ( (a < 0) ? 0 : ((a > mDisplayMode.virtual_height-1) ? \
			mDisplayMode.virtual_height-1 : a) )
#define CHECK_X(a) ( (a >= 0) || (a <= mDisplayMode.virtual_width-1) )
#define CHECK_Y(a) ( (a >= 0) || (a <= mDisplayMode.virtual_height-1) )

/* TODO: Add handling of draw modes */

AccLineCalc::AccLineCalc()
{
}

AccLineCalc::AccLineCalc(const BPoint &pta, const BPoint &ptb)
{
	start=pta;
	end=ptb;
	slope=(start.y-end.y)/(start.x-end.x);
	offset=start.y-(slope * start.x);
	minx = MIN(start.x,end.x);
	maxx = MAX(start.x,end.x);
	miny = MIN(start.y,end.y);
	maxy = MAX(start.y,end.y);
}

void AccLineCalc::SetPoints(const BPoint &pta, const BPoint &ptb)
{
	start=pta;
	end=ptb;
	slope=(start.y-end.y)/(start.x-end.x);
	offset=start.y-(slope * start.x);
	minx = MIN(start.x,end.x);
	maxx = MAX(start.x,end.x);
	miny = MIN(start.y,end.y);
	maxy = MAX(start.y,end.y);
}

void AccLineCalc::Swap(AccLineCalc &from)
{
	BPoint pta, ptb;
	pta = start;
	ptb = end;
	SetPoints(from.start,from.end);
	from.SetPoints(pta,ptb);
}

float AccLineCalc::GetX(float y)
{
	if (start.x == end.x)
		return start.x;
	return ( (y-offset)/slope );
}

float AccLineCalc::MinX()
{
	return minx;
}

float AccLineCalc::MaxX()
{
	return maxx;
}

float AccLineCalc::GetY(float x)
{
	if ( start.x == end.x )
		return start.y;
	return ( (slope * x) + offset );
}

float AccLineCalc::MinY()
{
	return miny;
}

float AccLineCalc::MaxY()
{
	return maxy;
}

/*!
	\brief Sets up internal variables needed by AccelerantDriver
	
*/
AccelerantDriver::AccelerantDriver(void) : DisplayDriver()
{
	cursor=NULL;
	under_cursor=NULL;
	cursorframe.Set(0,0,0,0);

	card_fd = -1;
	accelerant_image = -1;
	mode_list = NULL;
}


/*!
	\brief Deletes the heap memory used by the AccelerantDriver
	
*/
AccelerantDriver::~AccelerantDriver(void)
{
	if (cursor)
		delete cursor;
	if (under_cursor)
		delete under_cursor;
	if (mode_list)
		free(mode_list);
}

/*!
	\brief Initializes the driver object.
	\return true if successful, false if not
	
	Initialize sets up the driver for display, including the initial clearing
	of the screen. If things do not go as they should, false should be returned.
*/
bool AccelerantDriver::Initialize(void)
{
	int i;
	char signature[1024];
	char path[PATH_MAX];
	struct stat accelerant_stat;
	const static directory_which dirs[] =
	{
		B_USER_ADDONS_DIRECTORY,
		B_COMMON_ADDONS_DIRECTORY,
		B_BEOS_ADDONS_DIRECTORY
	};

	card_fd = OpenGraphicsDevice(1);
	if ( card_fd < 0 )
	{
		printf("Failed to open graphics device\n");
		return false;
	}

	if (ioctl(card_fd, B_GET_ACCELERANT_SIGNATURE, &signature, sizeof(signature)) != B_OK)
	{
		close(card_fd);
		return false;
	}
	//printf("signature %s\n",signature);

	accelerant_image = -1;
	for (i=0; i<3; i++)
	{
		if ( find_directory(dirs[i], -1, false, path, PATH_MAX) != B_OK )
			continue;
		strcat(path,"/accelerants/");
		strcat(path,signature);
		if (stat(path, &accelerant_stat) != 0)
			continue;
		accelerant_image = load_add_on(path);
		if (accelerant_image >= 0)
		{
			if ( get_image_symbol(accelerant_image,B_ACCELERANT_ENTRY_POINT,
					B_SYMBOL_TYPE_ANY,(void**)(&accelerant_hook)) != B_OK )
				return false;

			init_accelerant InitAccelerant;
			InitAccelerant = (init_accelerant)accelerant_hook(B_INIT_ACCELERANT,NULL);
			if (!InitAccelerant || (InitAccelerant(card_fd) != B_OK))
				return false;
			break;
		}
	}
	if (accelerant_image < 0)
		return false;

	accelerant_mode_count GetModeCount = (accelerant_mode_count)accelerant_hook(B_ACCELERANT_MODE_COUNT, NULL);
	if ( !GetModeCount )
		return false;
	mode_count = GetModeCount();
	if ( !mode_count )
		return false;
	get_mode_list GetModeList = (get_mode_list)accelerant_hook(B_GET_MODE_LIST, NULL);
	if ( !GetModeList )
		return false;
	mode_list = (display_mode *)calloc(sizeof(display_mode), mode_count);
	if ( !mode_list )
		return false;
	if ( GetModeList(mode_list) != B_OK )
		return false;

#ifdef RUN_UNDER_R5
	get_display_mode GetDisplayMode = (get_display_mode)accelerant_hook(B_GET_DISPLAY_MODE,NULL);
	if ( !GetDisplayMode )
		return false;
	if ( GetDisplayMode(&mDisplayMode) != B_OK )
		return false;
	_SetDepth(GetDepthFromColorspace(mDisplayMode.space));
	_SetWidth(mDisplayMode.virtual_width);
	_SetHeight(mDisplayMode.virtual_height);
	_SetMode(GetModeFromResolution(mDisplayMode.virtual_width,mDisplayMode.virtual_height,
		GetDepthFromColorspace(mDisplayMode.space)));

	memcpy(&R5DisplayMode,&mDisplayMode,sizeof(display_mode));
#else
	SetMode(B_8_BIT_640x480);
#endif

	get_frame_buffer_config GetFrameBufferConfig = (get_frame_buffer_config)accelerant_hook(B_GET_FRAME_BUFFER_CONFIG, NULL);
	if ( !GetFrameBufferConfig )
		return false;
	if ( GetFrameBufferConfig(&mFrameBufferConfig) != B_OK )
		return false;

	AcquireEngine = (acquire_engine)accelerant_hook(B_ACQUIRE_ENGINE,NULL);
	ReleaseEngine = (release_engine)accelerant_hook(B_RELEASE_ENGINE,NULL);
	accFillRect = (fill_rectangle)accelerant_hook(B_FILL_RECTANGLE,NULL);
	accInvertRect = (invert_rectangle)accelerant_hook(B_INVERT_RECTANGLE,NULL);
	accSetCursorShape = (set_cursor_shape)accelerant_hook(B_SET_CURSOR_SHAPE,NULL);
	accMoveCursor = (move_cursor)accelerant_hook(B_MOVE_CURSOR,NULL);
	accShowCursor = (show_cursor)accelerant_hook(B_SHOW_CURSOR,NULL);

	return true;
}

/*!
	\brief Shuts down the driver's video subsystem
	
	Any work done by Initialize() should be undone here. Note that Shutdown() is
	called even if Initialize() was unsuccessful.
*/
void AccelerantDriver::Shutdown(void)
{
#ifdef RUN_UNDER_R5
	set_display_mode SetDisplayMode = (set_display_mode)accelerant_hook(B_SET_DISPLAY_MODE, NULL);
	if ( SetDisplayMode )
		SetDisplayMode(&R5DisplayMode);
#endif
	uninit_accelerant UninitAccelerant = (uninit_accelerant)accelerant_hook(B_UNINIT_ACCELERANT,NULL);
	if ( UninitAccelerant )
		UninitAccelerant();
	if (accelerant_image >= 0)
		unload_add_on(accelerant_image);
	if (card_fd >= 0)
		close(card_fd);
}

/*!
	\brief Called for all BView::CopyBits calls
	\param src Source rectangle.
	\param rect Destination rectangle.
	
	Bounds checking must be done in this call. If the destination is not the same size 
	as the source, the source should be scaled to fit.
*/
void AccelerantDriver::CopyBits(BRect src, BRect dest)
{
  /* TODO: implement */
}

/*!
	\brief Called for all BView::DrawBitmap calls
	\param bmp Bitmap to be drawn. It will always be non-NULL and valid. The color 
	space is not guaranteed to match.
	\param src Source rectangle
	\param dest Destination rectangle. Source will be scaled to fit if not the same size.
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	
	Bounds checking must be done in this call.
*/
void AccelerantDriver::DrawBitmap(ServerBitmap *bmp, BRect src, BRect dest, LayerData *d)
{
  /* TODO: implement */
}

/*!
	\brief Utilizes the font engine to draw a string to the frame buffer
	\param string String to be drawn. Always non-NULL.
	\param length Number of characters in the string to draw. Always greater than 0. If greater
	than the number of characters in the string, draw the entire string.
	\param pt Point at which the baseline starts. Characters are to be drawn 1 pixel above
	this for backwards compatibility. While the point itself is guaranteed to be inside
	the frame buffers coordinate range, the clipping of each individual glyph must be
	performed by the driver itself.
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param delta Extra character padding
*/
void AccelerantDriver::DrawString(const char *string, int32 length, BPoint pt, LayerData *d, escapement_delta *edelta)
{
	if(!string || !d)
		return;

	Lock();

	pt.y--;	// because of Be's backward compatibility hack

	ServerFont *font=&(d->font);
	FontStyle *style=font->Style();

	if(!style)
	{
		Unlock();
		return;
	}

	FT_Face face;
	FT_GlyphSlot slot;
	FT_Matrix rmatrix,smatrix;
	FT_UInt glyph_index=0, previous=0;
	FT_Vector pen,delta,space,nonspace;
	int16 error=0;
	int32 strlength,i;
	Angle rotation(font->Rotation()), shear(font->Shear());

	bool antialias=( (font->Size()<18 && font->Flags()& B_DISABLE_ANTIALIASING==0)
		|| font->Flags()& B_FORCE_ANTIALIASING)?true:false;

	// Originally, I thought to do this shear checking here, but it really should be
	// done in BFont::SetShear()
	float shearangle=shear.Value();
	if(shearangle>135)
		shearangle=135;
	if(shearangle<45)
		shearangle=45;

	if(shearangle>90)
		shear=90+((180-shearangle)*2);
	else
		shear=90-(90-shearangle)*2;
	
	error=FT_New_Face(ftlib, style->GetPath(), 0, &face);
	if(error)
	{
		printf("Couldn't create face object\n");
		Unlock();
		return;
	}

	slot=face->glyph;

	bool use_kerning=FT_HAS_KERNING(face) && font->Spacing()==B_STRING_SPACING;
	
	error=FT_Set_Char_Size(face, 0,int32(font->Size())*64,72,72);
	if(error)
	{
		Unlock();
		return;
	}

	// if we do any transformation, we do a call to FT_Set_Transform() here
	
	// First, rotate
	rmatrix.xx = (FT_Fixed)( rotation.Cosine()*0x10000); 
	rmatrix.xy = (FT_Fixed)( rotation.Sine()*0x10000); 
	rmatrix.yx = (FT_Fixed)(-rotation.Sine()*0x10000); 
	rmatrix.yy = (FT_Fixed)( rotation.Cosine()*0x10000); 
	
	// Next, shear
	smatrix.xx = (FT_Fixed)(0x10000); 
	smatrix.xy = (FT_Fixed)(-shear.Cosine()*0x10000); 
	smatrix.yx = (FT_Fixed)(0); 
	smatrix.yy = (FT_Fixed)(0x10000); 

	//FT_Matrix_Multiply(&rmatrix,&smatrix);
	FT_Matrix_Multiply(&smatrix,&rmatrix);
	
	// Set up the increment value for escapement padding
	space.x=int32(d->edelta.space * rotation.Cosine()*64);
	space.y=int32(d->edelta.space * rotation.Sine()*64);
	nonspace.x=int32(d->edelta.nonspace * rotation.Cosine()*64);
	nonspace.y=int32(d->edelta.nonspace * rotation.Sine()*64);
	
	// set the pen position in 26.6 cartesian space coordinates
	pen.x=(int32)pt.x * 64;
	pen.y=(int32)pt.y * 64;
	
	slot=face->glyph;

	
	strlength=strlen(string);
	if(length<strlength)
		strlength=length;

	for(i=0;i<strlength;i++)
	{
		//FT_Set_Transform(face,&smatrix,&pen);
		FT_Set_Transform(face,&rmatrix,&pen);

		// Handle escapement padding option
		if((uint8)string[i]<=0x20)
		{
			pen.x+=space.x;
			pen.y+=space.y;
		}
		else
		{
			pen.x+=nonspace.x;
			pen.y+=nonspace.y;
		}

	
		// get kerning and move pen
		if(use_kerning && previous && glyph_index)
		{
			FT_Get_Kerning(face, previous, glyph_index,ft_kerning_default, &delta);
			pen.x+=delta.x;
			pen.y+=delta.y;
		}

		error=FT_Load_Char(face,string[i],
			((antialias)?FT_LOAD_RENDER:FT_LOAD_RENDER | FT_LOAD_MONOCHROME) );

		if(!error)
		{
		  //TODO: Replace BlitGray2RGB32 and BlitMono2RGB32
		  /*
			if(antialias)
				BlitGray2RGB32(&slot->bitmap,
					BPoint(slot->bitmap_left,pt.y-(slot->bitmap_top-pt.y)), d);
			else
				BlitMono2RGB32(&slot->bitmap,
					BPoint(slot->bitmap_left,pt.y-(slot->bitmap_top-pt.y)), d);
					*/
		}
		else
			printf("Couldn't load character %c\n", string[i]);

		// increment pen position
		pen.x+=slot->advance.x;
		pen.y+=slot->advance.y;
		previous=glyph_index;
	}
	FT_Done_Face(face);
	Unlock();
}

/*!
	\brief Called for all BView::FillArc calls
	\param r Rectangle enclosing the entire arc
	\param angle Starting angle for the arc in degrees
	\param span Span of the arc in degrees. Ending angle = angle+span.
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the arc may end up
	being clipped.
*/
void AccelerantDriver::FillArc(BRect r, float angle, float span, LayerData *d, const Pattern &pat)
{
	float xc = (r.left+r.right)/2;
	float yc = (r.top+r.bottom)/2;
	float rx = r.Width()/2;
	float ry = r.Height()/2;
	int Rx2 = ROUND(rx*rx);
	int Ry2 = ROUND(ry*ry);
	int twoRx2 = 2*Rx2;
	int twoRy2 = 2*Ry2;
	int p;
	int x=0;
	int y = (int)ry;
	int px = 0;
	int py = twoRx2 * y;
	int startx, endx;
	int starty, endy;
	int xclip, startclip, endclip;
	int startQuad, endQuad;
	bool useQuad1, useQuad2, useQuad3, useQuad4;
	bool shortspan = false;
	float oldpensize;
	BPoint center(xc,yc);

	// Watch out for bozos giving us whacko spans
	if ( (span >= 360) || (span <= -360) )
	{
	  FillEllipse(r,d,pat);
	  return;
	}

	Lock();
	PatternHandler pattern(pat);
	pattern.SetColors(d->highcolor, d->lowcolor);
	oldpensize = d->pensize;
	d->pensize = 1;

	if ( span > 0 )
	{
		startQuad = (int)(angle/90)%4+1;
		endQuad = (int)((angle+span)/90)%4+1;
		startx = ROUND(.5*r.Width()*fabs(cos(angle*M_PI/180)));
		endx = ROUND(.5*r.Width()*fabs(cos((angle+span)*M_PI/180)));
	}
	else
	{
		endQuad = (int)(angle/90)%4+1;
		startQuad = (int)((angle+span)/90)%4+1;
		endx = ROUND(.5*r.Width()*fabs(cos(angle*M_PI/180)));
		startx = ROUND(.5*r.Width()*fabs(cos((angle+span)*M_PI/180)));
	}

	starty = ROUND(ry*sqrt(1-(double)startx*startx/(rx*rx)));
	endy = ROUND(ry*sqrt(1-(double)endx*endx/(rx*rx)));

	if ( startQuad != endQuad )
	{
		useQuad1 = (endQuad > 1) && (startQuad > endQuad);
		useQuad2 = ((startQuad == 1) && (endQuad > 2)) || ((startQuad > endQuad) && (endQuad > 2));
		useQuad3 = ((startQuad < 3) && (endQuad == 4)) || ((startQuad < 3) && (endQuad < startQuad));
		useQuad4 = (startQuad < 4) && (startQuad > endQuad);
	}
	else
	{
		if ( (span < 90) && (span > -90) )
		{
			useQuad1 = false;
			useQuad2 = false;
			useQuad3 = false;
			useQuad4 = false;
			shortspan = true;
		}
		else
		{
			useQuad1 = (startQuad != 1);
			useQuad2 = (startQuad != 2);
			useQuad3 = (startQuad != 3);
			useQuad4 = (startQuad != 4);
		}
	}

	if ( useQuad1 && CHECK_Y(yc-y) && (CHECK_X(xc) || CHECK_X(xc+x)) )
		HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+x)),ROUND(yc-y),&pattern);
	if ( useQuad2 && CHECK_Y(yc-y) && (CHECK_X(xc) || CHECK_X(xc-x)) )
		HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc-x)),ROUND(yc-y),&pattern);
	if ( useQuad3 && CHECK_Y(yc+y) && (CHECK_X(xc) || CHECK_X(xc-x)) )
		HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc-x)),ROUND(yc+y),&pattern);
	if ( useQuad4 && CHECK_Y(yc+y) && (CHECK_X(xc) || CHECK_X(xc+x)) )
		HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+x)),ROUND(yc+y),&pattern);
	if ( (!shortspan && (((startQuad == 1) && (x <= startx)) || ((endQuad == 1) && (x >= endx)))) || 
	     (shortspan && (startQuad == 1) && (x <= startx) && (x >= endx)) ) 
		StrokeLine(BPoint(xc+x,yc-y),center,d,pat);
	if ( (!shortspan && (((startQuad == 2) && (x >= startx)) || ((endQuad == 2) && (x <= endx)))) || 
	     (shortspan && (startQuad == 2) && (x >= startx) && (x <= endx)) ) 
		StrokeLine(BPoint(xc-x,yc-y),center,d,pat);
	if ( (!shortspan && (((startQuad == 3) && (x <= startx)) || ((endQuad == 3) && (x >= endx)))) || 
	     (shortspan && (startQuad == 3) && (x <= startx) && (x >= endx)) ) 
		StrokeLine(BPoint(xc-x,yc+y),center,d,pat);
	if ( (!shortspan && (((startQuad == 4) && (x >= startx)) || ((endQuad == 4) && (x <= endx)))) || 
	     (shortspan && (startQuad == 4) && (x >= startx) && (x <= endx)) ) 
		StrokeLine(BPoint(xc+x,yc+y),center,d,pat);

	p = ROUND (Ry2 - (Rx2 * ry) + (.25 * Rx2));
	while (px < py)
	{
		x++;
		px += twoRy2;
		if ( p < 0 )
			p += Ry2 + px;
		else
		{
			y--;
			py -= twoRx2;
			p += Ry2 + px - py;
		}

		if ( useQuad1 && CHECK_Y(yc-y) && (CHECK_X(xc) || CHECK_X(xc+x)) )
			HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+x)),ROUND(yc-y),&pattern);
		if ( useQuad2 && CHECK_Y(yc-y) && (CHECK_X(xc) || CHECK_X(xc-x)) )
			HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc-x)),ROUND(yc-y),&pattern);
		if ( useQuad3 && CHECK_Y(yc+y) && (CHECK_X(xc) || CHECK_X(xc-x)) )
			HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc-x)),ROUND(yc+y),&pattern);
		if ( useQuad4 && CHECK_Y(yc+y) && (CHECK_X(xc) || CHECK_X(xc+x)) )
			HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+x)),ROUND(yc+y),&pattern);
		if ( !shortspan )
		{
			if ( startQuad == 1 )
			{
				if ( CHECK_Y(yc-y) )
				{
					if ( x <= startx )
					{
						if ( CHECK_X(xc) || CHECK_X(xc+x) )
							HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+x)),ROUND(yc-y),&pattern);
					}
					else
					{
						xclip = ROUND(y*startx/(double)starty);
						if ( CHECK_X(xc) || CHECK_X(xc+xclip) )
							HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+xclip)),ROUND(yc-y),&pattern);
					}
				}
			}
			else if ( startQuad == 2 )
			{
				if ( CHECK_Y(yc-y) )
				{
					if ( x >= startx )
					{
						xclip = ROUND(y*startx/(double)starty);
						if ( CHECK_X(xc-x) || CHECK_X(xc-xclip) )
							HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc-xclip)),ROUND(yc-y),&pattern);
					}
				}
			}
			else if ( startQuad == 3 )
			{
				if ( CHECK_Y(yc+y) )
				{
					if ( x <= startx )
					{
						if ( CHECK_X(xc-x) || CHECK_X(xc) )
							HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc)),ROUND(yc+y),&pattern);
					}
					else
					{
						xclip = ROUND(y*startx/(double)starty);
						if ( CHECK_X(xc-xclip) || CHECK_X(xc) )
							HLine(ROUND(CLIP_X(xc-xclip)),ROUND(CLIP_X(xc)),ROUND(yc+y),&pattern);
					}
				}
			}
			else if ( startQuad == 4 )
			{
				if ( CHECK_Y(yc+y) )
				{
					if ( x >= startx )
					{
						xclip = ROUND(y*startx/(double)starty);
						if ( CHECK_X(xc+xclip) || CHECK_X(xc+x) )
							HLine(ROUND(CLIP_X(xc+xclip)),ROUND(CLIP_X(xc+x)),ROUND(yc+y),&pattern);
					}
				}
			}

			if ( endQuad == 1 )
			{
				if ( CHECK_Y(yc-y) )
				{
					if ( x >= endx )
					{
						xclip = ROUND(y*endx/(double)endy);
						if ( CHECK_X(xc+xclip) || CHECK_X(xc+x) )
							HLine(ROUND(CLIP_X(xc+xclip)),ROUND(CLIP_X(xc+x)),ROUND(yc-y),&pattern);
					}
				}
			}
			else if ( endQuad == 2 )
			{
				if ( CHECK_Y(yc-y) )
				{
					if ( x <= endx )
					{
						if ( CHECK_X(xc-x) || CHECK_X(xc) )
							HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc)),ROUND(yc-y),&pattern);
					}
					else
					{
						xclip = ROUND(y*endx/(double)endy);
						if ( CHECK_X(xc-xclip) || CHECK_X(xc) )
							HLine(ROUND(CLIP_X(xc-xclip)),ROUND(CLIP_X(xc)),ROUND(yc-y),&pattern);
					}
				}
			}
			else if ( endQuad == 3 )
			{
				if ( CHECK_Y(yc+y) )
				{
					if ( x >= endx )
					{
						xclip = ROUND(y*endx/(double)endy);
						if ( CHECK_X(xc-x) || CHECK_X(xc-xclip) )
							HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc-xclip)),ROUND(yc+y),&pattern);
					}
				}
			}
			else if ( endQuad == 4 )
			{
				if ( CHECK_Y(yc+y) )
				{
					if ( x <= endx )
					{
						if ( CHECK_X(xc) || CHECK_X(xc+x) )
							HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+x)),ROUND(yc+y),&pattern);
					}
					else
					{
						xclip = ROUND(y*endx/(double)endy);
						if ( CHECK_X(xc) || CHECK_X(xc+xclip) )
							HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+xclip)),ROUND(yc+y),&pattern);
					}
				}
			}
		}
		else
		{
			startclip = ROUND(y*startx/(double)starty);
			endclip = ROUND(y*endx/(double)endy);
			if ( startQuad == 1 )
			{
				if ( CHECK_Y(yc-y) )
				{
					if ( (x <= startx) && (x >= endx) )
					{
						if ( CHECK_X(xc+endclip) || CHECK_X(xc+x) )
							HLine(ROUND(CLIP_X(xc+endclip)),ROUND(CLIP_X(xc+x)),ROUND(yc-y),&pattern);
					}
					else
					{
						if ( CHECK_X(xc+endclip) || CHECK_X(xc+startclip) )
							HLine(ROUND(CLIP_X(xc+endclip)),ROUND(CLIP_X(xc+startclip)),ROUND(yc-y),&pattern);
					}
				}
			}
			else if ( startQuad == 2 )
			{
				if ( CHECK_Y(yc-y) )
				{
					if ( (x <= startx) && (x >= endx) )
					{
						if ( CHECK_X(xc-x) || CHECK_X(xc-startclip) )
							HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc-startclip)),ROUND(yc-y),&pattern);
					}
					else
					{
						if ( CHECK_X(xc-endclip) || CHECK_X(xc-startclip) )
							HLine(ROUND(CLIP_X(xc-endclip)),ROUND(CLIP_X(xc-startclip)),ROUND(yc-y),&pattern);
					}
				}
			}
			else if ( startQuad == 3 )
			{
				if ( CHECK_Y(yc+y) )
				{
					if ( (x <= startx) && (x >= endx) )
					{
						if ( CHECK_X(xc-x) || CHECK_X(xc-endclip) )
							HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc-endclip)),ROUND(yc+y),&pattern);
					}
					else
					{
						if ( CHECK_X(xc-startclip) || CHECK_X(xc-endclip) )
							HLine(ROUND(CLIP_X(xc-startclip)),ROUND(CLIP_X(xc-endclip)),ROUND(yc+y),&pattern);
					}
				}
			}
			else if ( startQuad == 4 )
			{
				if ( CHECK_Y(yc+y) )
				{
					if ( (x <= startx) && (x >= endx) )
					{
						if ( CHECK_X(xc+startclip) || CHECK_X(xc+x) )
							HLine(ROUND(CLIP_X(xc+startclip)),ROUND(CLIP_X(xc+x)),ROUND(yc+y),&pattern);
					}
					else
					{
						if ( CHECK_X(xc+startclip) || CHECK_X(xc+endclip) )
							HLine(ROUND(CLIP_X(xc+startclip)),ROUND(CLIP_X(xc+endclip)),ROUND(yc+y),&pattern);
					}
				}
			}
		}
	}

	p = ROUND(Ry2*(x+.5)*(x+.5) + Rx2*(y-1)*(y-1) - Rx2*Ry2);
	while (y>0)
	{
		y--;
		py -= twoRx2;
		if (p>0)
			p += Rx2 - py;
		else
		{
			x++;
			px += twoRy2;
			p += Rx2 - py +px;
		}

		if ( useQuad1 && CHECK_Y(yc-y) && (CHECK_X(xc) || CHECK_X(xc+x)) )
			HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+x)),ROUND(yc-y),&pattern);
		if ( useQuad2 && CHECK_Y(yc-y) && (CHECK_X(xc) || CHECK_X(xc-x)) )
			HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc-x)),ROUND(yc-y),&pattern);
		if ( useQuad3 && CHECK_Y(yc+y) && (CHECK_X(xc) || CHECK_X(xc-x)) )
			HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc-x)),ROUND(yc+y),&pattern);
		if ( useQuad4 && CHECK_Y(yc+y) && (CHECK_X(xc) || CHECK_X(xc+x)) )
			HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+x)),ROUND(yc+y),&pattern);
		if ( !shortspan )
		{
			if ( startQuad == 1 )
			{
				if ( CHECK_Y(yc-y) )
				{
					if ( x <= startx )
					{
						if ( CHECK_X(xc) || CHECK_X(xc+x) )
							HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+x)),ROUND(yc-y),&pattern);
					}
					else
					{
						xclip = ROUND(y*startx/(double)starty);
						if ( CHECK_X(xc) || CHECK_X(xc+xclip) )
							HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+xclip)),ROUND(yc-y),&pattern);
					}
				}
			}
			else if ( startQuad == 2 )
			{
				if ( CHECK_Y(yc-y) )
				{
					if ( x >= startx )
					{
						xclip = ROUND(y*startx/(double)starty);
						if ( CHECK_X(xc-x) || CHECK_X(xc-xclip) )
							HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc-xclip)),ROUND(yc-y),&pattern);
					}
				}
			}
			else if ( startQuad == 3 )
			{
				if ( CHECK_Y(yc+y) )
				{
					if ( x <= startx )
					{
						if ( CHECK_X(xc-x) || CHECK_X(xc) )
							HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc)),ROUND(yc+y),&pattern);
					}
					else
					{
						xclip = ROUND(y*startx/(double)starty);
						if ( CHECK_X(xc-xclip) || CHECK_X(xc) )
							HLine(ROUND(CLIP_X(xc-xclip)),ROUND(CLIP_X(xc)),ROUND(yc+y),&pattern);
					}
				}
			}
			else if ( startQuad == 4 )
			{
				if ( CHECK_Y(yc+y) )
				{
					if ( x >= startx )
					{
						xclip = ROUND(y*startx/(double)starty);
						if ( CHECK_X(xc+xclip) || CHECK_X(xc+x) )
							HLine(ROUND(CLIP_X(xc+xclip)),ROUND(CLIP_X(xc+x)),ROUND(yc+y),&pattern);
					}
				}
			}

			if ( endQuad == 1 )
			{
				if ( CHECK_Y(yc-y) )
				{
					if ( x >= endx )
					{
						xclip = ROUND(y*endx/(double)endy);
						if ( CHECK_X(xc+xclip) || CHECK_X(xc+x) )
							HLine(ROUND(CLIP_X(xc+xclip)),ROUND(CLIP_X(xc+x)),ROUND(yc-y),&pattern);
					}
				}
			}
			else if ( endQuad == 2 )
			{
				if ( CHECK_Y(yc-y) )
				{
					if ( x <= endx )
					{
						if ( CHECK_X(xc-x) || CHECK_X(xc) )
							HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc)),ROUND(yc-y),&pattern);
					}
					else
					{
						xclip = ROUND(y*endx/(double)endy);
						if ( CHECK_X(xc-xclip) || CHECK_X(xc) )
							HLine(ROUND(CLIP_X(xc-xclip)),ROUND(CLIP_X(xc)),ROUND(yc-y),&pattern);
					}
				}
			}
			else if ( endQuad == 3 )
			{
				if ( CHECK_Y(yc+y) )
				{
					if ( x >= endx )
					{
						xclip = ROUND(y*endx/(double)endy);
						if ( CHECK_X(xc-x) || CHECK_X(xc-xclip) )
							HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc-xclip)),ROUND(yc+y),&pattern);
					}
				}
			}
			else if ( endQuad == 4 )
			{
				if ( CHECK_Y(yc+y) )
				{
					if ( x <= endx )
					{
						if ( CHECK_X(xc) || CHECK_X(xc+x) )
							HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+x)),ROUND(yc+y),&pattern);
					}
					else
					{
						xclip = ROUND(y*endx/(double)endy);
						if ( CHECK_X(xc) || CHECK_X(xc+xclip) )
							HLine(ROUND(CLIP_X(xc)),ROUND(CLIP_X(xc+xclip)),ROUND(yc+y),&pattern);
					}
				}
			}
		}
		else
		{
			startclip = ROUND(y*startx/(double)starty);
			endclip = ROUND(y*endx/(double)endy);
			if ( startQuad == 1 )
			{
				if ( CHECK_Y(yc-y) )
				{
					if ( (x <= startx) && (x >= endx) )
					{
						if ( CHECK_X(xc+endclip) || CHECK_X(xc+x) )
							HLine(ROUND(CLIP_X(xc+endclip)),ROUND(CLIP_X(xc+x)),ROUND(yc-y),&pattern);
					}
					else
					{
						if ( CHECK_X(xc+endclip) || CHECK_X(xc+startclip) )
							HLine(ROUND(CLIP_X(xc+endclip)),ROUND(CLIP_X(xc+startclip)),ROUND(yc-y),&pattern);
					}
				}
			}
			else if ( startQuad == 2 )
			{
				if ( CHECK_Y(yc-y) )
				{
					if ( (x <= startx) && (x >= endx) )
					{
						if ( CHECK_X(xc-x) || CHECK_X(xc-startclip) )
							HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc-startclip)),ROUND(yc-y),&pattern);
					}
					else
					{
						if ( CHECK_X(xc-endclip) || CHECK_X(xc-startclip) )
							HLine(ROUND(CLIP_X(xc-endclip)),ROUND(CLIP_X(xc-startclip)),ROUND(yc-y),&pattern);
					}
				}
			}
			else if ( startQuad == 3 )
			{
				if ( CHECK_Y(yc+y) )
				{
					if ( (x <= startx) && (x >= endx) )
					{
						if ( CHECK_X(xc-x) || CHECK_X(xc-endclip) )
							HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc-endclip)),ROUND(yc+y),&pattern);
					}
					else
					{
						if ( CHECK_X(xc-startclip) || CHECK_X(xc-endclip) )
							HLine(ROUND(CLIP_X(xc-startclip)),ROUND(CLIP_X(xc-endclip)),ROUND(yc+y),&pattern);
					}
				}
			}
			else if ( startQuad == 4 )
			{
				if ( CHECK_Y(yc+y) )
				{
					if ( (x <= startx) && (x >= endx) )
					{
						if ( CHECK_X(xc+startclip) || CHECK_X(xc+x) )
							HLine(ROUND(CLIP_X(xc+startclip)),ROUND(CLIP_X(xc+x)),ROUND(yc+y),&pattern);
					}
					else
					{
						if ( CHECK_X(xc+startclip) || CHECK_X(xc+endclip) )
							HLine(ROUND(CLIP_X(xc+startclip)),ROUND(CLIP_X(xc+endclip)),ROUND(yc+y),&pattern);
					}
				}
			}
		}
	}
	d->pensize = oldpensize;
	Unlock();

}

/*!
	\brief Called for all BView::FillBezier calls.
	\param pts 4-element array of BPoints in the order of start, end, and then the two control
	points. 
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call.
	I am not sure if this correctly handles cases where the curve backtracks along the diagonal
	line.  I should probably investigate how the be code handles that, but it is a low priority.
*/
void AccelerantDriver::FillBezier(BPoint *pts, LayerData *d, const Pattern &pat)
{
	double Ax, Bx, Cx, Dx;
	double Ay, By, Cy, Dy;
	int x, y;
	int lastx=-1, lasty=-1;
	double t;
	double dt = .0002;
	double dt2, dt3;
	double X, Y, dx, ddx, dddx, dy, ddy, dddy;
	float oldpensize;
	bool steep = false;

	Lock();
	if ( fabs(pts[3].y-pts[0].y) > fabs(pts[3].x-pts[0].x) )
		steep = true;
	
	AccLineCalc line(pts[0], pts[3]);
	oldpensize = d->pensize;
	d->pensize = 1;

	Ax = -pts[0].x + 3*pts[1].x - 3*pts[2].x + pts[3].x;
	Bx = 3*pts[0].x - 6*pts[1].x + 3*pts[2].x;
	Cx = -3*pts[0].x + 3*pts[1].x;
	Dx = pts[0].x;

	Ay = -pts[0].y + 3*pts[1].y - 3*pts[2].y + pts[3].y;
	By = 3*pts[0].y - 6*pts[1].y + 3*pts[2].y;
	Cy = -3*pts[0].y + 3*pts[1].y;
	Dy = pts[0].y;
	
	dt2 = dt * dt;
	dt3 = dt2 * dt;
	X = Dx;
	dx = Ax*dt3 + Bx*dt2 + Cx*dt;
	ddx = 6*Ax*dt3 + 2*Bx*dt2;
	dddx = 6*Ax*dt3;
	Y = Dy;
	dy = Ay*dt3 + By*dt2 + Cy*dt;
	ddy = 6*Ay*dt3 + 2*By*dt2;
	dddy = 6*Ay*dt3;

	lastx = -1;
	lasty = -1;

	for (t=0; t<=1; t+=dt)
	{
		x = ROUND(X);
		y = ROUND(Y);
		if ( (x!=lastx) || (y!=lasty) )
		{
			if ( steep )
				StrokeLine(BPoint(x,y),BPoint(line.GetX(y),y),d,pat);
			else
				StrokeLine(BPoint(x,y),BPoint(x,line.GetY(x)),d,pat);
		}
		lastx = x;
		lasty = y;

		X += dx;
		dx += ddx;
		ddx += dddx;
		Y += dy;
		dy += ddy;
		ddy += dddy;
	}
	d->pensize = oldpensize;
	Unlock();
}

/*!
	\brief Called for all BView::FillEllipse calls
	\param BRect enclosing the ellipse to be drawn.
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the ellipse may end up
	being clipped.
*/
void AccelerantDriver::FillEllipse(BRect r, LayerData *d, const Pattern &pat)
{
	float xc = (r.left+r.right)/2;
	float yc = (r.top+r.bottom)/2;
	float rx = r.Width()/2;
	float ry = r.Height()/2;
	int Rx2 = ROUND(rx*rx);
	int Ry2 = ROUND(ry*ry);
	int twoRx2 = 2*Rx2;
	int twoRy2 = 2*Ry2;
	int p;
	int x=0;
	int y = (int)ry;
	int px = 0;
	int py = twoRx2 * y;

	Lock();
	PatternHandler pattern(pat);
	pattern.SetColors(d->highcolor, d->lowcolor);

	if ( CHECK_X(xc) )
	{
		if ( CHECK_Y(yc-y) )
			SetPixel(ROUND(xc),ROUND(yc-y),pattern.GetColor(xc,yc-y));
		if ( CHECK_Y(yc+y) )
			SetPixel(ROUND(xc),ROUND(yc+y),pattern.GetColor(xc,yc+y));
	}

	p = ROUND (Ry2 - (Rx2 * ry) + (.25 * Rx2));
	while (px < py)
	{
		x++;
		px += twoRy2;
		if ( p < 0 )
			p += Ry2 + px;
		else
		{
			y--;
			py -= twoRx2;
			p += Ry2 + px - py;
		}

		if ( CHECK_X(xc-x) || CHECK_X(xc+x) )
		{
			if ( CHECK_Y(yc-y) )
	                	HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc+x)),ROUND(yc-y),&pattern);
			if ( CHECK_Y(yc+y) )
		                HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc+x)),ROUND(yc+y),&pattern);
		}
	}

	p = ROUND(Ry2*(x+.5)*(x+.5) + Rx2*(y-1)*(y-1) - Rx2*Ry2);
	while (y>0)
	{
		y--;
		py -= twoRx2;
		if (p>0)
			p += Rx2 - py;
		else
		{
			x++;
			px += twoRy2;
			p += Rx2 - py +px;
		}

		if ( CHECK_X(xc-x) || CHECK_X(xc+x) )
		{
			if ( CHECK_Y(yc-y) )
	                	HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc+x)),ROUND(yc-y),&pattern);
			if ( CHECK_Y(yc+y) )
		                HLine(ROUND(CLIP_X(xc-x)),ROUND(CLIP_X(xc+x)),ROUND(yc+y),&pattern);
		}
	}
	Unlock();
}

/*!
	\brief Called for all BView::FillPolygon calls
	\param ptlist Array of BPoints defining the polygon.
	\param numpts Number of points in the BPoint array.
	\param rect Rectangle which contains the polygon
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	The points in the array are not guaranteed to be within the framebuffer's 
	coordinate range.
*/
void AccelerantDriver::FillPolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, const Pattern &pat)
{
	/* Here's the plan.  Record all line segments in polygon.  If a line segments crosses
	   the y-value of a point not in the segment, split the segment into 2 segments.
	   Once we have gone through all of the segments, sort them primarily on y-value
	   and secondarily on x-value.  Step through each y-value in the bounding rectangle
	   and look for intersections with line segments.  First intersection is start of
	   horizontal line, second intersection is end of horizontal line.  Continue for
	   all pairs of intersections.  Watch out for horizontal line segments.
	*/
	Lock();
	if ( !ptlist || (numpts < 3) )
	{
		Unlock();
		return;
	}
	PatternHandler pattern(pat);
	pattern.SetColors(d->highcolor, d->lowcolor);

	BPoint *currentPoint, *nextPoint;
	BPoint tempNextPoint;
	BPoint tempCurrentPoint;
	int currentIndex, bestIndex, i, j, y;
	AccLineCalc *segmentArray = new AccLineCalc[2*numpts];
	int numSegments = 0;

	/* Generate the segment list */
	currentPoint = ptlist;
	currentIndex = 0;
	nextPoint = &ptlist[1];
	while (currentPoint)
	{
		if ( numSegments >= 2*numpts )
		{
			printf("ERROR: Insufficient memory allocated to segment array\n");
			delete[] segmentArray;
			return;
		}

		for (i=0; i<numpts; i++)
		{
			if ( ((ptlist[i].y > currentPoint->y) && (ptlist[i].y < nextPoint->y)) ||
				((ptlist[i].y < currentPoint->y) && (ptlist[i].y > nextPoint->y)) )
			{
				segmentArray[numSegments].SetPoints(*currentPoint,*nextPoint);
				tempNextPoint.x = segmentArray[numSegments].GetX(ptlist[i].y);
				tempNextPoint.y = ptlist[i].y;
				nextPoint = &tempNextPoint;
			}
		}

		segmentArray[numSegments].SetPoints(*currentPoint,*nextPoint);
		numSegments++;
		if ( nextPoint == &tempNextPoint )
		{
			tempCurrentPoint = tempNextPoint;
			currentPoint = &tempCurrentPoint;
			nextPoint = &ptlist[(currentIndex+1)%numpts];
		}
		else if ( nextPoint == ptlist )
		{
			currentPoint = NULL;
		}
		else
		{
			currentPoint = nextPoint;
			currentIndex++;
			nextPoint = &ptlist[(currentIndex+1)%numpts];
		}
	}

	/* Selection sort the segments.  Probably should replace this later. */
	for (i=0; i<numSegments; i++)
	{
		bestIndex = i;
		for (j=i+1; j<numSegments; j++)
		{
			if ( (segmentArray[j].MinY() < segmentArray[bestIndex].MinY()) ||
				((segmentArray[j].MinY() == segmentArray[bestIndex].MinY()) &&
				 (segmentArray[j].MinX() < segmentArray[bestIndex].MinX())) )
				bestIndex = j;
		}
		if (bestIndex != i)
			segmentArray[i].Swap(segmentArray[bestIndex]);
	}

	/* Draw the lines */
	for (y=ROUND(rect.top); y<=ROUND(rect.bottom); y++)
	{
		if ( CHECK_Y(y) )
		{
			i = 0;
			while (i<numSegments)
			{
				if (segmentArray[i].MinY() > y)
					break;
				if (segmentArray[i].MaxY() < y)
				{
					i++;
					continue;
				}
				if (segmentArray[i].MinY() == segmentArray[i].MaxY())
				{
					if ( (segmentArray[i].MinX() < mDisplayMode.virtual_width) &&
						(segmentArray[i].MaxX() >= 0) )
						HLine(CLIP_X(ROUND(segmentArray[i].MinX())),
						      CLIP_X(ROUND(segmentArray[i].MaxX())),
						      y, &pattern);
					i++;
				}
				else
				{
					if ( (segmentArray[i].GetX(y) < mDisplayMode.virtual_width) &&
						(segmentArray[i+1].GetX(y) >= 0) )
						HLine(CLIP_X(ROUND(segmentArray[i].GetX(y))),
						      CLIP_X(ROUND(segmentArray[i+1].GetX(y))),
						      y, &pattern);
					i+=2;
				}
			}
		}
	}
	delete[] segmentArray;
	Unlock();
}

/*!
	\brief Called for all BView::FillRect calls
	\param r BRect to be filled. Guaranteed to be in the frame buffer's coordinate space
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

*/
void AccelerantDriver::FillRect(BRect r, LayerData *d, const Pattern &pattern)
{
	Lock();
	const int8 *pat=pattern.GetInt8();
#ifndef DISABLE_HARDWARE_ACCELERATION
	if ( accFillRect && AcquireEngine && (((uint8)*pat == 0xFF) || (*pat == 0)) )
	{
		bool solidColor = true;
		int i;
		for (i=1; i<8; i++)
			if ( pat[i] != pat[i-1] )
				solidColor = false;
		if ( solidColor && (AcquireEngine(0,0,NULL,&mEngineToken) == B_OK) )
		{
			fill_rect_params fillParams;
			uint32 color=0;
			fillParams.right = (uint16)r.right;
			fillParams.left = (uint16)r.left;
			fillParams.top = (uint16)r.top;
			fillParams.bottom = (uint16)r.bottom;
			if ( (uint8)*pat == 0xFF )
			{
				switch (mDisplayMode.space)
				{
					case B_CMAP8:
					case B_GRAY8:
						color = d->highcolor.GetColor8();
						break;
					case B_RGB16_BIG:
					case B_RGB16_LITTLE:
					case B_RGB15_BIG:
					case B_RGBA15_BIG:
					case B_RGB15_LITTLE:
					case B_RGBA15_LITTLE:
						color = d->highcolor.GetColor16();
						break;
					case B_RGB32_BIG:
					case B_RGBA32_BIG:
					case B_RGB32_LITTLE:
					case B_RGBA32_LITTLE:
					{
						rgb_color rgbcolor = d->highcolor.GetColor32();
						color = (rgbcolor.alpha << 24) | (rgbcolor.red << 16) | (rgbcolor.green << 8) | (rgbcolor.blue);
					}
						break;
				}
			}
			else
			{
				switch (mDisplayMode.space)
				{
					case B_CMAP8:
					case B_GRAY8:
						color = d->lowcolor.GetColor8();
						break;
					case B_RGB16_BIG:
					case B_RGB16_LITTLE:
					case B_RGB15_BIG:
					case B_RGBA15_BIG:
					case B_RGB15_LITTLE:
					case B_RGBA15_LITTLE:
						color = d->lowcolor.GetColor16();
						break;
					case B_RGB32_BIG:
					case B_RGBA32_BIG:
					case B_RGB32_LITTLE:
					case B_RGBA32_LITTLE:
					{
						rgb_color rgbcolor = d->lowcolor.GetColor32();
						color = (rgbcolor.alpha << 24) | (rgbcolor.red << 16) | (rgbcolor.green << 8) | (rgbcolor.blue);
					}
						break;
				}
			}
			accFillRect(mEngineToken, color, &fillParams, 1);
			if ( ReleaseEngine )
				ReleaseEngine(mEngineToken,NULL);
			Unlock();
			return;
		}
	}
#endif

	PatternHandler pathandler(pattern);
	pathandler.SetColors(d->highcolor, d->lowcolor);

	switch (mDisplayMode.space)
	{
		case B_CMAP8:
		case B_GRAY8:
			{
				uint8 *fb = (uint8 *)mFrameBufferConfig.frame_buffer + (int)r.top*mFrameBufferConfig.bytes_per_row;
				int x,y;
				for (y=(int)r.top; y<=(int)r.bottom; y++)
				{
					for (x=(int)r.left; x<=r.right; x++)
					{
						fb[x] = pathandler.GetColor(x,y).GetColor8();
					}
					fb += mFrameBufferConfig.bytes_per_row;
				}
			} break;
		case B_RGB16_BIG:
		case B_RGB16_LITTLE:
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_RGB15_LITTLE:
		case B_RGBA15_LITTLE:
			{
				uint16 *fb = (uint16 *)((uint8 *)mFrameBufferConfig.frame_buffer + (int)r.top*mFrameBufferConfig.bytes_per_row);
				int x,y;
				for (y=(int)r.top; y<=(int)r.bottom; y++)
				{
					for (x=(int)r.left; x<=r.right; x++)
					{
						fb[x] = pathandler.GetColor(x,y).GetColor16();
					}
					fb = (uint16 *)((uint8 *)fb + mFrameBufferConfig.bytes_per_row);
				}
			} break;
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGB32_LITTLE:
		case B_RGBA32_LITTLE:
			{
				uint32 *fb = (uint32 *)((uint8 *)mFrameBufferConfig.frame_buffer + (int)r.top*mFrameBufferConfig.bytes_per_row);
				int x,y;
				rgb_color color;
				for (y=(int)r.top; y<=(int)r.bottom; y++)
				{
					for (x=(int)r.left; x<=r.right; x++)
					{
						color = pathandler.GetColor(x,y).GetColor32();
						fb[x] = (color.alpha << 24) | (color.red << 16) | (color.green << 8) | (color.blue);
					}
					fb = (uint32 *)((uint8 *)fb + mFrameBufferConfig.bytes_per_row);
				}
			} break;
		default:
			printf("Error: Unknown color space\n");
	}

	Unlock();
}

/*!
	\brief Called for all BView::FillRoundRect calls
	\param r Rectangle to fill
	\param X radius of the corner arcs
	\param Y radius of the corner arcs
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the roundrect may end 
	up being clipped.
*/
void AccelerantDriver::FillRoundRect(BRect r, float xrad, float yrad, LayerData *d, const Pattern &pat)
{
	float arc_x;
	float yrad2 = yrad*yrad;
	int i;

	Lock();
	PatternHandler pattern(pat);
	pattern.SetColors(d->highcolor, d->lowcolor);
	
	for (i=0; i<=(int)yrad; i++)
	{
		arc_x = xrad*sqrt(1-i*i/yrad2);
		if ( CHECK_Y(r.top+yrad-i) )
			HLine(ROUND(CLIP_X(r.left+xrad-arc_x)), ROUND(CLIP_X(r.right-xrad+arc_x)),
				ROUND(r.top+yrad-i), &pattern);
		if ( CHECK_Y(r.bottom-yrad+i) )
			HLine(ROUND(CLIP_X(r.left+xrad-arc_x)), ROUND(CLIP_X(r.right-xrad+arc_x)),
				ROUND(r.bottom-yrad+i), &pattern);
	}
	FillRect(BRect(CLIP_X(r.left),CLIP_Y(r.top+yrad),
			CLIP_X(r.right),CLIP_Y(r.bottom-yrad)), d, pat);

	Unlock();
}

//void AccelerantDriver::FillShape(SShape *sh, LayerData *d, const Pattern &pat)
//{
//}

/*!
	\brief Called for all BView::FillTriangle calls
	\param pts Array of 3 BPoints. Always non-NULL.
	\param r BRect enclosing the triangle. While it will definitely enclose the triangle,
	it may not be within the frame buffer's bounds.
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the triangle may end 
	up being clipped.
*/
void AccelerantDriver::FillTriangle(BPoint *pts, BRect r, LayerData *d, const Pattern &pat)
{
	if(!pts || !d)
		return;

	Lock();
	BPoint first, second, third;
	PatternHandler pattern(pat);
	pattern.SetColors(d->highcolor, d->lowcolor);

	// Sort points according to their y values and x values (y is primary)
	if ( (pts[0].y < pts[1].y) ||
		((pts[0].y == pts[1].y) && (pts[0].x <= pts[1].x)) )
	{
		first=pts[0];
		second=pts[1];
	}
	else
	{
		first=pts[1];
		second=pts[0];
	}
	
	if ( (second.y<pts[2].y) ||
		((second.y == pts[2].y) && (second.x <= pts[2].x)) )
	{
		third=pts[2];
	}
	else
	{
		// second is lower than "third", so we must ensure that this third point
		// isn't higher than our first point
		third=second;
		if ( (first.y<pts[2].y) ||
			((first.y == pts[2].y) && (first.x <= pts[2].x)) )
			second=pts[2];
		else
		{
			second=first;
			first=pts[2];
		}
	}
	
	// Now that the points are sorted, check to see if they all have the same
	// y value
	if(first.y==second.y && second.y==third.y)
	{
		BPoint start,end;
		start.y=first.y;
		end.y=first.y;
		start.x=MIN(first.x,MIN(second.x,third.x));
		end.x=MAX(first.x,MAX(second.x,third.x));
		if ( CHECK_Y(start.y) && (CHECK_X(start.x) || CHECK_X(end.x)) )
			HLine(ROUND(CLIP_X(start.x)), ROUND(CLIP_X(end.x)), ROUND(start.y), &pattern);
		Unlock();
		return;
	}

	int32 i;

	// Special case #1: first and second in the same row
	if(first.y==second.y)
	{
		AccLineCalc lineA(first, third);
		AccLineCalc lineB(second, third);
		
		if ( CHECK_Y(first.y) && (CHECK_X(first.x) || CHECK_X(second.x)) )
			HLine(ROUND(CLIP_X(first.x)), ROUND(CLIP_X(second.x)), ROUND(first.y), &pattern);
		for(i=(int32)first.y+1; i<=third.y; i++)
			if ( CHECK_Y(i) && (CHECK_X(lineA.GetX(i)) || CHECK_X(lineB.GetX(i))) )
				HLine(ROUND(CLIP_X(lineA.GetX(i))), ROUND(CLIP_X(lineB.GetX(i))), i, &pattern);
		Unlock();
		return;
	}
	
	// Special case #2: second and third in the same row
	if(second.y==third.y)
	{
		AccLineCalc lineA(first, second);
		AccLineCalc lineB(first, third);
		
		if ( CHECK_Y(second.y) && (CHECK_X(second.x) || CHECK_X(third.x)) )
			HLine(ROUND(CLIP_X(second.x)), ROUND(CLIP_X(third.x)), ROUND(second.y), &pattern);
		for(i=(int32)first.y; i<third.y; i++)
			if ( CHECK_Y(i) && (CHECK_X(lineA.GetX(i)) || CHECK_X(lineB.GetX(i))) )
				HLine(ROUND(CLIP_X(lineA.GetX(i))), ROUND(CLIP_X(lineB.GetX(i))), i, &pattern);
		Unlock();
		return;
	}
	
	// Normal case.	
	AccLineCalc lineA(first, second);
	AccLineCalc lineB(first, third);
	AccLineCalc lineC(second, third);
	
	for(i=(int32)first.y; i<(int32)second.y; i++)
		if ( CHECK_Y(i) && (CHECK_X(lineA.GetX(i)) || CHECK_X(lineB.GetX(i))) )
			HLine(ROUND(CLIP_X(lineA.GetX(i))), ROUND(CLIP_X(lineB.GetX(i))), i, &pattern);

	for(i=(int32)second.y; i<=third.y; i++)
		if ( CHECK_Y(i) && (CHECK_X(lineC.GetX(i)) || CHECK_X(lineB.GetX(i))) )
			HLine(ROUND(CLIP_X(lineC.GetX(i))), ROUND(CLIP_X(lineB.GetX(i))), i, &pattern);
		
	Unlock();
}

/*!
	\brief Hides the cursor.
	
	Hide calls are not nestable, unlike that of the BApplication class. Subclasses should
	call _SetCursorHidden(true) somewhere within this function to ensure that data is
	maintained accurately.
*/
void AccelerantDriver::HideCursor(void)
{
	Lock();
	if(!IsCursorHidden())
	{
		if ( accShowCursor )
			accShowCursor(false);
		else
			BlitBitmap(under_cursor,under_cursor->Bounds(),cursorframe, B_OP_COPY);
	}
	DisplayDriver::HideCursor();
	Unlock();
}

/*!
	\brief Moves the cursor to the given point.
	\param x Cursor's new x coordinate
	\param y Cursor's new y coordinate

	The coordinates passed to MoveCursorTo are guaranteed to be within the frame buffer's
	range, but the cursor data itself will need to be clipped. A check to see if the 
	cursor is obscured should be made and if so, a call to _SetCursorObscured(false) 
	should be made the cursor in addition to displaying at the passed coordinates.
*/
void AccelerantDriver::MoveCursorTo(float x, float y)
{
	/* TODO: Add correct handling of obscured cursors */
	Lock();
	if ( accMoveCursor )
	{
		accMoveCursor((uint16)x,(uint16)y);
	}
	else
	{
		if(!IsCursorHidden())
			BlitBitmap(under_cursor,under_cursor->Bounds(),cursorframe, B_OP_COPY);

		cursorframe.OffsetTo(x,y);
		ExtractToBitmap(under_cursor,under_cursor->Bounds(),cursorframe);
	
		if(!IsCursorHidden())
			BlitBitmap(cursor,cursor->Bounds(),cursorframe, B_OP_OVER);
	}
	
	Unlock();
}

/*!
	\brief Inverts the colors in the rectangle.
	\param r Rectangle of the area to be inverted. Guaranteed to be within bounds.
*/
void AccelerantDriver::InvertRect(BRect r)
{
	Lock();
	if ( accInvertRect && AcquireEngine && (AcquireEngine(0,0,NULL,&mEngineToken) == B_OK) )
	{
		fill_rect_params fillParams;
		fillParams.right = (uint16)r.right;
		fillParams.left = (uint16)r.left;
		fillParams.top = (uint16)r.top;
		fillParams.bottom = (uint16)r.bottom;
		accInvertRect(mEngineToken, &fillParams, 1);
		if ( ReleaseEngine )
			ReleaseEngine(mEngineToken,NULL);
		Unlock();
		return;
	}
	switch (mDisplayMode.space)
	{
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGB32_LITTLE:
		case B_RGBA32_LITTLE:
		{
			uint16 width=r.IntegerWidth();
			uint16 height=r.IntegerHeight();
			uint32 *start=(uint32*)mFrameBufferConfig.frame_buffer;
			uint32 *index;
			start = (uint32 *)((uint8 *)start+(int32)r.top*mFrameBufferConfig.bytes_per_row);
			start+=(int32)r.left;

			index = start;
			for(int32 i=0;i<height;i++)
			{
				/* Are we inverting the right bits?
				   Not sure about location of alpha */
				for(int32 j=0; j<width; j++)
					index[j]^=0xFFFFFF00L;
				index = (uint32 *)((uint8 *)index+mFrameBufferConfig.bytes_per_row);
			}
		}
		break;
		case B_RGB16_BIG:
		case B_RGB16_LITTLE:
		{
			uint16 width=r.IntegerWidth();
			uint16 height=r.IntegerHeight();
			uint16 *start=(uint16*)mFrameBufferConfig.frame_buffer;
			uint16 *index;
			start = (uint16 *)((uint8 *)start+(int32)r.top*mFrameBufferConfig.bytes_per_row);
			start+=(int32)r.left;

			index = start;
			for(int32 i=0;i<height;i++)
			{
				for(int32 j=0; j<width; j++)
					index[j]^=0xFFFF;
				index = (uint16 *)((uint8 *)index+mFrameBufferConfig.bytes_per_row);
			}
		}
		break;
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_RGB15_LITTLE:
		case B_RGBA15_LITTLE:
		{
			uint16 width=r.IntegerWidth();
			uint16 height=r.IntegerHeight();
			uint16 *start=(uint16*)mFrameBufferConfig.frame_buffer;
			uint16 *index;
			start = (uint16 *)((uint8 *)start+(int32)r.top*mFrameBufferConfig.bytes_per_row);
			start+=(int32)r.left;

			index = start;
			for(int32 i=0;i<height;i++)
			{
				/* TODO: Where is the alpha bit */
				for(int32 j=0; j<width; j++)
					index[j]^=0xFFFF;
				index = (uint16 *)((uint8 *)index+mFrameBufferConfig.bytes_per_row);
			}
		}
		break;
		case B_CMAP8:
		case B_GRAY8:
		{
			uint16 width=r.IntegerWidth();
			uint16 height=r.IntegerHeight();
			uint8 *start=(uint8*)mFrameBufferConfig.frame_buffer;
			uint8 *index;
			start = (uint8 *)start+(int32)r.top*mFrameBufferConfig.bytes_per_row;
			start+=(int32)r.left;

			index = start;
			for(int32 i=0;i<height;i++)
			{
				for(int32 j=0; j<width; j++)
					index[j]^=0xFF;
				index = (uint8 *)index+mFrameBufferConfig.bytes_per_row;
			}
		}
		break;
		default:
		break;
	}
	Unlock();
}

/*!
	\brief Shows the cursor.
	
	Show calls are not nestable, unlike that of the BApplication class. Subclasses should
	call _SetCursorHidden(false) somewhere within this function to ensure that data is
	maintained accurately.
*/
void AccelerantDriver::ShowCursor(void)
{
	Lock();
	if(IsCursorHidden())
	{
		if ( accShowCursor )
			accShowCursor(true);
		else
			BlitBitmap(cursor,cursor->Bounds(),cursorframe, B_OP_OVER);
	}
	DisplayDriver::ShowCursor();
	Unlock();
}

/*!
	\brief Obscures the cursor.
	
	Obscure calls are not nestable. Subclasses should call _SetCursorObscured(true) 
	somewhere within this function to ensure that data is maintained accurately. When the
	next call to MoveCursorTo() is made, the cursor will be shown again.
*/
void AccelerantDriver::ObscureCursor(void)
{
	Lock();
	if (!IsCursorHidden() )
	{
		if ( accShowCursor )
			accShowCursor(false);
		else
			BlitBitmap(under_cursor,under_cursor->Bounds(),cursorframe, B_OP_COPY);
	}
	DisplayDriver::ObscureCursor();
	Unlock();
}

/*!
	\brief Changes the cursor.
	\param cursor The new cursor. Guaranteed to be non-NULL.
	
	The driver does not take ownership of the given cursor. Subclasses should make
	a copy of the cursor passed to it. The default version of this function hides the
	cursor, replaces it, and shows the cursor if previously visible.
*/
void AccelerantDriver::SetCursor(ServerCursor *csr)
{
	if(!csr)
		return;
		
	Lock();
	if ( accSetCursorShape && (csr->BitsPerPixel() == 1) )
	{
		/* TODO: Need to fix transparency */
		if(cursor)
			delete cursor;
		cursor=new ServerCursor(csr);
		cursorframe.right=cursorframe.left+csr->Bounds().Width();
		cursorframe.bottom=cursorframe.top+csr->Bounds().Height();
		uint16 width = (uint16)cursor->Bounds().Width();
		uint16 height = (uint16)cursor->Bounds().Height();
		uint16 hot_x = (uint16)cursor->GetHotSpot().x;
		uint16 hot_y = (uint16)cursor->GetHotSpot().y;
		uint8 *andMask = new uint8[width*height/8];
		//uint8 *xorMask = new uint8[width*height/8];
		memset(andMask,(uint8)255,width*height/8);
		accSetCursorShape(width,height,hot_x,hot_y,andMask,cursor->Bits());

		delete[] andMask;
		//delete[] xorMask;
	}
	else
	{
		// erase old if visible
		if(!IsCursorHidden() && under_cursor)
			BlitBitmap(under_cursor,under_cursor->Bounds(),cursorframe, B_OP_COPY);

		if(cursor)
			delete cursor;
		if(under_cursor)
			delete under_cursor;
	
		cursor=new ServerCursor(csr);
		under_cursor=new ServerCursor(csr);
	
		cursorframe.right=cursorframe.left+csr->Bounds().Width();
		cursorframe.bottom=cursorframe.top+csr->Bounds().Height();
	
		ExtractToBitmap(under_cursor,under_cursor->Bounds(),cursorframe);
	
		if(!IsCursorHidden())
			BlitBitmap(cursor,cursor->Bounds(),cursorframe, B_OP_OVER);
	}
	Unlock();
}

/*!
	\brief Called for all BView::StrokeArc calls
	\param r Rectangle enclosing the entire ellipse the arc is part of
	\param angle Starting angle for the arc in degrees (zero is pointing to the right)
	\param span Span of the arc in degrees counterclockwise. Ending angle = angle+span.
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the arc may end up
	being clipped.
*/void AccelerantDriver::StrokeArc(BRect r, float angle, float span, LayerData *d, const Pattern &pat)
{
	float xc = (r.left+r.right)/2;
	float yc = (r.top+r.bottom)/2;
	float rx = r.Width()/2;
	float ry = r.Height()/2;
	int Rx2 = ROUND(rx*rx);
	int Ry2 = ROUND(ry*ry);
	int twoRx2 = 2*Rx2;
	int twoRy2 = 2*Ry2;
	int p;
	int x=0;
	int y = (int)ry;
	int px = 0;
	int py = twoRx2 * y;
	int startx, endx;
	int startQuad, endQuad;
	bool useQuad1, useQuad2, useQuad3, useQuad4;
	bool shortspan = false;
	int thick;

	// Watch out for bozos giving us whacko spans
	if ( (span >= 360) || (span <= -360) )
	{
	  StrokeEllipse(r,d,pat);
	  return;
	}

	Lock();
	PatternHandler pattern(pat);
	pattern.SetColors(d->highcolor, d->lowcolor);

	thick = (int)d->pensize;
	if ( span > 0 )
	{
		startQuad = (int)(angle/90)%4+1;
		endQuad = (int)((angle+span)/90)%4+1;
		startx = ROUND(.5*r.Width()*fabs(cos(angle*M_PI/180)));
		endx = ROUND(.5*r.Width()*fabs(cos((angle+span)*M_PI/180)));
	}
	else
	{
		endQuad = (int)(angle/90)%4+1;
		startQuad = (int)((angle+span)/90)%4+1;
		endx = ROUND(.5*r.Width()*fabs(cos(angle*M_PI/180)));
		startx = ROUND(.5*r.Width()*fabs(cos((angle+span)*M_PI/180)));
	}

	if ( startQuad != endQuad )
	{
		useQuad1 = (endQuad > 1) && (startQuad > endQuad);
		useQuad2 = ((startQuad == 1) && (endQuad > 2)) || ((startQuad > endQuad) && (endQuad > 2));
		useQuad3 = ((startQuad < 3) && (endQuad == 4)) || ((startQuad < 3) && (endQuad < startQuad));
		useQuad4 = (startQuad < 4) && (startQuad > endQuad);
	}
	else
	{
		if ( (span < 90) && (span > -90) )
		{
			useQuad1 = false;
			useQuad2 = false;
			useQuad3 = false;
			useQuad4 = false;
			shortspan = true;
		}
		else
		{
			useQuad1 = (startQuad != 1);
			useQuad2 = (startQuad != 2);
			useQuad3 = (startQuad != 3);
			useQuad4 = (startQuad != 4);
		}
	}

        /* SetThickPixel does bounds checking, so we don't have to worry about it here */
	if ( useQuad1 || 
	     (!shortspan && (((startQuad == 1) && (x <= startx)) || ((endQuad == 1) && (x >= endx)))) || 
	     (shortspan && (startQuad == 1) && (x <= startx) && (x >= endx)) ) 
		SetThickPixel(ROUND(xc+x),ROUND(yc-y),thick,&pattern);
	if ( useQuad2 || 
	     (!shortspan && (((startQuad == 2) && (x >= startx)) || ((endQuad == 2) && (x <= endx)))) || 
	     (shortspan && (startQuad == 2) && (x >= startx) && (x <= endx)) ) 
		SetThickPixel(ROUND(xc-x),ROUND(yc-y),thick,&pattern);
	if ( useQuad3 || 
	     (!shortspan && (((startQuad == 3) && (x <= startx)) || ((endQuad == 3) && (x >= endx)))) || 
	     (shortspan && (startQuad == 3) && (x <= startx) && (x >= endx)) ) 
		SetThickPixel(ROUND(xc-x),ROUND(yc+y),thick,&pattern);
	if ( useQuad4 || 
	     (!shortspan && (((startQuad == 4) && (x >= startx)) || ((endQuad == 4) && (x <= endx)))) || 
	     (shortspan && (startQuad == 4) && (x >= startx) && (x <= endx)) ) 
		SetThickPixel(ROUND(xc+x),ROUND(yc+y),thick,&pattern);

	p = ROUND (Ry2 - (Rx2 * ry) + (.25 * Rx2));
	while (px < py)
	{
		x++;
		px += twoRy2;
		if ( p < 0 )
			p += Ry2 + px;
		else
		{
			y--;
			py -= twoRx2;
			p += Ry2 + px - py;
		}

		if ( useQuad1 || 
		     (!shortspan && (((startQuad == 1) && (x <= startx)) || ((endQuad == 1) && (x >= endx)))) || 
		     (shortspan && (startQuad == 1) && (x <= startx) && (x >= endx)) ) 
			SetThickPixel(ROUND(xc+x),ROUND(yc-y),thick,&pattern);
		if ( useQuad2 || 
		     (!shortspan && (((startQuad == 2) && (x >= startx)) || ((endQuad == 2) && (x <= endx)))) || 
		     (shortspan && (startQuad == 2) && (x >= startx) && (x <= endx)) ) 
			SetThickPixel(ROUND(xc-x),ROUND(yc-y),thick,&pattern);
		if ( useQuad3 || 
		     (!shortspan && (((startQuad == 3) && (x <= startx)) || ((endQuad == 3) && (x >= endx)))) || 
		     (shortspan && (startQuad == 3) && (x <= startx) && (x >= endx)) ) 
			SetThickPixel(ROUND(xc-x),ROUND(yc+y),thick,&pattern);
		if ( useQuad4 || 
		     (!shortspan && (((startQuad == 4) && (x >= startx)) || ((endQuad == 4) && (x <= endx)))) || 
		     (shortspan && (startQuad == 4) && (x >= startx) && (x <= endx)) ) 
			SetThickPixel(ROUND(xc+x),ROUND(yc+y),thick,&pattern);
	}

	p = ROUND(Ry2*(x+.5)*(x+.5) + Rx2*(y-1)*(y-1) - Rx2*Ry2);
	while (y>0)
	{
		y--;
		py -= twoRx2;
		if (p>0)
			p += Rx2 - py;
		else
		{
			x++;
			px += twoRy2;
			p += Rx2 - py +px;
		}

		if ( useQuad1 || 
		     (!shortspan && (((startQuad == 1) && (x <= startx)) || ((endQuad == 1) && (x >= endx)))) || 
		     (shortspan && (startQuad == 1) && (x <= startx) && (x >= endx)) ) 
			SetThickPixel(ROUND(xc+x),ROUND(yc-y),thick,&pattern);
		if ( useQuad2 || 
		     (!shortspan && (((startQuad == 2) && (x >= startx)) || ((endQuad == 2) && (x <= endx)))) || 
		     (shortspan && (startQuad == 2) && (x >= startx) && (x <= endx)) ) 
			SetThickPixel(ROUND(xc-x),ROUND(yc-y),thick,&pattern);
		if ( useQuad3 || 
		     (!shortspan && (((startQuad == 3) && (x <= startx)) || ((endQuad == 3) && (x >= endx)))) || 
		     (shortspan && (startQuad == 3) && (x <= startx) && (x >= endx)) ) 
			SetThickPixel(ROUND(xc-x),ROUND(yc+y),thick,&pattern);
		if ( useQuad4 || 
		     (!shortspan && (((startQuad == 4) && (x >= startx)) || ((endQuad == 4) && (x <= endx)))) || 
		     (shortspan && (startQuad == 4) && (x >= startx) && (x <= endx)) ) 
			SetThickPixel(ROUND(xc+x),ROUND(yc+y),thick,&pattern);
	}
	Unlock();
}

/*!
	\brief Called for all BView::StrokeBezier calls.
	\param pts 4-element array of BPoints in the order of start, end, and then the two control
	points. 
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call.
*/
void AccelerantDriver::StrokeBezier(BPoint *pts, LayerData *d, const Pattern &pat)
{
	double Ax, Bx, Cx, Dx;
	double Ay, By, Cy, Dy;
	int x, y;
	int lastx=-1, lasty=-1;
	double t;
	double dt = .0005;
	double dt2, dt3;
	double X, Y, dx, ddx, dddx, dy, ddy, dddy;

	Lock();
	PatternHandler pattern(pat);
	pattern.SetColors(d->highcolor, d->lowcolor);

	Ax = -pts[0].x + 3*pts[1].x - 3*pts[2].x + pts[3].x;
	Bx = 3*pts[0].x - 6*pts[1].x + 3*pts[2].x;
	Cx = -3*pts[0].x + 3*pts[1].x;
	Dx = pts[0].x;

	Ay = -pts[0].y + 3*pts[1].y - 3*pts[2].y + pts[3].y;
	By = 3*pts[0].y - 6*pts[1].y + 3*pts[2].y;
	Cy = -3*pts[0].y + 3*pts[1].y;
	Dy = pts[0].y;
	
	dt2 = dt * dt;
	dt3 = dt2 * dt;
	X = Dx;
	dx = Ax*dt3 + Bx*dt2 + Cx*dt;
	ddx = 6*Ax*dt3 + 2*Bx*dt2;
	dddx = 6*Ax*dt3;
	Y = Dy;
	dy = Ay*dt3 + By*dt2 + Cy*dt;
	ddy = 6*Ay*dt3 + 2*By*dt2;
	dddy = 6*Ay*dt3;

	lastx = -1;
	lasty = -1;

	for (t=0; t<=1; t+=dt)
	{
		x = ROUND(X);
		y = ROUND(Y);
                /* SetThickPixel does bounds checking, so we don't have to worry about it */
		if ( (x!=lastx) || (y!=lasty) )
			SetThickPixel(x,y,ROUND(d->pensize),&pattern);
		lastx = x;
		lasty = y;

		X += dx;
		dx += ddx;
		ddx += dddx;
		Y += dy;
		dy += ddy;
		ddy += dddy;
	}
	Unlock();
}

/*!
	\brief Called for all BView::StrokeEllipse calls
	\param r BRect enclosing the ellipse to be drawn.
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the ellipse may end up
	being clipped.
*/
void AccelerantDriver::StrokeEllipse(BRect r, LayerData *d, const Pattern &pat)
{
	float xc = (r.left+r.right)/2;
	float yc = (r.top+r.bottom)/2;
	float rx = r.Width()/2;
	float ry = r.Height()/2;
	int Rx2 = ROUND(rx*rx);
	int Ry2 = ROUND(ry*ry);
	int twoRx2 = 2*Rx2;
	int twoRy2 = 2*Ry2;
	int p;
	int x=0;
	int y = (int)ry;
	int px = 0;
	int py = twoRx2 * y;
	int thick;

	Lock();
	thick = (int)d->pensize;
	PatternHandler pattern(pat);
	pattern.SetColors(d->highcolor, d->lowcolor);

        /* SetThickPixel does bounds checking, so we don't have to worry about it */

	SetThickPixel(ROUND(xc+x),ROUND(yc-y),thick,&pattern);
	SetThickPixel(ROUND(xc-x),ROUND(yc-y),thick,&pattern);
	SetThickPixel(ROUND(xc-x),ROUND(yc+y),thick,&pattern);
	SetThickPixel(ROUND(xc+x),ROUND(yc+y),thick,&pattern);

	p = ROUND (Ry2 - (Rx2 * ry) + (.25 * Rx2));
	while (px < py)
	{
		x++;
		px += twoRy2;
		if ( p < 0 )
			p += Ry2 + px;
		else
		{
			y--;
			py -= twoRx2;
			p += Ry2 + px - py;
		}

		SetThickPixel(ROUND(xc+x),ROUND(yc-y),thick,&pattern);
		SetThickPixel(ROUND(xc-x),ROUND(yc-y),thick,&pattern);
		SetThickPixel(ROUND(xc-x),ROUND(yc+y),thick,&pattern);
		SetThickPixel(ROUND(xc+x),ROUND(yc+y),thick,&pattern);
	}

	p = ROUND(Ry2*(x+.5)*(x+.5) + Rx2*(y-1)*(y-1) - Rx2*Ry2);
	while (y>0)
	{
		y--;
		py -= twoRx2;
		if (p>0)
			p += Rx2 - py;
		else
		{
			x++;
			px += twoRy2;
			p += Rx2 - py +px;
		}

		SetThickPixel(ROUND(xc+x),ROUND(yc-y),thick,&pattern);
		SetThickPixel(ROUND(xc-x),ROUND(yc-y),thick,&pattern);
		SetThickPixel(ROUND(xc-x),ROUND(yc+y),thick,&pattern);
		SetThickPixel(ROUND(xc+x),ROUND(yc+y),thick,&pattern);
	}
	Unlock();
}

/*!
	\brief Draws a line. Really.
	\param start Starting point
	\param end Ending point
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.
	
	The endpoints themselves are guaranteed to be in bounds, but clipping for lines with
	a thickness greater than 1 will need to be done.
	This is capable of handling lines with invalid points.
*/
void AccelerantDriver::StrokeLine(BPoint start, BPoint end, LayerData *d, const Pattern &pat)
{
	int x1 = ROUND(start.x);
	int y1 = ROUND(start.y);
	int x2 = ROUND(end.x);
	int y2 = ROUND(end.y);
	int dx = x2 - x1;
	int dy = y2 - y1;
	int steps, k;
	double xInc, yInc;
	double x = x1;
	double y = y1;
	int thick;

	Lock();
	thick = (int)d->pensize;
	PatternHandler pattern(pat);
	pattern.SetColors(d->highcolor, d->lowcolor);

	if ( abs(dx) > abs(dy) )
		steps = abs(dx);
	else
		steps = abs(dy);
	xInc = dx / (double) steps;
	yInc = dy / (double) steps;

        /* SetThickPixel does bounds checking, so we don't have to worry about it */
	SetThickPixel(ROUND(x),ROUND(y),thick,&pattern);
	for (k=0; k<steps; k++)
	{
		x += xInc;
		y += yInc;
		SetThickPixel(ROUND(x),ROUND(y),thick,&pattern);
	}
	Unlock();
}

/*!
	\brief Called for all BView::StrokePolygon calls
	\param ptlist Array of BPoints defining the polygon.
	\param numpts Number of points in the BPoint array.
	\param rect Rectangle which contains the polygon
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	The points in the array are not guaranteed to be within the framebuffer's 
	coordinate range.
*/
void AccelerantDriver::StrokePolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, const Pattern &pat, bool is_closed)
{
	/* Bounds checking is handled by StrokeLine and the functions it uses */
	Lock();
	for(int32 i=0; i<(numpts-1); i++)
		StrokeLine(ptlist[i],ptlist[i+1],d,pat);
	if(is_closed)
		StrokeLine(ptlist[numpts-1],ptlist[0],d,pat);
	Unlock();
}

/*!
	\brief Called for all BView::StrokeRect calls
	\param r BRect to be filled. Guaranteed to be in the frame buffer's coordinate space
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

*/
void AccelerantDriver::StrokeRect(BRect r, LayerData *d, const Pattern &pat)
{
	Lock();
	int thick = (int)d->pensize;
	PatternHandler pattern(pat);
	pattern.SetColors(d->highcolor, d->lowcolor);

	HLineThick(ROUND(r.left), ROUND(r.right), ROUND(r.top), thick, &pattern);
	StrokeLine(r.RightTop(), r.RightBottom(), d, pat);
	HLineThick(ROUND(r.right), ROUND(r.left), ROUND(r.bottom), thick, &pattern);
	StrokeLine(r.LeftTop(), r.LeftBottom(), d, pat);
	Unlock();
}

/*!
	\brief Called for all BView::StrokeRoundRect calls
	\param r The rect itself
	\param xrad X radius of the corner arcs
	\param yrad Y radius of the corner arcs
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the roundrect may end 
	up being clipped.
*/
void AccelerantDriver::StrokeRoundRect(BRect r, float xrad, float yrad, LayerData *d, const Pattern &pat)
{
	float hLeft, hRight;
	float vTop, vBottom;
	float bLeft, bRight, bTop, bBottom;
	Lock();
	int thick = (int)d->pensize;
	PatternHandler pattern(pat);
	pattern.SetColors(d->highcolor, d->lowcolor);

	hLeft = r.left + xrad;
	hRight = r.right - xrad;
	vTop = r.top + yrad;
	vBottom = r.bottom - yrad;
	bLeft = hLeft + xrad;
	bRight = hRight -xrad;
	bTop = vTop + yrad;
	bBottom = vBottom - yrad;
	StrokeArc(BRect(bRight, r.top, r.right, bTop), 0, 90, d, pat);
	HLineThick(ROUND(hRight), ROUND(hLeft), ROUND(r.top), thick, &pattern);
	
	StrokeArc(BRect(r.left,r.top,bLeft,bTop), 90, 90, d, pat);
	StrokeLine(BPoint(r.left,vTop),BPoint(r.left,vBottom),d,pat);

	StrokeArc(BRect(r.left,bBottom,bLeft,r.bottom), 180, 90, d, pat);
	HLineThick(ROUND(hLeft), ROUND(hRight), ROUND(r.bottom), thick, &pattern);

	StrokeArc(BRect(bRight,bBottom,r.right,r.bottom), 270, 90, d, pat);
	StrokeLine(BPoint(r.right,vBottom),BPoint(r.right,vTop),d,pat);
	Unlock();
}

//void AccelerantDriver::StrokeShape(SShape *sh, LayerData *d, const Pattern &pat)
//{
//}

/*!
	\brief Called for all BView::StrokeTriangle calls
	\param pts Array of 3 BPoints. Always non-NULL.
	\param r BRect enclosing the triangle. While it will definitely enclose the triangle,
	it may not be within the frame buffer's bounds.
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the triangle may end 
	up being clipped.
*/
void AccelerantDriver::StrokeTriangle(BPoint *pts, BRect r, LayerData *d, const Pattern &pat)
{
	/* Bounds checking is handled by StrokeLine and the functions it calls */
	Lock();
	StrokeLine(pts[0],pts[1],d,pat);
	StrokeLine(pts[1],pts[2],d,pat);
	StrokeLine(pts[2],pts[0],d,pat);
	Unlock();
}

/*!
	\brief Draws a series of lines - optimized for speed
	\param pts Array of BPoints pairs
	\param numlines Number of lines to be drawn
	\param colors Array of colors for each respective line
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	
	Data for this call is passed directly from userland - this call is responsible for all
	checking. All lines are to be processed in the call using the same LayerData settings
	for each line.
*/
void AccelerantDriver::StrokeLineArray(BPoint *pts, int32 numlines, RGBColor *colors, LayerData *d)
{
	Lock();
	
	Unlock();
}

/*!
	\brief Sets the screen mode to specified resolution and color depth.
	\param mode constant as defined in GraphicsDefs.h
	
	Subclasses must include calls to _SetDepth, _SetHeight, _SetWidth, and _SetMode
	to update the state variables kept internally by the DisplayDriver class.
*/
void AccelerantDriver::SetMode(int32 mode)
{
  /* TODO: Still needs some work to fine tune color hassles in picking the mode */
	set_display_mode SetDisplayMode = (set_display_mode)accelerant_hook(B_SET_DISPLAY_MODE, NULL);
	int proposed_width, proposed_height, proposed_depth;
	int i;

	Lock();

	if ( SetDisplayMode )
	{
		proposed_width = GetWidthFromMode(mode);
		proposed_height = GetHeightFromMode(mode);
		proposed_depth = GetDepthFromMode(mode);
		for (i=0; i<mode_count; i++)
		{
			if ( (proposed_width == mode_list[i].virtual_width) &&
				(proposed_height == mode_list[i].virtual_height) &&
				(proposed_depth == GetDepthFromColorspace(mode_list[i].space)) )
			{
				if ( SetDisplayMode(&(mode_list[i])) == B_OK )
				{
					memcpy(&mDisplayMode,&(mode_list[i]),sizeof(display_mode));
					_SetDepth(GetDepthFromColorspace(mDisplayMode.space));
					_SetWidth(mDisplayMode.virtual_width);
					_SetHeight(mDisplayMode.virtual_height);
					_SetMode(GetModeFromResolution(mDisplayMode.virtual_width,mDisplayMode.virtual_height,
						GetDepthFromColorspace(mDisplayMode.space)));
				}
				break;
			}
		}
	}

	Unlock();
}

/*!
	\brief Dumps the contents of the frame buffer to a file.
	\param path Path and leaf of the file to be created without an extension
	\return False if unimplemented or unsuccessful. True if otherwise.
	
	Subclasses should add an extension based on what kind of file is saved
*/
bool AccelerantDriver::DumpToFile(const char *path)
{
        /* TODO: impelement */
	return false;
}

/*!
	\brief Gets the width of a string in pixels
	\param string Source null-terminated string
	\param length Number of characters in the string
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\return Width of the string in pixels
	
	This corresponds to BView::StringWidth.
*/
float AccelerantDriver::StringWidth(const char *string, int32 length, LayerData *d)
{
	if(!string || !d)
		return 0.0;
	Lock();

	ServerFont *font=&(d->font);
	FontStyle *style=font->Style();

	if(!style)
	{
		Unlock();
		return 0.0;
	}

	FT_Face face;
	FT_GlyphSlot slot;
	FT_UInt glyph_index=0, previous=0;
	FT_Vector pen,delta;
	int16 error=0;
	int32 strlength,i;
	float returnval;

	error=FT_New_Face(ftlib, style->GetPath(), 0, &face);
	if(error)
	{
		Unlock();
		return 0.0;
	}

	slot=face->glyph;

	bool use_kerning=FT_HAS_KERNING(face) && font->Spacing()==B_STRING_SPACING;
	
	error=FT_Set_Char_Size(face, 0,int32(font->Size())*64,72,72);
	if(error)
	{
		Unlock();
		return 0.0;
	}

	// set the pen position in 26.6 cartesian space coordinates
	pen.x=0;
	
	slot=face->glyph;
	
	strlength=strlen(string);
	if(length<strlength)
		strlength=length;

	for(i=0;i<strlength;i++)
	{
		// get kerning and move pen
		if(use_kerning && previous && glyph_index)
		{
			FT_Get_Kerning(face, previous, glyph_index,ft_kerning_default, &delta);
			pen.x+=delta.x;
		}

		error=FT_Load_Char(face,string[i],FT_LOAD_MONOCHROME);

		// increment pen position
		pen.x+=slot->advance.x;
		previous=glyph_index;
	}

	FT_Done_Face(face);

	returnval=pen.x>>6;
	Unlock();
	return returnval;
}

/*!
	\brief Gets the height of a string in pixels
	\param string Source null-terminated string
	\param length Number of characters in the string
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\return Height of the string in pixels
	
	The height calculated in this function does not include any padding - just the
	precise maximum height of the characters within and does not necessarily equate
	with a font's height, i.e. the strings 'case' and 'alps' will have different values
	even when called with all other values equal.
*/
float AccelerantDriver::StringHeight(const char *string, int32 length, LayerData *d)
{
	if(!string || !d)
		return 0.0;
	Lock();

	ServerFont *font=&(d->font);
	FontStyle *style=font->Style();

	if(!style)
	{
		Unlock();
		return 0.0;
	}

	FT_Face face;
	FT_GlyphSlot slot;
	int16 error=0;
	int32 strlength,i;
	float returnval=0.0,ascent=0.0,descent=0.0;

	error=FT_New_Face(ftlib, style->GetPath(), 0, &face);
	if(error)
	{
		Unlock();
		return 0.0;
	}

	slot=face->glyph;
	
	error=FT_Set_Char_Size(face, 0,int32(font->Size())*64,72,72);
	if(error)
	{
		Unlock();
		return 0.0;
	}

	slot=face->glyph;
	
	strlength=strlen(string);
	if(length<strlength)
		strlength=length;

	for(i=0;i<strlength;i++)
	{
		FT_Load_Char(face,string[i],FT_LOAD_RENDER);
		if(slot->metrics.horiBearingY<slot->metrics.height)
			descent=MAX((slot->metrics.height-slot->metrics.horiBearingY)>>6,descent);
		else
			ascent=MAX(slot->bitmap.rows,ascent);
	}
	Unlock();

	FT_Done_Face(face);

	returnval=ascent+descent;
	Unlock();
	return returnval;
}

/*!
	\brief Retrieves the bounding box each character in the string
	\param string Source null-terminated string
	\param count Number of characters in the string
	\param mode Metrics mode for either screen or printing
	\param delta Optional glyph padding. This value may be NULL.
	\param rectarray Array of BRect objects which will have at least count elements
	\param d Data structure containing any other data necessary for the call. Always non-NULL.

	See BFont::GetBoundingBoxes for more details on this function.
*/
void AccelerantDriver::GetBoundingBoxes(const char *string, int32 count, 
		font_metric_mode mode, escapement_delta *delta, BRect *rectarray, LayerData *d)
{
}

/*!
	\brief Retrieves the escapements for each character in the string
	\param string Source null-terminated string
	\param charcount Number of characters in the string
	\param delta Optional glyph padding. This value may be NULL.
	\param escapements Array of escapement_delta objects which will have at least charcount elements
	\param offsets Actual offset values when iterating over the string. This array will also 
		have at least charcount elements and the values placed therein will reflect 
		the current kerning/spacing mode.
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	
	See BFont::GetEscapements for more details on this function.
*/
void AccelerantDriver::GetEscapements(const char *string, int32 charcount, 
		escapement_delta *delta, escapement_delta *escapements, escapement_delta *offsets, LayerData *d)
{
}

/*!
	\brief Retrieves the inset values of each glyph from its escapement values
	\param string Source null-terminated string
	\param charcount Number of characters in the string
	\param edgearray Array of edge_info objects which will have at least charcount elements
	\param d Data structure containing any other data necessary for the call. Always non-NULL.

	See BFont::GetEdges for more details on this function.
*/
void AccelerantDriver::GetEdges(const char *string, int32 charcount, edge_info *edgearray, LayerData *d)
{
}

/*!
	\brief Determines whether a font contains a certain string of characters
	\param string Source null-terminated string
	\param charcount Number of characters in the string
	\param hasarray Array of booleans which will have at least charcount elements

	See BFont::GetHasGlyphs for more details on this function.
*/
void AccelerantDriver::GetHasGlyphs(const char *string, int32 charcount, bool *hasarray)
{
}

/*!
	\brief Truncates an array of strings to a certain width
	\param instrings Array of null-terminated strings
	\param stringcount Number of strings passed to the function
	\param mode Truncation mode
	\param maxwidth Maximum width for all strings
	\param outstrings String array provided by the caller into which the truncated strings are
		to be placed.

	See BFont::GetTruncatedStrings for more details on this function.
*/
void AccelerantDriver::GetTruncatedStrings( const char **instrings, int32 stringcount, 
	uint32 mode, float maxwidth, char **outstrings)
{
}

/*!
	\brief Draws a pixel in the specified color
	\param x The x coordinate (guaranteed to be in bounds)
	\param y The y coordinate (guaranteed to be in bounds)
	\param col The color to draw
*/
void AccelerantDriver::SetPixel(int x, int y, RGBColor col)
{
	switch (mDisplayMode.space)
	{
		case B_CMAP8:
		case B_GRAY8:
			{
				uint8 *fb = (uint8 *)mFrameBufferConfig.frame_buffer + y*mFrameBufferConfig.bytes_per_row;
				fb[x] = col.GetColor8();
			} break;
		case B_RGB16_BIG:
		case B_RGB16_LITTLE:
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_RGB15_LITTLE:
		case B_RGBA15_LITTLE:
			{
				uint16 *fb = (uint16 *)((uint8 *)mFrameBufferConfig.frame_buffer + y*mFrameBufferConfig.bytes_per_row);
				fb[x] = col.GetColor16();
			} break;
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGB32_LITTLE:
		case B_RGBA32_LITTLE:
			{
				uint32 *fb = (uint32 *)((uint8 *)mFrameBufferConfig.frame_buffer + y*mFrameBufferConfig.bytes_per_row);
				rgb_color color = col.GetColor32();
				fb[x] = (color.alpha << 24) | (color.red << 16) | (color.green << 8) | (color.blue);
			} break;
		default:
			printf("Error: Unknown color space\n");
	}
}

/*!
	\brief Draws a point of a specified thickness
	\param x The x coordinate (not guaranteed to be in bounds)
	\param y The y coordinate (not guaranteed to be in bounds)
	\param thick The thickness of the point
	\param pat The PatternHandler which detemines pixel colors
*/
void AccelerantDriver::SetThickPixel(int x, int y, int thick, PatternHandler *pat)
{
	if ( (!CHECK_X(x-thick/2) && !CHECK_X(x+thick/2)) ||
		(!CHECK_Y(y-thick/2) && !CHECK_Y(y+thick/2)) )
		return;
	int left, right, top, bottom;
	left = CLIP_X(x-thick/2);
	right = CLIP_X(x+thick/2);
	top = CLIP_Y(y-thick/2);
	bottom = CLIP_Y(y+thick/2);
	switch (mDisplayMode.space)
	{
		case B_CMAP8:
		case B_GRAY8:
			{
				int x,y;
				uint8 *fb = (uint8 *)mFrameBufferConfig.frame_buffer + top*mFrameBufferConfig.bytes_per_row;
				for (y=top; y<=bottom; y++)
				{
					for (x=left; x<=right; x++)
					{
						fb[x] = pat->GetColor(x,y).GetColor8();;
					}
					fb += mFrameBufferConfig.bytes_per_row;
				}
			} break;
		case B_RGB16_BIG:
		case B_RGB16_LITTLE:
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_RGB15_LITTLE:
		case B_RGBA15_LITTLE:
			{
				int x,y;
				uint16 *fb = (uint16 *)((uint8 *)mFrameBufferConfig.frame_buffer + top*mFrameBufferConfig.bytes_per_row);
				for (y=top; y<=bottom; y++)
				{
					for (x=left; x<=right; x++)
					{
						fb[x] = pat->GetColor(x,y).GetColor16();
					}
					fb = (uint16 *)((uint8 *)fb + mFrameBufferConfig.bytes_per_row);
				}
			} break;
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGB32_LITTLE:
		case B_RGBA32_LITTLE:
			{
				int x,y;
				uint32 *fb = (uint32 *)((uint8 *)mFrameBufferConfig.frame_buffer + top*mFrameBufferConfig.bytes_per_row);
				rgb_color color;
				for (y=top; y<=bottom; y++)
				{
					for (x=left; x<=right; x++)
					{
						color = pat->GetColor(x,y).GetColor32();
						fb[x] = (color.alpha << 24) | (color.red << 16) | (color.green << 8) | (color.blue);
					}
					fb = (uint32 *)((uint8 *)fb + mFrameBufferConfig.bytes_per_row);
				}
			} break;
		default:
			printf("Error: Unknown color space\n");
	}
}

/*!
	\brief Draws a horizontal line
	\param x1 The first x coordinate (guaranteed to be in bounds)
	\param x2 The second x coordinate (guaranteed to be in bounds)
	\param y The y coordinate (guaranteed to be in bounds)
	\param pat The PatternHandler which detemines pixel colors
*/
void AccelerantDriver::HLine(int32 x1, int32 x2, int32 y, PatternHandler *pat)
{
	/* TODO: Add hardware acceleration */
	int x;
	if ( x1 > x2 )
	{
		x = x2;
		x2 = x1;
		x1 = x;
	}
	switch (mDisplayMode.space)
	{
		case B_CMAP8:
		case B_GRAY8:
			{
				uint8 *fb = (uint8 *)mFrameBufferConfig.frame_buffer + y*mFrameBufferConfig.bytes_per_row;
				for (x=x1; x<=x2; x++)
					fb[x] = pat->GetColor(x,y).GetColor8();
			} break;
		case B_RGB16_BIG:
		case B_RGB16_LITTLE:
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_RGB15_LITTLE:
		case B_RGBA15_LITTLE:
			{
				uint16 *fb = (uint16 *)((uint8 *)mFrameBufferConfig.frame_buffer + y*mFrameBufferConfig.bytes_per_row);
				for (x=x1; x<=x2; x++)
					fb[x] = pat->GetColor(x,y).GetColor16();
			} break;
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGB32_LITTLE:
		case B_RGBA32_LITTLE:
			{
				uint32 *fb = (uint32 *)((uint8 *)mFrameBufferConfig.frame_buffer + y*mFrameBufferConfig.bytes_per_row);
				rgb_color color;
				for (x=x1; x<=x2; x++)
				{
					color = pat->GetColor(x,y).GetColor32();
					fb[x] = (color.alpha << 24) | (color.red << 16) | (color.green << 8) | (color.blue);
				}
			} break;
		default:
			printf("Error: Unknown color space\n");
	}
}

/*!
	\brief Draws a horizontal line
	\param x1 The first x coordinate (not guaranteed to be in bounds)
	\param x2 The second x coordinate (not guaranteed to be in bounds)
	\param y The y coordinate (not guaranteed to be in bounds)
	\param thick The thickness of the line
	\param pat The PatternHandler which detemines pixel colors
*/
void AccelerantDriver::HLineThick(int32 x1, int32 x2, int32 y, int32 thick, PatternHandler *pat)
{
	/* TODO: Add hardware acceleration */
	int x, y1, y2;

	if ( x1 > x2 )
	{
		x = x2;
		x2 = x1;
		x1 = x;
	}
	y1 = y-thick/2;
	y2 = y+thick/2;
	if ( (x2 < 0) || (x1 >= mDisplayMode.virtual_width) || (!CHECK_Y(y1) && !CHECK_Y(y2)) )
		return;
	x1 = CLIP_X(x1);
	x2 = CLIP_X(x2);
	y1 = CLIP_Y(y1);
	y2 = CLIP_Y(y2);
	switch (mDisplayMode.space)
	{
		case B_CMAP8:
		case B_GRAY8:
			{
				uint8 *fb = (uint8 *)mFrameBufferConfig.frame_buffer + y1*mFrameBufferConfig.bytes_per_row;
				for (y=y1; y<=y2; y++)
				{
					for (x=x1; x<=x2; x++)
						fb[x] = pat->GetColor(x,y).GetColor8();
					fb += mFrameBufferConfig.bytes_per_row;
				}
			} break;
		case B_RGB16_BIG:
		case B_RGB16_LITTLE:
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_RGB15_LITTLE:
		case B_RGBA15_LITTLE:
			{
				uint16 *fb = (uint16 *)((uint8 *)mFrameBufferConfig.frame_buffer + y*mFrameBufferConfig.bytes_per_row);
				for (y=y1; y<=y2; y++)
				{
					for (x=x1; x<=x2; x++)
						fb[x] = pat->GetColor(x,y).GetColor16();
					fb = (uint16 *)((uint8 *)fb + mFrameBufferConfig.bytes_per_row);
				}
			} break;
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGB32_LITTLE:
		case B_RGBA32_LITTLE:
			{
				uint32 *fb = (uint32 *)((uint8 *)mFrameBufferConfig.frame_buffer + y*mFrameBufferConfig.bytes_per_row);
				rgb_color color;
				for (y=y1; y<=y2; y++)
				{
					for (x=x1; x<=x2; x++)
					{
						color = pat->GetColor(x,y).GetColor32();
						fb[x] = (color.alpha << 24) | (color.red << 16) | (color.green << 8) | (color.blue);
					}
					fb = (uint32 *)((uint8 *)fb + mFrameBufferConfig.bytes_per_row);
				}
			} break;
		default:
			printf("Error: Unknown color space\n");
	}
}

/*!
	\brief Copies a bitmap to the screen
	\param sourcebmp The bitmap containing the data to blit to the screen
	\param sourcerect The rectangle defining the section of the bitmap to blit
	\param destrect The rectangle defining the section of the screen to be blitted
	\param mode The drawing mode to use when blitting the bitmap
	The bitmap and the screen must have the same color depth, or this will do nothing.
*/
void AccelerantDriver::BlitBitmap(ServerBitmap *sourcebmp, BRect sourcerect, BRect destrect, drawing_mode mode)
{
	/* TODO: Need to check for hardware support for this. */
	if(!sourcebmp)
		return;

	if(sourcebmp->BitsPerPixel() != GetDepthFromColorspace(mDisplayMode.space))
		return;

	uint8 colorspace_size=sourcebmp->BitsPerPixel()/8;
	// First, clip source rect to destination
	if(sourcerect.Width() > destrect.Width())
		sourcerect.right=sourcerect.left+destrect.Width();
	

	if(sourcerect.Height() > destrect.Height())
		sourcerect.bottom=sourcerect.top+destrect.Height();
	

	// Second, check rectangle bounds against their own bitmaps
	BRect work_rect;

	work_rect.Set(	sourcebmp->Bounds().left,
					sourcebmp->Bounds().top,
					sourcebmp->Bounds().right,
					sourcebmp->Bounds().bottom	);
	if( !(work_rect.Contains(sourcerect)) )
	{	// something in selection must be clipped
		if(sourcerect.left < 0)
			sourcerect.left = 0;
		if(sourcerect.right > work_rect.right)
			sourcerect.right = work_rect.right;
		if(sourcerect.top < 0)
			sourcerect.top = 0;
		if(sourcerect.bottom > work_rect.bottom)
			sourcerect.bottom = work_rect.bottom;
	}

	work_rect.Set(0,0,mDisplayMode.virtual_width-1,mDisplayMode.virtual_height-1);

	if( !(work_rect.Contains(destrect)) )
	{	// something in selection must be clipped
		if(destrect.left < 0)
			destrect.left = 0;
		if(destrect.right > work_rect.right)
			destrect.right = work_rect.right;
		if(destrect.top < 0)
			destrect.top = 0;
		if(destrect.bottom > work_rect.bottom)
			destrect.bottom = work_rect.bottom;
	}

	// Set pointers to the actual data
	uint8 *src_bits  = (uint8*) sourcebmp->Bits();	
	uint8 *dest_bits = (uint8*) mFrameBufferConfig.frame_buffer;

	// Get row widths for offset looping
	uint32 src_width  = uint32 (sourcebmp->BytesPerRow());
	uint32 dest_width = uint32 (mFrameBufferConfig.bytes_per_row);

	// Offset bitmap pointers to proper spot in each bitmap
	src_bits += uint32 ( (sourcerect.top  * src_width)  + (sourcerect.left  * colorspace_size) );
	dest_bits += uint32 ( (destrect.top * dest_width) + (destrect.left * colorspace_size) );

	uint32 line_length = uint32 ((destrect.right - destrect.left+1)*colorspace_size);
	uint32 lines = uint32 (destrect.bottom-destrect.top+1);

	switch(mode)
	{
		case B_OP_OVER:
		{
			uint32 srow_pixels=src_width>>2;
			uint8 *srow_index, *drow_index;
			
			
			// This could later be optimized to use uint32's for faster copying
			for (uint32 pos_y=0; pos_y!=lines; pos_y++)
			{
				
				srow_index=src_bits;
				drow_index=dest_bits;
				
				for(uint32 pos_x=0; pos_x!=srow_pixels;pos_x++)
				{
					// 32-bit RGBA32 mode byte order is BGRA
					if(srow_index[3]>127)
					{
						*drow_index=*srow_index; drow_index++; srow_index++;
						*drow_index=*srow_index; drow_index++; srow_index++;
						*drow_index=*srow_index; drow_index++; srow_index++;
						// we don't copy the alpha channel
						drow_index++; srow_index++;
					}
					else
					{
						srow_index+=4;
						drow_index+=4;
					}
				}
		
				// Increment offsets
		   		src_bits += src_width;
		   		dest_bits += dest_width;
			}
			break;
		}
		default:	// B_OP_COPY
		{
			for (uint32 pos_y = 0; pos_y != lines; pos_y++)
			{
				memcpy(dest_bits,src_bits,line_length);
		
				// Increment offsets
		   		src_bits += src_width;
		   		dest_bits += dest_width;
			}
			break;
		}
	}

}

/*!
	\brief Copies a bitmap to the screen
	\param destbmp The bitmap receing the data from the screen
	\param destrect The rectangle defining the section of the bitmap to receive data
	\param sourcerect The rectangle defining the section of the screen to be copied
	The bitmap and the screen must have the same color depth or this will do nothing.
*/
void AccelerantDriver::ExtractToBitmap(ServerBitmap *destbmp, BRect destrect, BRect sourcerect)
{
	/* TODO: Need to check for hardware support for this. */
	if(!destbmp)
		return;

	if(destbmp->BitsPerPixel() != GetDepthFromColorspace(mDisplayMode.space))
		return;

	uint8 colorspace_size=destbmp->BitsPerPixel()/8;
	// First, clip source rect to destination
	if(sourcerect.Width() > destrect.Width())
		sourcerect.right=sourcerect.left+destrect.Width();
	

	if(sourcerect.Height() > destrect.Height())
		sourcerect.bottom=sourcerect.top+destrect.Height();
	

	// Second, check rectangle bounds against their own bitmaps
	BRect work_rect;

	work_rect.Set(	destbmp->Bounds().left,
					destbmp->Bounds().top,
					destbmp->Bounds().right,
					destbmp->Bounds().bottom	);
	if( !(work_rect.Contains(destrect)) )
	{	// something in selection must be clipped
		if(destrect.left < 0)
			destrect.left = 0;
		if(destrect.right > work_rect.right)
			destrect.right = work_rect.right;
		if(destrect.top < 0)
			destrect.top = 0;
		if(destrect.bottom > work_rect.bottom)
			destrect.bottom = work_rect.bottom;
	}

	work_rect.Set(0,0,mDisplayMode.virtual_width-1,mDisplayMode.virtual_height-1);

	if( !(work_rect.Contains(sourcerect)) )
	{	// something in selection must be clipped
		if(sourcerect.left < 0)
			sourcerect.left = 0;
		if(sourcerect.right > work_rect.right)
			sourcerect.right = work_rect.right;
		if(sourcerect.top < 0)
			sourcerect.top = 0;
		if(sourcerect.bottom > work_rect.bottom)
			sourcerect.bottom = work_rect.bottom;
	}

	// Set pointers to the actual data
	uint8 *dest_bits  = (uint8*) destbmp->Bits();	
	uint8 *src_bits = (uint8*) mFrameBufferConfig.frame_buffer;

	// Get row widths for offset looping
	uint32 dest_width  = uint32 (destbmp->BytesPerRow());
	uint32 src_width = uint32 (mFrameBufferConfig.bytes_per_row);

	// Offset bitmap pointers to proper spot in each bitmap
	src_bits += uint32 ( (sourcerect.top  * src_width)  + (sourcerect.left  * colorspace_size) );
	dest_bits += uint32 ( (destrect.top * dest_width) + (destrect.left * colorspace_size) );

	uint32 line_length = uint32 ((destrect.right - destrect.left+1)*colorspace_size);
	uint32 lines = uint32 (destrect.bottom-destrect.top+1);

	for (uint32 pos_y = 0; pos_y != lines; pos_y++)
	{ 
		memcpy(dest_bits,src_bits,line_length);

		// Increment offsets
   		src_bits += src_width;
   		dest_bits += dest_width;
	}
}

/*!
	\brief Opens a graphics device for read-write access
	\param deviceNumber Number identifying which graphics card to open (1 for first card)
	\return The file descriptor for the opened graphics device
	
	The deviceNumber is relative to the number of graphics devices that can be successfully
	opened.  One represents the first card that can be successfully opened (not necessarily
	the first one listed in the directory).
*/
int AccelerantDriver::OpenGraphicsDevice(int deviceNumber)
{
	int current_card_fd = -1;
	int count = 0;
	char path[PATH_MAX];
	DIR *directory;
	struct dirent *entry;

	directory = opendir("/dev/graphics");
	if ( !directory )
		return -1;
	while ( (count < deviceNumber) && ((entry = readdir(directory)) != NULL) )
	{
		if ( !strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..") ||
			!strcmp(entry->d_name, "stub") )
			continue;
		if (current_card_fd >= 0)
		{
			close(current_card_fd);
			current_card_fd = -1;
		}
		sprintf(path,"/dev/graphics/%s",entry->d_name);
		current_card_fd = open(path,B_READ_WRITE);
		if ( current_card_fd >= 0 )
			count++;
	}
	if ( (count < deviceNumber) && (current_card_fd >= 0) )
	{
		if ( deviceNumber == 1 )
		{
			close(current_card_fd);
			sprintf(path,"/dev/graphics/stub");
			current_card_fd = open(path,B_READ_WRITE);
		}
		else
		{
			close(current_card_fd);
			current_card_fd = -1;
		}
	}

	return current_card_fd;
}

/*!
	\brief Determines a display mode constant from display size and depth
	\param width The display width
	\param height The display height
	\param depth The color depth
	\return The display mode constant
*/
int AccelerantDriver::GetModeFromResolution(int width, int height, int depth)
{
	int mode = 0;
	switch (depth)
	{
		case 8:
			switch (width)
			{
				case 640:
					if ( height == 400 )
						mode = B_8_BIT_640x400;
					else
						mode = B_8_BIT_640x480;
					break;
				case 800:
					mode = B_8_BIT_800x600;
					break;
				case 1024:
					mode = B_8_BIT_1024x768;
					break;
				case 1152:
					mode = B_8_BIT_1152x900;
					break;
				case 1280:
					mode = B_8_BIT_1280x1024;
					break;
				case 1600:
					mode = B_8_BIT_1600x1200;
					break;
			}
			break;
		case 15:
			switch (width)
			{
				case 640:
					mode = B_15_BIT_640x480;
					break;
				case 800:
					mode = B_15_BIT_800x600;
					break;
				case 1024:
					mode = B_15_BIT_1024x768;
					break;
				case 1152:
					mode = B_15_BIT_1152x900;
					break;
				case 1280:
					mode = B_15_BIT_1280x1024;
					break;
				case 1600:
					mode = B_15_BIT_1600x1200;
					break;
			}
			break;
		case 16:
			switch (width)
			{
				case 640:
					mode = B_16_BIT_640x480;
					break;
				case 800:
					mode = B_16_BIT_800x600;
					break;
				case 1024:
					mode = B_16_BIT_1024x768;
					break;
				case 1152:
					mode = B_16_BIT_1152x900;
					break;
				case 1280:
					mode = B_16_BIT_1280x1024;
					break;
				case 1600:
					mode = B_16_BIT_1600x1200;
					break;
			}
			break;
		case 32:
			switch (width)
			{
				case 640:
					mode = B_32_BIT_640x480;
					break;
				case 800:
					mode = B_32_BIT_800x600;
					break;
				case 1024:
					mode = B_32_BIT_1024x768;
					break;
				case 1152:
					mode = B_32_BIT_1152x900;
					break;
				case 1280:
					mode = B_32_BIT_1280x1024;
					break;
				case 1600:
					mode = B_32_BIT_1600x1200;
					break;
			}
			break;
	}
	return mode;
}

/*!
	\brief Determines the display width from a display mode constant
	\param mode The display mode
	\return The display height (640, 800, 1024, 1152, 1280, or 1600)
*/
int AccelerantDriver::GetWidthFromMode(int mode)
{
	int width=0;

	switch (mode)
	{
		case B_8_BIT_640x400:
		case B_8_BIT_640x480:
		case B_15_BIT_640x480:
		case B_16_BIT_640x480:
		case B_32_BIT_640x480:
			width = 640;
			break;
		case B_8_BIT_800x600:
		case B_15_BIT_800x600:
		case B_16_BIT_800x600:
		case B_32_BIT_800x600:
			width = 800;
			break;
		case B_8_BIT_1024x768:
		case B_15_BIT_1024x768:
		case B_16_BIT_1024x768:
		case B_32_BIT_1024x768:
			width = 1024;
			break;
		case B_8_BIT_1152x900:
		case B_15_BIT_1152x900:
		case B_16_BIT_1152x900:
		case B_32_BIT_1152x900:
			width = 1152;
			break;
		case B_8_BIT_1280x1024:
		case B_15_BIT_1280x1024:
		case B_16_BIT_1280x1024:
		case B_32_BIT_1280x1024:
			width = 1280;
			break;
		case B_8_BIT_1600x1200:
		case B_15_BIT_1600x1200:
		case B_16_BIT_1600x1200:
		case B_32_BIT_1600x1200:
			width = 1600;
			break;
	}
	return width;
}

/*!
	\brief Determines the display height from a display mode constant
	\param mode The display mode
	\return The display height (400, 480, 600, 768, 900, 1024, or 1200)
*/
int AccelerantDriver::GetHeightFromMode(int mode)
{
	int height=0;

	switch (mode)
	{
		case B_8_BIT_640x400:
			height = 400;
			break;
		case B_8_BIT_640x480:
		case B_15_BIT_640x480:
		case B_16_BIT_640x480:
		case B_32_BIT_640x480:
			height = 480;
			break;
		case B_8_BIT_800x600:
		case B_15_BIT_800x600:
		case B_16_BIT_800x600:
		case B_32_BIT_800x600:
			height = 600;
			break;
		case B_8_BIT_1024x768:
		case B_15_BIT_1024x768:
		case B_16_BIT_1024x768:
		case B_32_BIT_1024x768:
			height = 768;
			break;
		case B_8_BIT_1152x900:
		case B_15_BIT_1152x900:
		case B_16_BIT_1152x900:
		case B_32_BIT_1152x900:
			height = 900;
			break;
		case B_8_BIT_1280x1024:
		case B_15_BIT_1280x1024:
		case B_16_BIT_1280x1024:
		case B_32_BIT_1280x1024:
			height = 1024;
			break;
		case B_8_BIT_1600x1200:
		case B_15_BIT_1600x1200:
		case B_16_BIT_1600x1200:
		case B_32_BIT_1600x1200:
			height = 1200;
			break;
	}
	return height;
}

/*!
	\brief Determines the color depth from a display mode constant
	\param mode The display mode
	\return The color depth (8,15,16 or 32)
*/
int AccelerantDriver::GetDepthFromMode(int mode)
{
	int depth=0;

	switch (mode)
	{
		case B_8_BIT_640x400:
		case B_8_BIT_640x480:
		case B_8_BIT_800x600:
		case B_8_BIT_1024x768:
		case B_8_BIT_1152x900:
		case B_8_BIT_1280x1024:
		case B_8_BIT_1600x1200:
			depth = 8;
			break;
		case B_15_BIT_640x480:
		case B_15_BIT_800x600:
		case B_15_BIT_1024x768:
		case B_15_BIT_1152x900:
		case B_15_BIT_1280x1024:
		case B_15_BIT_1600x1200:
			depth = 15;
			break;
		case B_16_BIT_640x480:
		case B_16_BIT_800x600:
		case B_16_BIT_1024x768:
		case B_16_BIT_1152x900:
		case B_16_BIT_1280x1024:
		case B_16_BIT_1600x1200:
			depth = 16;
			break;
		case B_32_BIT_640x480:
		case B_32_BIT_800x600:
		case B_32_BIT_1024x768:
		case B_32_BIT_1152x900:
		case B_32_BIT_1280x1024:
		case B_32_BIT_1600x1200:
			depth = 32;
			break;
	}
	return depth;
}

/*!
	\brief Determines the color depth from a color space constant
	\param mode The color space constant
	\return The color depth (1,8,16 or 32)
*/
int AccelerantDriver::GetDepthFromColorspace(int space)
{
	int depth=0;

	switch (space)
	{
		case B_GRAY1:
			depth = 1;
			break;
		case B_CMAP8:
		case B_GRAY8:
			depth = 8;
			break;
		case B_RGB16:
		case B_RGB15:
		case B_RGBA15:
		case B_RGB16_BIG:
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
			depth = 16;
			break;
		case B_RGB32:
		case B_RGBA32:
		case B_RGB24:
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGB24_BIG:
			depth = 32;
			break;
	}
	return depth;
}
