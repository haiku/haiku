//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
//	File Name:		DefaultDecorator.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Fallback decorator for the app_server
//  
//------------------------------------------------------------------------------
#include <Rect.h>
#include "DisplayDriver.h"
#include <View.h>
#include "LayerData.h"
#include "ColorUtils.h"
#include "DefaultDecorator.h"
#include "PatternHandler.h"
#include "RGBColor.h"
#include "RectUtils.h"
#include <stdio.h>


//#define USE_VIEW_FILL_HACK

//#define DEBUG_DECORATOR
#ifdef DEBUG_DECORATOR
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

DefaultDecorator::DefaultDecorator(BRect rect, int32 wlook, int32 wfeel, int32 wflags)
 : Decorator(rect,wlook,wfeel,wflags)
{

	taboffset=0;
	titlepixelwidth=0;

	framecolors=new RGBColor[6];
	framecolors[0].SetColor(152,152,152);
	framecolors[1].SetColor(255,255,255);
	framecolors[2].SetColor(216,216,216);
	framecolors[3].SetColor(136,136,136);
	framecolors[4].SetColor(152,152,152);
	framecolors[5].SetColor(96,96,96);

	// Set appropriate colors based on the current focus value. In this case, each decorator
	// defaults to not having the focus.
	_SetFocus();
	
	// Do initial decorator setup
	_DoLayout();
	
//	tab_highcol=_colors->window_tab;
//	tab_lowcol=_colors->window_tab;

STRACE(("DefaultDecorator:\n"));
STRACE(("\tFrame (%.1f,%.1f,%.1f,%.1f)\n",rect.left,rect.top,rect.right,rect.bottom));
}

DefaultDecorator::~DefaultDecorator(void)
{
STRACE(("DefaultDecorator: ~DefaultDecorator()\n"));
	delete [] framecolors;
}

click_type DefaultDecorator::Clicked(BPoint pt, int32 buttons, int32 modifiers)
{
	#ifdef DEBUG_DECORATOR
	printf("DefaultDecorator: Clicked\n");
	printf("\tPoint: (%.1f,%.1f)\n",pt.x,pt.y);
	printf("\tButtons:\n");
	if(buttons==0)
		printf("\t\tNone\n");
	else
	{
		if(buttons & B_PRIMARY_MOUSE_BUTTON)
			printf("\t\tPrimary\n");
		if(buttons & B_SECONDARY_MOUSE_BUTTON)
			printf("\t\tSecondary\n");
		if(buttons & B_TERTIARY_MOUSE_BUTTON)
			printf("\t\tTertiary\n");
	}
	printf("\tModifiers:\n");
	if(modifiers==0)
		printf("\t\tNone\n");
	else
	{
		if(modifiers & B_CAPS_LOCK)
			printf("\t\tCaps Lock\n");
		if(modifiers & B_NUM_LOCK)
			printf("\t\tNum Lock\n");
		if(modifiers & B_SCROLL_LOCK)
			printf("\t\tScroll Lock\n");
		if(modifiers & B_LEFT_COMMAND_KEY)
			printf("\t\t Left Command\n");
		if(modifiers & B_RIGHT_COMMAND_KEY)
			printf("\t\t Right Command\n");
		if(modifiers & B_LEFT_CONTROL_KEY)
			printf("\t\tLeft Control\n");
		if(modifiers & B_RIGHT_CONTROL_KEY)
			printf("\t\tRight Control\n");
		if(modifiers & B_LEFT_OPTION_KEY)
			printf("\t\tLeft Option\n");
		if(modifiers & B_RIGHT_OPTION_KEY)
			printf("\t\tRight Option\n");
		if(modifiers & B_LEFT_SHIFT_KEY)
			printf("\t\tLeft Shift\n");
		if(modifiers & B_RIGHT_SHIFT_KEY)
			printf("\t\tRight Shift\n");
		if(modifiers & B_MENU_KEY)
			printf("\t\tMenu\n");
	}
	#endif
	
	// In checking for hit test stuff, we start with the smallest rectangles the user might
	// be clicking on and gradually work our way out into larger rectangles.
	if(_closerect.Contains(pt))
		return DEC_CLOSE;

	if(_zoomrect.Contains(pt))
		return DEC_ZOOM;
	
	if(_resizerect.Contains(pt) && _look==B_DOCUMENT_WINDOW_LOOK)
		return DEC_RESIZE;

	// Clicking in the tab?
	if(_tabrect.Contains(pt))
	{
		// Here's part of our window management stuff
		if(buttons==B_SECONDARY_MOUSE_BUTTON)
			return DEC_MOVETOBACK;
		return DEC_DRAG;
	}

	// We got this far, so user is clicking on the border?
	if (leftborder.Contains(pt) || rightborder.Contains(pt)
		|| topborder.Contains(pt) || bottomborder.Contains(pt))
	{
		if (_look == B_TITLED_WINDOW_LOOK || _look == B_FLOATING_WINDOW_LOOK){
			BRect		temp(BPoint(bottomborder.right-18, bottomborder.bottom-18), bottomborder.RightBottom());
			if (temp.Contains(pt))
				return DEC_RESIZE;
		}
		return DEC_DRAG;
	}

	// Guess user didn't click anything
	return DEC_NONE;
}

