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
//	File Name:		BeDecorator.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Fallback decorator for the app_server
//  
//------------------------------------------------------------------------------
#include <Rect.h>
#include "DisplayDriver.h"
#include <View.h>
#include "LayerData.h"
#include "ColorUtils.h"
#include "BeDecorator.h"
#include "PatternHandler.h"
#include "RGBColor.h"
#include "RectUtils.h"
#include <stdio.h>

#define USE_VIEW_FILL_HACK

#//#define DEBUG_DECORATOR
#ifdef DEBUG_DECORATOR
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

BeDecorator::BeDecorator(BRect rect, int32 wlook, int32 wfeel, int32 wflags)
 : Decorator(rect,wlook,wfeel,wflags)
{

	taboffset=0;
	titlepixelwidth=0;

	framecolors=new RGBColor[5];
	framecolors[0].SetColor(255,255,255);
	framecolors[1].SetColor(216,216,216);
	framecolors[2].SetColor(152,152,152);
	framecolors[3].SetColor(136,136,136);
	framecolors[4].SetColor(96,96,96);

	// Set appropriate colors based on the current focus value. In this case, each decorator
	// defaults to not having the focus.
	_SetFocus();
	
	// Do initial decorator setup
	_DoLayout();
	
	// This flag is used to determine whether or not we're moving the tab
	slidetab=false;

//	tab_highcol=_colors->window_tab;
//	tab_lowcol=_colors->window_tab;

STRACE(("BeDecorator:\n"));
STRACE(("\tFrame (%.1f,%.1f,%.1f,%.1f)\n",rect.left,rect.top,rect.right,rect.bottom));
}

BeDecorator::~BeDecorator(void)
{
STRACE(("BeDecorator: ~BeDecorator()\n"));
	delete [] framecolors;
}

click_type BeDecorator::Clicked(BPoint pt, int32 buttons, int32 modifiers)
{
	#ifdef DEBUG_DECORATOR
	printf("BeDecorator: Clicked\n");
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
	BRect brect(_frame);
	brect.top+=19;
	BRect clientrect(brect.InsetByCopy(3,3));
	if(brect.Contains(pt) && !clientrect.Contains(pt))
	{
		if(_resizerect.Contains(pt))
			return DEC_RESIZE;
		
		return DEC_DRAG;
	}

	// Guess user didn't click anything
	return DEC_NONE;
}

void BeDecorator::_DoLayout(void)
{
	STRACE(("BeDecorator: Do Layout\n"));
	// Here we determine the size of every rectangle that we use
	// internally when we are given the size of the client rectangle.

	switch(GetLook())
	{
		case B_FLOATING_WINDOW_LOOK:
		case B_MODAL_WINDOW_LOOK:
		
		// We're going to make the frame 5 pixels wide, no matter what. R5's decorator frame
		// requires the skills of a gaming master to click on the tiny frame if resizing is necessary,
		// and there *are* apps which do this
//			borderwidth=3;
//			break;

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
					_frame.top - (borderwidth-1) );

	// make it text width sensitive
	if(strlen(GetTitle())>1)
	{
		if(_driver)
			titlepixelwidth=_driver->StringWidth(GetTitle(),_TitleWidth(), &_layerdata);
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
		_borderrect		= _frame.InsetByCopy( -borderwidth, -borderwidth );
		leftborder.Set( _borderrect.left, _frame.top - borderwidth,
						_frame.left, _frame.bottom + borderwidth );
		rightborder.Set( _frame.right, _frame.top - borderwidth,
						 _borderrect.right, _frame.bottom + borderwidth );
		topborder.Set( 	_borderrect.left, _borderrect.top,
						_borderrect.right, _frame.top );
		bottomborder.Set( _borderrect.left, _frame.bottom,
						  _borderrect.right, _borderrect.bottom );
	}
	else{
		// no border ... (?) useful when displaying windows that are just images
		_borderrect	= _frame;
		leftborder.Set( 0.0, 0.0, -1.0, -1.0 );
		rightborder.Set( 0.0, 0.0, -1.0, -1.0 );
		topborder.Set( 0.0, 0.0, -1.0, -1.0 );
		bottomborder.Set( 0.0, 0.0, -1.0, -1.0 );						
	}

	// calculate resize rect
	_resizerect.Set(	_borderrect.right - 19.0, _borderrect.bottom - 19.0,
						_borderrect.right, _borderrect.bottom);

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

void BeDecorator::MoveBy(float x, float y)
{
	MoveBy(BPoint(x,y));
}

void BeDecorator::MoveBy(BPoint pt)
{
	STRACE(("BeDecorator: Move By (%.1f, %.1f)\n",pt.x,pt.y));
	// Move all internal rectangles the appropriate amount
	_frame.OffsetBy(pt);
	_closerect.OffsetBy(pt);
	_tabrect.OffsetBy(pt);
	_resizerect.OffsetBy(pt);
	_borderrect.OffsetBy(pt);
	_zoomrect.OffsetBy(pt);
	
	leftborder.OffsetBy(pt);
	rightborder.OffsetBy(pt);
	topborder.OffsetBy(pt);
	bottomborder.OffsetBy(pt);
}

void BeDecorator::GetFootprint(BRegion *region)
{
	STRACE(("BeDecorator: Get Footprint\n"));
	// This function calculates the decorator's footprint in coordinates
	// relative to the layer. This is most often used to set a WinBorder
	// object's visible region.
	if(!region)
		return;

	if(_look == B_NO_BORDER_WINDOW_LOOK)
	{
		region->Set(_frame);
		return;
	}
	
	if(_look == B_BORDERED_WINDOW_LOOK)
	{
		region->Set(_borderrect);
		return;
	}

	region->Set(_borderrect);
	region->Include(_tabrect);

}


BRect BeDecorator::SlideTab(float dx, float dy)
{
	//return Decorator::SlideTab(dx,dy);
	return _tabrect;
}

void BeDecorator::_DrawTitle(BRect r)
{
	STRACE(("_DrawTitle(%f,%f,%f,%f)\n", r.left, r.top, r.right, r.bottom));
	// Designed simply to redraw the title when it has changed on
	// the client side.
	_layerdata.highcolor=_colors->window_tab_text;
	_layerdata.lowcolor=(GetFocus())?_colors->window_tab:_colors->inactive_window_tab;

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
		BPoint(_closerect.right+textoffset,_closerect.bottom+1),&_layerdata);
	else	
	_driver->DrawString(titlestr.String(),titlecount,
		BPoint(_closerect.right+textoffset,_closerect.bottom),&_layerdata);
}

