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
#define DRAW_TEST

/* TODO: Add handling of draw modes */

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

#ifdef DRAW_TEST
	RGBColor red(255,0,0,0);
	RGBColor green(0,255,0,0);
	RGBColor blue(0,0,255,0);
	FillRect(BRect(0,0,1024,768),blue);
#endif
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


void AccelerantDriver::FillArc(const BRect r, float angle, float span, RGBColor& color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	DisplayDriver::FillArc(r,angle,span,this,(SetHorizontalLineFuncType)&AccelerantDriver::HLinePatternThick);
	Unlock();
}

void AccelerantDriver::FillArc(const BRect r, float angle, float span, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::FillArc(r,angle,span,this,(SetHorizontalLineFuncType)&AccelerantDriver::HLinePatternThick);
	Unlock();
}

void AccelerantDriver::FillBezier(BPoint *pts, RGBColor& color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	DisplayDriver::FillBezier(pts,this,(SetHorizontalLineFuncType)&AccelerantDriver::HLinePatternThick);
	Unlock();
}

void AccelerantDriver::FillBezier(BPoint *pts, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::FillBezier(pts,this,(SetHorizontalLineFuncType)&AccelerantDriver::HLinePatternThick);
	Unlock();
}

void AccelerantDriver::FillEllipse(BRect r, RGBColor& color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	DisplayDriver::FillEllipse(r,this,(SetHorizontalLineFuncType)&AccelerantDriver::HLinePatternThick);
	Unlock();
}

void AccelerantDriver::FillEllipse(BRect r, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::FillEllipse(r,this,(SetHorizontalLineFuncType)&AccelerantDriver::HLinePatternThick);
	Unlock();
}

void AccelerantDriver::FillPolygon(BPoint *ptlist, int32 numpts, RGBColor& color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	DisplayDriver::FillPolygon(ptlist,numpts,this,(SetHorizontalLineFuncType)&AccelerantDriver::HLinePatternThick);
	Unlock();
}

void AccelerantDriver::FillPolygon(BPoint *ptlist, int32 numpts, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::FillPolygon(ptlist,numpts,this,(SetHorizontalLineFuncType)&AccelerantDriver::HLinePatternThick);
	Unlock();
}

void AccelerantDriver::FillRect(const BRect r, RGBColor& color)
{
	Lock();
	fDrawColor = color;
	FillSolidRect((int32)r.left,(int32)r.top,(int32)r.right,(int32)r.bottom);
	Unlock();
}

/*!
	\brief Called for all BView::FillRect calls
	\param r BRect to be filled. Guaranteed to be in the frame buffer's coordinate space
	\param pattern The pattern to be used when filling the rectangle
	\param high_color The high color of the pattern to fill
	\param low_color  The low color of the pattern to fill
*/
void AccelerantDriver::FillRect(const BRect r, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	FillPatternRect((int32)r.left,(int32)r.top,(int32)r.right,(int32)r.bottom);
	Unlock();
}

void AccelerantDriver::FillRoundRect(BRect r, float xrad, float yrad, RGBColor& color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	fDrawColor = color;
	DisplayDriver::FillRoundRect(r,xrad,yrad,this,(SetRectangleFuncType)&AccelerantDriver::FillSolidRect,(SetHorizontalLineFuncType)&AccelerantDriver::HLinePatternThick);
	Unlock();
}

void AccelerantDriver::FillRoundRect(BRect r, float xrad, float yrad, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::FillRoundRect(r,xrad,yrad,this,(SetRectangleFuncType)&AccelerantDriver::FillPatternRect,(SetHorizontalLineFuncType)&AccelerantDriver::HLinePatternThick);
	Unlock();
}

void AccelerantDriver::FillTriangle(BPoint *pts, RGBColor& color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	DisplayDriver::FillTriangle(pts,this,(SetHorizontalLineFuncType)&AccelerantDriver::HLinePatternThick);
	Unlock();
}