void DefaultDecorator::_DoLayout(void)
{
STRACE(("DefaultDecorator: Do Layout\n"));
	// Here we determine the size of every rectangle that we use
	// internally when we are given the size of the client rectangle.

	switch(GetLook())
	{
		case B_FLOATING_WINDOW_LOOK:
		case B_MODAL_WINDOW_LOOK:
		case B_BORDERED_WINDOW_LOOK:
		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
			borderwidth = 5;
			break;
		default:
			borderwidth = 0;
	}
	
	// distance from one item of the tab bar to another. In this case the text and close/zoom rects
	textoffset = (_look==B_FLOATING_WINDOW_LOOK) ? 7 : 10;
	
	// calculate our tab rect
	_tabrect.Set( 	_frame.left - borderwidth,
					_frame.top - borderwidth - 19.0,
					((_frame.right - _frame.left) < 35.0 ?
							_frame.left + 35.0 : _frame.right) + borderwidth,
					_frame.top - borderwidth );

	// make it text width sensitive
	if(strlen(GetTitle())>1)
	{
		if(_driver)
			titlepixelwidth=_driver->StringWidth(GetTitle(), _TitleWidth(), &_drawdata);
		else
			titlepixelwidth=10;
		
		int32	tabLength	= 	int32(14 + // _closerect width
								textoffset + titlepixelwidth + textoffset +
								14 + // _zoomrect width
								8); // margins
		int32	tabWidth	= (int32)_tabrect.Width();
		if ( tabLength < tabWidth )
			_tabrect.right	= _tabrect.left + tabLength;
	}
	else
		_tabrect.right		= _tabrect.left + _tabrect.Width()/2;

	// calculate left/top/right/bottom borders
	if ( borderwidth != 0 ){
		leftborder.Set( _frame.left - borderwidth, _frame.top,
						_frame.left - 1, _frame.bottom);
		rightborder.Set(_frame.right + 1, _frame.top ,
						_frame.right + borderwidth, _frame.bottom);
		topborder.Set( 	_frame.left - borderwidth, _frame.top - borderwidth,
						_frame.right + borderwidth, _frame.top - 1);
		bottomborder.Set(	_frame.left - borderwidth, _frame.bottom + 1,
							_frame.right + borderwidth, _frame.bottom + borderwidth);
	}
	else{
		// no border ... (?) useful when displaying windows that are just images
		leftborder.Set( 0.0, 0.0, -1.0, -1.0 );
		rightborder.Set( 0.0, 0.0, -1.0, -1.0 );
		topborder.Set( 0.0, 0.0, -1.0, -1.0 );
		bottomborder.Set( 0.0, 0.0, -1.0, -1.0 );						
	}

	// calculate resize rect
	_resizerect.Set(	bottomborder.right - 18.0, bottomborder.bottom - 18.0,
						bottomborder.right - 3, bottomborder.bottom - 3);

	// format tab rect for a floating window - make the rect smaller
	if ( _look == B_FLOATING_WINDOW_LOOK ){
		_tabrect.InsetBy( 0, 2 );
		_tabrect.OffsetBy( 0, 2 );
	}	

	// calulate close rect based on the tab rectangle
	_closerect.Set(	_tabrect.left + 4.0, _tabrect.top + 4.0,
					_tabrect.left + 4.0 + 13.0, _tabrect.top + 4.0 + 13.0 );
					
	// calulate zoom rect based on the tab rectangle
	_zoomrect.Set(  _tabrect.right - 4.0 - 13.0, _tabrect.top + 4.0,
					_tabrect.right - 4.0, _tabrect.top + 4.0 + 13.0 );

	// format close and zoom rects for a floating window - make rectangles smaller
	if ( _look == B_FLOATING_WINDOW_LOOK ){
		_closerect.InsetBy( 1, 1 );
		_zoomrect.InsetBy( 1, 1 );
		_closerect.OffsetBy( 0, -2 );
		_zoomrect.OffsetBy( 0, -2 );
	}
}