void BeDecorator::_SetFocus(void)
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

void BeDecorator::Draw(BRect update)
{
	STRACE(("BeDecorator: Draw(%.1f,%.1f,%.1f,%.1f)\n",update.left,update.top,update.right,update.bottom));

	// We need to draw a few things: the tab, the resize thumb, the borders,
	// and the buttons

	_DrawFrame(update);
	_DrawTab(update);
}

void BeDecorator::Draw(void)
{
	// Easy way to draw everything - no worries about drawing only certain
	// things
	_DrawFrame(_borderrect);
	_DrawTab(_tabrect);

}

void BeDecorator::_DrawZoom(BRect r)
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

void BeDecorator::_DrawClose(BRect r)
{
	STRACE(("_DrawClose(%f,%f,%f,%f)\n", r.left, r.top, r.right, r.bottom));
	// Just like DrawZoom, but for a close button
	DrawBlendedRect( r, GetClose());
}

void BeDecorator::_DrawTab(BRect r)
{
	STRACE(("_DrawTab(%f,%f,%f,%f)\n", r.left, r.top, r.right, r.bottom));
	// If a window has a tab, this will draw it and any buttons which are
	// in it.
	if(_look == B_NO_BORDER_WINDOW_LOOK || _look == B_BORDERED_WINDOW_LOOK)
		return;
	
	_layerdata.highcolor=(GetFocus())?_colors->window_tab:_colors->inactive_window_tab;
	_driver->FillRect(_tabrect,_layerdata.highcolor);
	
	_layerdata.highcolor=framecolors[2];
	_driver->StrokeLine(_tabrect.LeftTop(),_tabrect.LeftBottom(),_layerdata.pensize,_layerdata.highcolor);
	_driver->StrokeLine(_tabrect.LeftTop(),_tabrect.RightTop(),_layerdata.pensize,_layerdata.highcolor);
	_layerdata.highcolor=framecolors[4];
	_driver->StrokeLine(_tabrect.RightTop(),_tabrect.RightBottom(),_layerdata.pensize,_layerdata.highcolor);
	_layerdata.highcolor=framecolors[1];	
	_driver->StrokeLine( BPoint( _tabrect.left + 2, _tabrect.bottom ),
						 BPoint( _tabrect.right - 2, _tabrect.bottom ),
						 _layerdata.pensize,_layerdata.highcolor);
	
	_DrawTitle(_tabrect);

	// Draw the buttons if we're supposed to	
	if(!(_flags & B_NOT_CLOSABLE))
		_DrawClose(_closerect);
	if(!(_flags & B_NOT_ZOOMABLE) && _look != B_MODAL_WINDOW_LOOK)
		_DrawZoom(_zoomrect);
}