void AccelerantDriver::FillTriangle(BPoint *pts, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::FillTriangle(pts,this,(SetHorizontalLineFuncType)&AccelerantDriver::HLinePatternThick);
	Unlock();
}

void AccelerantDriver::StrokeArc(BRect r, float angle, float span, float pensize, RGBColor& color)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	DisplayDriver::StrokeArc(r,angle,span,this,(SetPixelFuncType)&AccelerantDriver::SetThickPatternPixel);
	Unlock();
}

void AccelerantDriver::StrokeArc(BRect r, float angle, float span, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::StrokeArc(r,angle,span,this,(SetPixelFuncType)&AccelerantDriver::SetThickPatternPixel);
	Unlock();
}

void AccelerantDriver::StrokeBezier(BPoint *pts, float pensize, RGBColor& color)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	DisplayDriver::StrokeBezier(pts,this,(SetPixelFuncType)&AccelerantDriver::SetThickPatternPixel);
	Unlock();
}

void AccelerantDriver::StrokeBezier(BPoint *pts, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::StrokeBezier(pts,this,(SetPixelFuncType)&AccelerantDriver::SetThickPatternPixel);
	Unlock();
}

void AccelerantDriver::StrokeEllipse(BRect r, float pensize, RGBColor& color)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	DisplayDriver::StrokeEllipse(r,this,(SetPixelFuncType)&AccelerantDriver::SetThickPatternPixel);
	Unlock();
}

void AccelerantDriver::StrokeEllipse(BRect r, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::StrokeEllipse(r,this,(SetPixelFuncType)&AccelerantDriver::SetThickPatternPixel);
	Unlock();
}

void AccelerantDriver::StrokeLine(BPoint start, BPoint end, float pensize, RGBColor& color)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	DisplayDriver::StrokeLine(start,end,this,(SetPixelFuncType)&AccelerantDriver::SetThickPatternPixel);
	Unlock();
}

void AccelerantDriver::StrokeLine(BPoint start, BPoint end, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::StrokeLine(start,end,this,(SetPixelFuncType)&AccelerantDriver::SetThickPatternPixel);
	Unlock();
}

void AccelerantDriver::StrokePoint(BPoint& pt, RGBColor& color)
{
	Lock();
	fLineThickness = 1;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	SetThickPatternPixel((int)pt.x,(int)pt.y);
	Unlock();
}

void AccelerantDriver::StrokePolygon(BPoint *ptlist, int32 numpts, float pensize, RGBColor& color, bool is_closed)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	DisplayDriver::StrokePolygon(ptlist,numpts,this,(SetPixelFuncType)&AccelerantDriver::SetThickPatternPixel,is_closed);
	Unlock();
}

void AccelerantDriver::StrokePolygon(BPoint *ptlist, int32 numpts, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, bool is_closed)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::StrokePolygon(ptlist,numpts,this,(SetPixelFuncType)&AccelerantDriver::SetThickPatternPixel);
	Unlock();
}

void AccelerantDriver::StrokeRect(BRect r, float pensize, RGBColor& color)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	DisplayDriver::StrokeRect(r,this,(SetHorizontalLineFuncType)&AccelerantDriver::HLinePatternThick,(SetVerticalLineFuncType)&AccelerantDriver::VLinePatternThick);
	Unlock();
}

void AccelerantDriver::StrokeRect(BRect r, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::StrokeRect(r,this,(SetHorizontalLineFuncType)&AccelerantDriver::HLinePatternThick,(SetVerticalLineFuncType)&AccelerantDriver::VLinePatternThick);
	Unlock();
}

void AccelerantDriver::StrokeRoundRect(BRect r, float xrad, float yrad, float pensize, RGBColor& color)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	fDrawPattern.SetColors(color,color);
	DisplayDriver::StrokeRoundRect(r,xrad,yrad,this,(SetHorizontalLineFuncType)&AccelerantDriver::HLinePatternThick,(SetVerticalLineFuncType)&AccelerantDriver::VLinePatternThick,(SetPixelFuncType)&AccelerantDriver::SetThickPatternPixel);
	Unlock();
}