void DefaultDecorator::MoveBy(float x, float y)
{
	MoveBy(BPoint(x,y));
}

void DefaultDecorator::MoveBy(BPoint pt)
{
	STRACE(("DefaultDecorator: Move By (%.1f, %.1f)\n",pt.x,pt.y));
	// Move all internal rectangles the appropriate amount
	_frame.OffsetBy(pt);
	_closerect.OffsetBy(pt);
	_tabrect.OffsetBy(pt);
	_resizerect.OffsetBy(pt);
	_zoomrect.OffsetBy(pt);
	_borderrect.OffsetBy(pt);
	
	leftborder.OffsetBy(pt);
	rightborder.OffsetBy(pt);
	topborder.OffsetBy(pt);
	bottomborder.OffsetBy(pt);
}

void DefaultDecorator::ResizeBy(float x, float y)
{
	ResizeBy(BPoint(x,y));
}

void DefaultDecorator::ResizeBy(BPoint pt)
{
	STRACE(("DefaultDecorator: Resize By (%.1f, %.1f)\n",pt.x,pt.y));
	// Move all internal rectangles the appropriate amount
	_frame.right	+= pt.x;
	_frame.bottom	+= pt.y;

// TODO: make bigger/smaller
//	_tabrect.

	_resizerect.OffsetBy(pt);
//	_zoomrect.OffsetBy(pt);
	_borderrect.right	+= pt.x;
	_borderrect.bottom	+= pt.y;
	
	leftborder.bottom	+= pt.y;
	topborder.right		+= pt.x;
	rightborder.OffsetBy(pt.x, 0.0f);
	rightborder.bottom	+= pt.y;
	
	bottomborder.OffsetBy(0.0, pt.y);
	bottomborder.right	+= pt.x;
}

void DefaultDecorator::GetFootprint(BRegion *region)
{
	STRACE(("DefaultDecorator: Get Footprint\n"));
	// This function calculates the decorator's footprint in coordinates
	// relative to the layer. This is most often used to set a WinBorder
	// object's visible region.
	if(!region)
		return;

	region->MakeEmpty();

	if(_look == B_NO_BORDER_WINDOW_LOOK)
	{
		return;
	}

	region->Include(leftborder);
	region->Include(rightborder);
	region->Include(topborder);
	region->Include(bottomborder);
	
	if(_look == B_BORDERED_WINDOW_LOOK)
		return;

	region->Include(_tabrect);

	if(_look == B_DOCUMENT_WINDOW_LOOK)
		region->Include(BRect(_frame.right - 13.0f, _frame.bottom - 13.0f,
								_frame.right, _frame.bottom));
}