void BeDecorator::_SetColors(void)
{
	_SetFocus();
}

void BeDecorator::DrawBlendedRect(BRect r, bool down)
{
	// This bad boy is used to draw a rectangle with a gradient.
	// Note that it is not part of the Decorator API - it's specific
	// to just the BeDecorator. Called by DrawZoom and DrawClose

	// Actually just draws a blended square
	int32 w=r.IntegerWidth(),  h=r.IntegerHeight();

	rgb_color tmpcol,halfcol, startcol, endcol;
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
		SetRGBColor(&tmpcol, uint8(startcol.red-(i*rstep)),
			uint8(startcol.green-(i*gstep)),
			uint8(startcol.blue-(i*bstep)));
		_layerdata.highcolor=tmpcol;

		_driver->StrokeLine(BPoint(r.left,r.top+i),
			BPoint(r.left+i,r.top),1.0,_layerdata.highcolor);

		SetRGBColor(&tmpcol, uint8(halfcol.red-(i*rstep)),
			uint8(halfcol.green-(i*gstep)),
			uint8(halfcol.blue-(i*bstep)));
		_layerdata.highcolor=tmpcol;

		_driver->StrokeLine(BPoint(r.left+steps,r.top+i),
			BPoint(r.left+i,r.top+steps),1.0,_layerdata.highcolor);

	}

//	_layerdata.highcolor=startcol;
//	_driver->FillRect(r,&_layerdata,pat_solidhigh);
	_driver->StrokeRect(r,1.0,framecolors[3]);
}