void AccelerantDriver::StrokeRoundRect(BRect r, float xrad, float yrad, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color)
{
	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget(pattern);
	fDrawPattern.SetColors(high_color,low_color);
	DisplayDriver::StrokeRoundRect(r,xrad,yrad,this,(SetHorizontalLineFuncType)&AccelerantDriver::HLinePatternThick,(SetVerticalLineFuncType)&AccelerantDriver::VLinePatternThick,(SetPixelFuncType)&AccelerantDriver::SetThickPatternPixel);
	Unlock();
}

/*!
	\brief Draws a series of lines - optimized for speed
	\param pts Array of BPoints pairs
	\param numlines Number of lines to be drawn
	\param pensize The thickness of the lines
	\param colors Array of colors for each respective line
*/
void AccelerantDriver::StrokeLineArray(BPoint *pts, int32 numlines, float pensize, RGBColor *colors)
{
	int x1, y1, x2, y2, dx, dy;
	int steps, k;
	double xInc, yInc;
	double x,y;
	int i;

	Lock();
	fLineThickness = (int)pensize;
	fDrawPattern.SetTarget((int8*)&B_SOLID_HIGH);
	for (i=0; i<numlines; i++)
	{
		//fDrawColor = colors[i];
		fDrawPattern.SetColors(colors[i],colors[i]);
		x1 = ROUND(pts[i*2].x);
		y1 = ROUND(pts[i*2].y);
		x2 = ROUND(pts[i*2+1].x);
		y2 = ROUND(pts[i*2+1].y);
		dx = x2-x1;
		dy = y2-y1;
		x = x1;
		y = y1;
		if ( abs(dx) > abs(dy) )
			steps = abs(dx);
		else
			steps = abs(dy);
		xInc = dx / (double) steps;
		yInc = dy / (double) steps;

		SetThickPatternPixel(ROUND(x),ROUND(y));
		for (k=0; k<steps; k++)
		{
			x += xInc;
			y += yInc;
			SetThickPatternPixel(ROUND(x),ROUND(y));
		}
	}
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
/*
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
*/

/*!
	\brief Draws a point using the display driver's specified thickness and pattern
	\param x The x coordinate (not guaranteed to be in bounds)
	\param y The y coordinate (not guaranteed to be in bounds)
*/
void AccelerantDriver::SetThickPatternPixel(int x, int y)
{
	int left, right, top, bottom;
	left = x - fLineThickness/2;
	right = x + fLineThickness/2;
	top = y - fLineThickness/2;
	bottom = y + fLineThickness/2;
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
						fb[x] = fDrawPattern.GetColor(x,y).GetColor8();;
					fb += mFrameBufferConfig.bytes_per_row;
				}
			} break;
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
						fb[x] = fDrawPattern.GetColor(x,y).GetColor15();
					fb = (uint16 *)((uint8 *)fb + mFrameBufferConfig.bytes_per_row);
				}
			} break;
		case B_RGB16_BIG:
		case B_RGB16_LITTLE:
			{
				int x,y;
				uint16 *fb = (uint16 *)((uint8 *)mFrameBufferConfig.frame_buffer + top*mFrameBufferConfig.bytes_per_row);
				for (y=top; y<=bottom; y++)
				{
					for (x=left; x<=right; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor16();
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
						color = fDrawPattern.GetColor(x,y).GetColor32();
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
/*
void AccelerantDriver::HLine(int32 x1, int32 x2, int32 y, PatternHandler *pat)
{
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
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_RGB15_LITTLE:
		case B_RGBA15_LITTLE:
			{
				uint16 *fb = (uint16 *)((uint8 *)mFrameBufferConfig.frame_buffer + y*mFrameBufferConfig.bytes_per_row);
				for (x=x1; x<=x2; x++)
					fb[x] = pat->GetColor(x,y).GetColor15();
			} break;
		case B_RGB16_BIG:
		case B_RGB16_LITTLE:
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
*/

/*!
	\brief Draws a horizontal line using the display driver's line thickness and pattern
	\param x1 The first x coordinate (not guaranteed to be in bounds)
	\param x2 The second x coordinate (not guaranteed to be in bounds)
	\param y The y coordinate (not guaranteed to be in bounds)
*/
void AccelerantDriver::HLinePatternThick(int32 x1, int32 x2, int32 y)
{
	int x, y1, y2;

	if ( x1 > x2 )
	{
		x = x2;
		x2 = x1;
		x1 = x;
	}
	y1 = y - fLineThickness/2;
	y2 = y + fLineThickness/2;
	switch (mDisplayMode.space)
	{
		case B_CMAP8:
		case B_GRAY8:
			{
				uint8 *fb = (uint8 *)mFrameBufferConfig.frame_buffer + y1*mFrameBufferConfig.bytes_per_row;
				for (y=y1; y<=y2; y++)
				{
					for (x=x1; x<=x2; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor8();
					fb += mFrameBufferConfig.bytes_per_row;
				}
			} break;
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_RGB15_LITTLE:
		case B_RGBA15_LITTLE:
			{
				uint16 *fb = (uint16 *)((uint8 *)mFrameBufferConfig.frame_buffer + y1*mFrameBufferConfig.bytes_per_row);
				for (y=y1; y<=y2; y++)
				{
					for (x=x1; x<=x2; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor15();
					fb = (uint16 *)((uint8 *)fb + mFrameBufferConfig.bytes_per_row);
				}
			} break;
		case B_RGB16_BIG:
		case B_RGB16_LITTLE:
			{
				uint16 *fb = (uint16 *)((uint8 *)mFrameBufferConfig.frame_buffer + y1*mFrameBufferConfig.bytes_per_row);
				for (y=y1; y<=y2; y++)
				{
					for (x=x1; x<=x2; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor16();
					fb = (uint16 *)((uint8 *)fb + mFrameBufferConfig.bytes_per_row);
				}
			} break;
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGB32_LITTLE:
		case B_RGBA32_LITTLE:
			{
				uint32 *fb = (uint32 *)((uint8 *)mFrameBufferConfig.frame_buffer + y1*mFrameBufferConfig.bytes_per_row);
				rgb_color color;
				for (y=y1; y<=y2; y++)
				{
					for (x=x1; x<=x2; x++)
					{
						color = fDrawPattern.GetColor(x,y).GetColor32();
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
	\brief Draws a vertical line using the display driver's line thickness and pattern
	\param x The x coordinate
	\param y1 The first y coordinate
	\param y2 The second y coordinate
*/
void AccelerantDriver::VLinePatternThick(int32 x, int32 y1, int32 y2)
{
	int y, x1, x2;

	if ( y1 > y2 )
	{
		y = y2;
		y2 = y1;
		y1 = y;
	}
	x1 = x - fLineThickness/2;
	x2 = x + fLineThickness/2;
	switch (mDisplayMode.space)
	{
		case B_CMAP8:
		case B_GRAY8:
			{
				uint8 *fb = (uint8 *)mFrameBufferConfig.frame_buffer + y1*mFrameBufferConfig.bytes_per_row;
				for (y=y1; y<=y2; y++)
				{
					for (x=x1; x<=x2; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor8();
					fb += mFrameBufferConfig.bytes_per_row;
				}
			} break;
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_RGB15_LITTLE:
		case B_RGBA15_LITTLE:
			{
				uint16 *fb = (uint16 *)((uint8 *)mFrameBufferConfig.frame_buffer + y1*mFrameBufferConfig.bytes_per_row);
				for (y=y1; y<=y2; y++)
				{
					for (x=x1; x<=x2; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor15();
					fb = (uint16 *)((uint8 *)fb + mFrameBufferConfig.bytes_per_row);
				}
			} break;
		case B_RGB16_BIG:
		case B_RGB16_LITTLE:
			{
				uint16 *fb = (uint16 *)((uint8 *)mFrameBufferConfig.frame_buffer + y1*mFrameBufferConfig.bytes_per_row);
				for (y=y1; y<=y2; y++)
				{
					for (x=x1; x<=x2; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor16();
					fb = (uint16 *)((uint8 *)fb + mFrameBufferConfig.bytes_per_row);
				}
			} break;
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGB32_LITTLE:
		case B_RGBA32_LITTLE:
			{
				uint32 *fb = (uint32 *)((uint8 *)mFrameBufferConfig.frame_buffer + y1*mFrameBufferConfig.bytes_per_row);
				rgb_color color;
				for (y=y1; y<=y2; y++)
				{
					for (x=x1; x<=x2; x++)
					{
						color = fDrawPattern.GetColor(x,y).GetColor32();
						fb[x] = (color.alpha << 24) | (color.red << 16) | (color.green << 8) | (color.blue);
					}
					fb = (uint32 *)((uint8 *)fb + mFrameBufferConfig.bytes_per_row);
				}
			} break;
		default:
			printf("Error: Unknown color space\n");
	}
}

void AccelerantDriver::FillSolidRect(int32 left, int32 top, int32 right, int32 bottom)
{
#ifndef DISABLE_HARDWARE_ACCELERATION
	if ( accFillRect && AcquireEngine )
	{
		if ( AcquireEngine(0,0,NULL,&mEngineToken) == B_OK )
		{
			fill_rect_params fillParams;
			uint32 fill_color=0;
			fillParams.right = (uint16)right;
			fillParams.left = (uint16)left;
			fillParams.top = (uint16)top;
			fillParams.bottom = (uint16)bottom;
			switch (mDisplayMode.space)
			{
				case B_CMAP8:
				case B_GRAY8:
					fill_color = fDrawColor.GetColor8();
					break;
				case B_RGB15_BIG:
				case B_RGBA15_BIG:
				case B_RGB15_LITTLE:
				case B_RGBA15_LITTLE:
					fill_color = fDrawColor.GetColor15();
					break;
				case B_RGB16_BIG:
				case B_RGB16_LITTLE:
					fill_color = fDrawColor.GetColor16();
					break;
				case B_RGB32_BIG:
				case B_RGBA32_BIG:
				case B_RGB32_LITTLE:
				case B_RGBA32_LITTLE:
				{
					rgb_color rgbcolor = fDrawColor.GetColor32();
					fill_color = (rgbcolor.alpha << 24) | (rgbcolor.red << 16) | (rgbcolor.green << 8) | (rgbcolor.blue);
				}
					break;
			}
			accFillRect(mEngineToken, fill_color, &fillParams, 1);
			if ( ReleaseEngine )
				ReleaseEngine(mEngineToken,NULL);
			Unlock();
			return;
		}
	}
#endif

	switch (mDisplayMode.space)
	{
		case B_CMAP8:
		case B_GRAY8:
			{
				uint8 *fb = (uint8 *)mFrameBufferConfig.frame_buffer + top*mFrameBufferConfig.bytes_per_row;
				uint8 color8 = fDrawColor.GetColor8();
				int x,y;
				for (y=top; y<=bottom; y++)
				{
					for (x=left; x<=right; x++)
						fb[x] = color8;
					fb += mFrameBufferConfig.bytes_per_row;
				}
			} break;
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_RGB15_LITTLE:
		case B_RGBA15_LITTLE:
			{
				uint16 *fb = (uint16 *)((uint8 *)mFrameBufferConfig.frame_buffer + top*mFrameBufferConfig.bytes_per_row);
				uint16 color15 = fDrawColor.GetColor15();
				int x,y;
				for (y=top; y<=bottom; y++)
				{
					for (x=left; x<=right; x++)
						fb[x] = color15;
					fb = (uint16 *)((uint8 *)fb + mFrameBufferConfig.bytes_per_row);
				}
			} break;
		case B_RGB16_BIG:
		case B_RGB16_LITTLE:
			{
				uint16 *fb = (uint16 *)((uint8 *)mFrameBufferConfig.frame_buffer + top*mFrameBufferConfig.bytes_per_row);
				uint16 color16 = fDrawColor.GetColor16();
				int x,y;
				for (y=top; y<=bottom; y++)
				{
					for (x=left; x<=right; x++)
						fb[x] = color16;
					fb = (uint16 *)((uint8 *)fb + mFrameBufferConfig.bytes_per_row);
				}
			} break;
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGB32_LITTLE:
		case B_RGBA32_LITTLE:
			{
				uint32 *fb = (uint32 *)((uint8 *)mFrameBufferConfig.frame_buffer + top*mFrameBufferConfig.bytes_per_row);
				rgb_color fill_color = fDrawColor.GetColor32();
				uint32 color32 = (fill_color.alpha << 24) | (fill_color.red << 16) | (fill_color.green << 8) | (fill_color.blue);
				int x,y;
				for (y=top; y<=bottom; y++)
				{
					for (x=left; x<=right; x++)
						fb[x] = color32;
					fb = (uint32 *)((uint8 *)fb + mFrameBufferConfig.bytes_per_row);
				}
			} break;
		default:
			printf("Error: Unknown color space\n");
	}
}

void AccelerantDriver::FillPatternRect(int32 left, int32 top, int32 right, int32 bottom)
{
	switch (mDisplayMode.space)
	{
		case B_CMAP8:
		case B_GRAY8:
			{
				uint8 *fb = (uint8 *)mFrameBufferConfig.frame_buffer + top*mFrameBufferConfig.bytes_per_row;
				int x,y;
				for (y=top; y<=bottom; y++)
				{
					for (x=left; x<=right; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor8();
					fb += mFrameBufferConfig.bytes_per_row;
				}
			} break;
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_RGB15_LITTLE:
		case B_RGBA15_LITTLE:
			{
				uint16 *fb = (uint16 *)((uint8 *)mFrameBufferConfig.frame_buffer + top*mFrameBufferConfig.bytes_per_row);
				int x,y;
				for (y=top; y<=bottom; y++)
				{
					for (x=left; x<=right; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor15();
					fb = (uint16 *)((uint8 *)fb + mFrameBufferConfig.bytes_per_row);
				}
			} break;
		case B_RGB16_BIG:
		case B_RGB16_LITTLE:
			{
				uint16 *fb = (uint16 *)((uint8 *)mFrameBufferConfig.frame_buffer + top*mFrameBufferConfig.bytes_per_row);
				int x,y;
				for (y=top; y<=bottom; y++)
				{
					for (x=left; x<=right; x++)
						fb[x] = fDrawPattern.GetColor(x,y).GetColor16();
					fb = (uint16 *)((uint8 *)fb + mFrameBufferConfig.bytes_per_row);
				}
			} break;
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGB32_LITTLE:
		case B_RGBA32_LITTLE:
			{
				uint32 *fb = (uint32 *)((uint8 *)mFrameBufferConfig.frame_buffer + top*mFrameBufferConfig.bytes_per_row);
				int x,y;
				rgb_color color;
				for (y=top; y<=bottom; y++)
				{
					for (x=left; x<=right; x++)
					{
						color = fDrawPattern.GetColor(x,y).GetColor32();
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
	if ( count < deviceNumber )
	{
		if ( deviceNumber == 1 )
		{
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
		case B_RGB15:
		case B_RGBA15:
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
			depth = 15;
			break;
		case B_RGB16:
		case B_RGB16_BIG:
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