BRect DefaultDecorator::SlideTab(float dx, float dy)
{
	//return Decorator::SlideTab(dx,dy);
	return _tabrect;
}

void DefaultDecorator::_DrawTitle(BRect r)
{
	STRACE(("_DrawTitle(%f,%f,%f,%f)\n", r.left, r.top, r.right, r.bottom));
	// Designed simply to redraw the title when it has changed on
	// the client side.
	_drawdata.highcolor=_colors->window_tab_text;
	_drawdata.lowcolor=(GetFocus())?_colors->window_tab:_colors->inactive_window_tab;

	int32 titlecount=_ClipTitle((_zoomrect.left-textoffset)-(_closerect.right+textoffset));
	BString titlestr( GetTitle() );
	
	if(titlecount<titlestr.CountChars())
	{
		titlestr.Truncate(titlecount-1);
		titlestr+="...";
		titlecount+=2;
	}
	
	// The text position needs tweaked when working as a floating window because the closerect placement
	// is a little different. If it isn't moved, title placement looks really funky
	if(_look==B_FLOATING_WINDOW_LOOK)
	_driver->DrawString(titlestr.String(),titlecount,
		BPoint(_closerect.right+textoffset,_closerect.bottom+1),&_drawdata);
	else	
	_driver->DrawString(titlestr.String(),titlecount,
		BPoint(_closerect.right+textoffset,_closerect.bottom),&_drawdata);
}

void DefaultDecorator::_SetFocus(void)
{
	// SetFocus() performs necessary duties for color swapping and
	// other things when a window is deactivated or activated.
	
	if(GetFocus())
	{
		button_highcol.SetColor(tint_color(_colors->window_tab.GetColor32(),B_LIGHTEN_2_TINT));
		button_lowcol.SetColor(tint_color(_colors->window_tab.GetColor32(),B_DARKEN_1_TINT));
		textcol=_colors->window_tab_text;
	}
	else
	{
		button_highcol.SetColor(tint_color(_colors->inactive_window_tab.GetColor32(),B_LIGHTEN_2_TINT));
		button_lowcol.SetColor(tint_color(_colors->inactive_window_tab.GetColor32(),B_DARKEN_1_TINT));
		textcol=_colors->inactive_window_tab_text;
	}
}

void DefaultDecorator::Draw(BRect update)
{
	STRACE(("DefaultDecorator: Draw(%.1f,%.1f,%.1f,%.1f)\n",update.left,update.top,update.right,update.bottom));

	// We need to draw a few things: the tab, the resize thumb, the borders,
	// and the buttons

	_DrawFrame(update);
	_DrawTab(update);
}

void DefaultDecorator::Draw(void)
{
	// Easy way to draw everything - no worries about drawing only certain
	// things

	_DrawFrame(BRect(topborder.LeftTop(), bottomborder.RightBottom()));
	_DrawTab(_tabrect);
}

void DefaultDecorator::_DrawZoom(BRect r)
{
	STRACE(("_DrawZoom(%f,%f,%f,%f)\n", r.left, r.top, r.right, r.bottom));
	// If this has been implemented, then the decorator has a Zoom button
	// which should be drawn based on the state of the member zoomstate
	BRect zr( r );
	
	zr.left		+= 3.0;
	zr.top		+= 3.0;
	DrawBlendedRect( zr, GetZoom() );
	
	zr			= r;
	zr.right	-= 5.0;
	zr.bottom	-= 5.0;
	DrawBlendedRect( zr, GetZoom() );
}

void DefaultDecorator::_DrawClose(BRect r)
{
	STRACE(("_DrawClose(%f,%f,%f,%f)\n", r.left, r.top, r.right, r.bottom));
	// Just like DrawZoom, but for a close button
	DrawBlendedRect( r, GetClose());
}

