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
/*	int i;
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
	// Commented out to remove a couple warnings
//	RGBColor red(255,0,0,0);
//	RGBColor green(0,255,0,0);
	RGBColor blue(0,0,255,0);
	FillRect(BRect(0,0,1024,768),blue);
#endif

*/
	return true;
}

/*!
	\brief Shuts down the driver's video subsystem
	
	Any work done by Initialize() should be undone here. Note that Shutdown() is
	called even if Initialize() was unsuccessful.
*/
void AccelerantDriver::Shutdown(void)
{
/*
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
*/
}

/*!
	\brief Draws a series of lines - optimized for speed
	\param pts Array of BPoints pairs
	\param numlines Number of lines to be drawn
	\param pensize The thickness of the lines
	\param colors Array of colors for each respective line
*/
void AccelerantDriver::StrokeLineArray(BPoint *pts, const int32 &numlines, const DrawData *d, RGBColor *colors)
{
	if(!d)
		return;
	
	int x1, y1, x2, y2, dx, dy;
	int steps, k;
	double xInc, yInc;
	double x,y;
	int i;

	Lock();
	fLineThickness = (int)d->pensize;
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
	\brief Inverts the colors in the rectangle.
	\param r Rectangle of the area to be inverted. Guaranteed to be within bounds.
*/
void AccelerantDriver::InvertRect(const BRect &r)
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
	\brief Sets the screen mode to specified resolution and color depth.
	\param mode constant as defined in GraphicsDefs.h
	
	Subclasses must include calls to _SetDepth, _SetHeight, _SetWidth, and _SetMode
	to update the state variables kept internally by the DisplayDriver class.
*/
void AccelerantDriver::SetMode(const int32 &mode)
{
  /* TODO: Still needs some work to fine tune color hassles in picking the mode */
/*	set_display_mode SetDisplayMode = (set_display_mode)accelerant_hook(B_SET_DISPLAY_MODE, NULL);
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
*/
}

void AccelerantDriver::SetMode(const display_mode &mode)
{
	// TODO: Implement
	
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