void BeDecorator::_DrawFrame(BRect invalid)
{

	// We need to test each side to determine whether or not it needs drawn. Additionally,
	// we must clip the lines drawn by this function to the invalid rectangle we are given
	
	#ifdef USE_VIEW_FILL_HACK
	_layerdata.highcolor = RGBColor(192,192,192 );	
	_driver->FillRect(_frame,_layerdata.highcolor);
	#endif

	if(!borderwidth)
		return;
	
	// Data specifically for the StrokeLineArray call.
	int32 numlines=0, maxlines=20;

	BPoint points[maxlines*2];
	RGBColor colors[maxlines];
	
	// For quick calculation of gradients for each side. Top is same as left, right is same as
	// bottom
//	int8 rightindices[borderwidth],leftindices[borderwidth];
	int8 *rightindices=new int8[borderwidth],
		*leftindices=new int8[borderwidth];
	
	if(borderwidth==5)
	{
		leftindices[0]=2;
		leftindices[1]=0;
		leftindices[2]=1;
		leftindices[3]=3;
		leftindices[4]=2;

		rightindices[0]=2;
		rightindices[1]=0;
		rightindices[2]=1;
		rightindices[3]=3;
		rightindices[4]=4;
	}
	else
	{
		// TODO: figure out border colors for floating window look
		leftindices[0]=2;
		leftindices[1]=2;
		leftindices[2]=1;
		leftindices[3]=1;
		leftindices[4]=4;

		rightindices[0]=2;
		rightindices[1]=2;
		rightindices[2]=1;
		rightindices[3]=1;
		rightindices[4]=4;
	}
	
	// Variables used in each calculation
	int32 startx,endx,starty,endy,i;
	bool topcorner,bottomcorner,leftcorner,rightcorner;
	int8 step,colorindex;
	BRect r;
	BPoint start, end;
	
	// Right side
	if(TestRectIntersection(rightborder,invalid))
	{
		
		// We may not have to redraw the entire width of the frame itself. Rare case, but
		// it must be accounted for.
		startx=(int32) MAX(invalid.left,rightborder.left);
		endx=(int32) MIN(invalid.right,rightborder.right);

		// We'll need these flags to see if we must include the corners in final line
		// calculations
		r=(rightborder);
		r.bottom=r.top+borderwidth;
		topcorner=TestRectIntersection(invalid,r);

		r=rightborder;
		r.top=r.bottom-borderwidth;
		bottomcorner=TestRectIntersection(invalid,r);
		step=(borderwidth==5)?1:2;
		colorindex=0;
		
		// Generate the lines for this side
		for(i=startx+1; i<=endx; i++)
		{
			start.x=end.x=i;
			
			if(topcorner)
			{
				start.y=rightborder.top+(borderwidth-(i-rightborder.left));
				start.y=MAX(start.y,invalid.top);
			}
			else
				start.y=MAX(start.y+borderwidth,invalid.top);

			if(bottomcorner)
			{
				end.y=rightborder.bottom-(borderwidth-(i-rightborder.left));
				end.y=MIN(end.y,invalid.bottom);
			}
			else
				end.y=MIN(end.y-borderwidth,invalid.bottom);
							
			// Make the appropriate 
			points[numlines*2]=start;
			points[(numlines*2)+1]=end;
			colors[numlines]=framecolors[rightindices[colorindex]];
			colorindex+=step;
			numlines++;
		}
	}

	// Left side
	if(TestRectIntersection(leftborder,invalid))
	{
		
		// We may not have to redraw the entire width of the frame itself. Rare case, but
		// it must be accounted for.
		startx=(int32) MAX(invalid.left,leftborder.left);
		endx=(int32) MIN(invalid.right,leftborder.right);

		// We'll need these flags to see if we must include the corners in final line
		// calculations
		r=leftborder;
		r.bottom=r.top+borderwidth;
		topcorner=TestRectIntersection(invalid,r);

		r=leftborder;
		r.top=r.bottom-borderwidth;
		bottomcorner=TestRectIntersection(invalid,r);
		step=(borderwidth==5)?1:2;
		colorindex=0;
		
		// Generate the lines for this side
		for(i=startx; i<endx; i++)
		{
			start.x=end.x=i;
			
			if(topcorner)
			{
				start.y=leftborder.top+(i-leftborder.left);
				start.y=MAX(start.y,invalid.top);
			}
			else
				start.y=MAX(start.y+borderwidth,invalid.top);

			if(bottomcorner)
			{
				end.y=leftborder.bottom-(i-leftborder.left);
				end.y=MIN(end.y,invalid.bottom);
			}
			else
				end.y=MIN(end.y-borderwidth,invalid.bottom);
							
			// Make the appropriate 
			points[numlines*2]=start;
			points[(numlines*2)+1]=end;
			colors[numlines]=framecolors[leftindices[colorindex]];
			colorindex+=step;
			numlines++;
		}
	}

	// Top side
	if(TestRectIntersection(topborder,invalid))
	{
		
		// We may not have to redraw the entire width of the frame itself. Rare case, but
		// it must be accounted for.
		starty=(int32) MAX(invalid.top,topborder.top);
		endy=(int32) MIN(invalid.bottom,topborder.bottom);

		// We'll need these flags to see if we must include the corners in final line
		// calculations
		r=topborder;
		r.bottom=r.top+borderwidth;
		r.right=r.left+borderwidth;
		leftcorner=TestRectIntersection(invalid,r);

		r=topborder;
		r.top=r.bottom-borderwidth;
		r.left=r.right-borderwidth;
		
		rightcorner=TestRectIntersection(invalid,r);
		step=(borderwidth==5)?1:2;
		colorindex=0;
		
		// Generate the lines for this side
		for(i=starty; i<endy; i++)
		{
			start.y=end.y=i;
			
			if(leftcorner)
			{
				start.x=topborder.left+(i-topborder.top);
				start.x=MAX(start.x,invalid.left);
			}
			else
				start.x=MAX(start.x+borderwidth,invalid.left);

			if(rightcorner)
			{
				end.x=topborder.right-(i-topborder.top);
				end.x=MIN(end.x,invalid.right);
			}
			else
				end.x=MIN(end.x-borderwidth,invalid.right);
							
			// Make the appropriate 
			points[numlines*2]=start;
			points[(numlines*2)+1]=end;
			
			// Top side uses the same color order as the left one
			colors[numlines]=framecolors[leftindices[colorindex]];
			colorindex+=step;
			numlines++;
		}
	}

	// Bottom side
	if(TestRectIntersection(bottomborder,invalid))
	{
		
		// We may not have to redraw the entire width of the frame itself. Rare case, but
		// it must be accounted for.
		starty=(int32) MAX(invalid.top,bottomborder.top);
		endy=(int32) MIN(invalid.bottom,bottomborder.bottom);

		// We'll need these flags to see if we must include the corners in final line
		// calculations
		r=bottomborder;
		r.bottom=r.top+borderwidth;
		r.right=r.left+borderwidth;
		leftcorner=TestRectIntersection(invalid,r);

		r=bottomborder;
		r.top=r.bottom-borderwidth;
		r.left=r.right-borderwidth;
		
		rightcorner=TestRectIntersection(invalid,r);
		step=(borderwidth==5)?1:2;
		colorindex=0;
		
		// Generate the lines for this side
		for(i=starty+1; i<=endy; i++)
		{
			start.y=end.y=i;
			
			if(leftcorner)
			{
				start.x=bottomborder.left+(borderwidth-(i-bottomborder.top));
				start.x=MAX(start.x,invalid.left);
			}
			else
				start.x=MAX(start.x+borderwidth,invalid.left);

			if(rightcorner)
			{
				end.x=bottomborder.right-(borderwidth-(i-bottomborder.top));
				end.x=MIN(end.x,invalid.right);
			}
			else
				end.x=MIN(end.x-borderwidth,invalid.right);
							
			// Make the appropriate 
			points[numlines*2]=start;
			points[(numlines*2)+1]=end;
			
			// Top side uses the same color order as the left one
			colors[numlines]=framecolors[rightindices[colorindex]];
			colorindex+=step;
			numlines++;
		}
	}

	_driver->StrokeLineArray(points,numlines,1.0,colors);
	
	delete rightindices;
	delete leftindices;
	
	// Draw the resize thumb if we're supposed to
	if(!(_flags & B_NOT_RESIZABLE))
	{
		r=_resizerect;

//		int32 w=r.IntegerWidth(),  h=r.IntegerHeight();
		
		// This code is strictly for B_DOCUMENT_WINDOW looks
		if(_look==B_DOCUMENT_WINDOW_LOOK)
		{
			r.right-=4;
			r.bottom-=4;

			_driver->StrokeLine(r.LeftTop(),r.RightTop(),1.0,framecolors[2]);
			_driver->StrokeLine(r.LeftTop(),r.LeftBottom(),1.0,framecolors[2]);

			r.OffsetBy(1,1);
			_driver->StrokeLine(r.LeftTop(),r.RightTop(),1.0,framecolors[0]);
			_driver->StrokeLine(r.LeftTop(),r.LeftBottom(),1.0,framecolors[0]);
			
			r.OffsetBy(1,1);
			_driver->FillRect(r,framecolors[1]);
			
/*			r.left+=2;
			r.top+=2;
			r.right-=3;
			r.bottom-=3;
*/
			r.right-=2;
			r.bottom-=2;
			int32 w=r.IntegerWidth(),  h=r.IntegerHeight();
		
			rgb_color halfcol, startcol, endcol;
			float rstep,gstep,bstep,i;
			
			int steps=(w<h)?w:h;
		
			startcol=framecolors[0].GetColor32();
			endcol=framecolors[4].GetColor32();
		
			halfcol=framecolors[0].MakeBlendColor(framecolors[4],0.5).GetColor32();
		
			rstep=(startcol.red-halfcol.red)/steps;
			gstep=(startcol.green-halfcol.green)/steps;
			bstep=(startcol.blue-halfcol.blue)/steps;
			
			// Explicitly locking the driver is normally unnecessary. However, we need to do
			// this because we are rapidly drawing a series of calls which would not necessarily
			// draw correctly if we didn't do so.
			_driver->Lock();
			for(i=0;i<=steps; i++)
			{
				_layerdata.highcolor.SetColor(uint8(startcol.red-(i*rstep)),
					uint8(startcol.green-(i*gstep)),
					uint8(startcol.blue-(i*bstep)));
				
				_driver->StrokeLine(BPoint(r.left,r.top+i),
					BPoint(r.left+i,r.top),1.0,_layerdata.highcolor);
		
				_layerdata.highcolor.SetColor(uint8(halfcol.red-(i*rstep)),
					uint8(halfcol.green-(i*gstep)),
					uint8(halfcol.blue-(i*bstep)));
				_driver->StrokeLine(BPoint(r.left+steps,r.top+i),
					BPoint(r.left+i,r.top+steps),1.0,_layerdata.highcolor);			
			}
			_driver->Unlock();
//			_layerdata.highcolor=framecolors[4];
//			_driver->StrokeRect(r,&_layerdata,pat_solidhigh);
		}
		else
		{
			_layerdata.highcolor=framecolors[4];
			_driver->StrokeLine(BPoint(r.right,r.top),BPoint(r.right-3,r.top),
				1.0,_layerdata.highcolor);
			_driver->StrokeLine(BPoint(r.left,r.bottom),BPoint(r.left,r.bottom-3),
				1.0,_layerdata.highcolor);
		}
	}
}

extern "C" float get_decorator_version(void)
{
	return 1.00;
}

extern "C" Decorator *instantiate_decorator(BRect rect, int32 wlook, int32 wfeel, int32 wflags)
{
	return new BeDecorator(rect,wlook,wfeel,wflags);
}