void DefaultDecorator::_DrawTab(BRect r)
{
	STRACE(("_DrawTab(%f,%f,%f,%f)\n", r.left, r.top, r.right, r.bottom));
	// If a window has a tab, this will draw it and any buttons which are
	// in it.
	if(_look == B_NO_BORDER_WINDOW_LOOK || _look == B_BORDERED_WINDOW_LOOK)
		return;

	RGBColor		tabColor = (GetFocus())?_colors->window_tab:_colors->inactive_window_tab;	
	_driver->FillRect(_tabrect, tabColor);
	
	_driver->StrokeLine(_tabrect.LeftTop(),_tabrect.LeftBottom(),framecolors[0]);
	_driver->StrokeLine(_tabrect.LeftTop(),_tabrect.RightTop(),framecolors[0]);
	_driver->StrokeLine(_tabrect.RightTop(),_tabrect.RightBottom(),framecolors[5]);
	_driver->StrokeLine( BPoint( _tabrect.left + 2, _tabrect.bottom+1 ),
						 BPoint( _tabrect.right - 2, _tabrect.bottom+1 ),
						 framecolors[2]);


	_driver->StrokeLine( BPoint( _tabrect.left + 1, _tabrect.top + 1 ),
						 BPoint( _tabrect.left + 1, _tabrect.bottom ),
						 framecolors[1]);
	_driver->StrokeLine( BPoint( _tabrect.left + 1, _tabrect.top + 1 ),
						 BPoint( _tabrect.right - 1, _tabrect.top + 1 ),
						 framecolors[1]);

	_driver->StrokeLine( BPoint( _tabrect.right - 1, _tabrect.top + 2 ),
						 BPoint( _tabrect.right - 1, _tabrect.bottom ),
						 framecolors[3]);

	_DrawTitle(_tabrect);

	// Draw the buttons if we're supposed to	
	if(!(_flags & B_NOT_CLOSABLE))
		_DrawClose(_closerect);
	if(!(_flags & B_NOT_ZOOMABLE) && _look != B_MODAL_WINDOW_LOOK)
		_DrawZoom(_zoomrect);
}

void DefaultDecorator::_SetColors(void)
{
	_SetFocus();
}

void DefaultDecorator::DrawBlendedRect(BRect r, bool down)
{
	// This bad boy is used to draw a rectangle with a gradient.
	// Note that it is not part of the Decorator API - it's specific
	// to just the BeDecorator. Called by DrawZoom and DrawClose

	// Actually just draws a blended square
	int32 w=r.IntegerWidth(),  h=r.IntegerHeight();
	
	RGBColor temprgbcol;
	rgb_color halfcol, startcol, endcol;
	float rstep,gstep,bstep,i;

	int steps=(w<h)?w:h;

	if(down)
	{
		startcol=button_lowcol.GetColor32();
		endcol=button_highcol.GetColor32();
	}
	else
	{
		startcol=button_highcol.GetColor32();
		endcol=button_lowcol.GetColor32();
	}

	halfcol=MakeBlendColor(startcol,endcol,0.5);

	rstep=float(startcol.red-halfcol.red)/steps;
	gstep=float(startcol.green-halfcol.green)/steps;
	bstep=float(startcol.blue-halfcol.blue)/steps;

	for(i=0;i<=steps; i++)
	{
		temprgbcol.SetColor(uint8(startcol.red-(i*rstep)),
			uint8(startcol.green-(i*gstep)),
			uint8(startcol.blue-(i*bstep)));
		
		_driver->StrokeLine(BPoint(r.left,r.top+i),
			BPoint(r.left+i,r.top),temprgbcol);

		temprgbcol.SetColor(uint8(halfcol.red-(i*rstep)),
			uint8(halfcol.green-(i*gstep)),
			uint8(halfcol.blue-(i*bstep)));

		_driver->StrokeLine(BPoint(r.left+steps,r.top+i),
			BPoint(r.left+i,r.top+steps),temprgbcol);
	}

//	_layerdata.highcolor=startcol;
//	_driver->FillRect(r,&_layerdata,pat_solidhigh);
	_driver->StrokeRect(r,framecolors[3]);
}

void DefaultDecorator::_DrawFrame(BRect invalid)
{
STRACE(("_DrawFrame(%f,%f,%f,%f)\n", invalid.left, invalid.top,
									 invalid.right, invalid.bottom));

	#ifdef USE_VIEW_FILL_HACK
		_drawdata.highcolor = RGBColor( 192, 192, 192 );	
		_driver->FillRect(_frame,_drawdata.highcolor);
	#endif

	if(_look == B_NO_BORDER_WINDOW_LOOK)
		return;

	if(!borderwidth)
		return;

	{ // a block start

	BRect		r = BRect(topborder.LeftTop(), bottomborder.RightBottom());
	//top
	for (int8 i=0; i<5; i++){
		_driver->StrokeLine(BPoint(r.left+i, r.top+i), BPoint(r.right-i, r.top+i), framecolors[i]);
	}
	//left
	for (int8 i=0; i<5; i++){
		_driver->StrokeLine(BPoint(r.left+i, r.top+i), BPoint(r.left+i, r.bottom-i), framecolors[i]);
	}
	//bottom
	for (int8 i=0; i<5; i++){
		_driver->StrokeLine(BPoint(r.left+i, r.bottom-i), BPoint(r.right-i, r.bottom-i), framecolors[(4-i)==4? 5: (4-i)]);
	}
	//right
	for (int8 i=0; i<5; i++){
		_driver->StrokeLine(BPoint(r.right-i, r.top+i), BPoint(r.right-i, r.bottom-i), framecolors[(4-i)==4? 5: (4-i)]);
	}

	} // end of the block/

	// Draw the resize thumb if we're supposed to
	if(!(_flags & B_NOT_RESIZABLE))
	{
		BRect		r = _resizerect;

		switch(_look){
		// This code is strictly for B_DOCUMENT_WINDOW looks
			case B_DOCUMENT_WINDOW_LOOK:{

				// Explicitly locking the driver is normally unnecessary. However, we need to do
				// this because we are rapidly drawing a series of calls which would not necessarily
				// draw correctly if we didn't do so.
				float	x = r.right;
				float	y = r.bottom;
				_driver->Lock();
				_driver->FillRect(BRect(x-13, y-13, x, y), framecolors[2]);
				_driver->StrokeLine(BPoint(x-15, y-15), BPoint(x-15, y-2), framecolors[0]);
				_driver->StrokeLine(BPoint(x-14, y-14), BPoint(x-14, y-1), framecolors[1]);
				_driver->StrokeLine(BPoint(x-15, y-15), BPoint(x-2, y-15), framecolors[0]);
				_driver->StrokeLine(BPoint(x-14, y-14), BPoint(x-1, y-14), framecolors[1]);

				for(int8 i=1; i <= 4; i++){
					for(int8 j=1; j<=i; j++){
						BPoint		pt1(x-(3*j)+1, y-(3*(5-i))+1);
						BPoint		pt2(x-(3*j)+2, y-(3*(5-i))+2);
						_driver->StrokePoint(pt1, framecolors[0]);
						_driver->StrokePoint(pt2, framecolors[1]);
					}
				}
//				RGBColor		c(255,128,0);
//				_driver->FillRect(BRect(50,50,600,400), c);
				_driver->Unlock();
				break;
			}

			case B_TITLED_WINDOW_LOOK:
			case B_FLOATING_WINDOW_LOOK:{
				_driver->StrokeLine(BPoint(rightborder.left, rightborder.bottom-13),
									BPoint(rightborder.right, rightborder.bottom-13),
									framecolors[2]);
				_driver->StrokeLine(BPoint(bottomborder.left-18, bottomborder.top),
									BPoint(bottomborder.right-18, bottomborder.bottom),
									framecolors[2]);
				break;
			}
			
			default:{
				// draw no resize corner
				break;
			}
		}
	}
}
