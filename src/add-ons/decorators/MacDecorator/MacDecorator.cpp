/*
 * Copyright 2009-2010, Haiku.
 * Distributed under the terms of the MIT License.
 */


/*! Decorator looking like Mac OS 9 */


#include "MacDecorator.h"

#include <new>
#include <stdio.h>

#include <GradientLinear.h>
#include <Point.h>
#include <View.h>

#include "DesktopSettings.h"
#include "DrawingEngine.h"
#include "PatternHandler.h"
#include "RGBColor.h"


//#define DEBUG_DECORATOR
#ifdef DEBUG_DECORATOR
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif


MacDecorAddOn::MacDecorAddOn(image_id id, const char* name)
	:
	DecorAddOn(id, name)
{

}


Decorator*
MacDecorAddOn::_AllocateDecorator(DesktopSettings& settings, BRect rect)
{
	return new (std::nothrow)MacDecorator(settings, rect);
}


MacDecorator::MacDecorator(DesktopSettings& settings, BRect rect)
	:
	Decorator(settings, rect)
{
	_UpdateFont(settings);

	frame_highcol = (rgb_color){ 255, 255, 255, 255 };
	frame_midcol = (rgb_color){ 216, 216, 216, 255 };
	frame_lowcol = (rgb_color){ 110, 110, 110, 255 };
	frame_lowercol = (rgb_color){ 0, 0, 0, 255 };

	fButtonHighColor = (rgb_color){ 232, 232, 232, 255 };
	fButtonLowColor = (rgb_color){ 128, 128, 128, 255 };

	fFocusTextColor = settings.UIColor(B_WINDOW_TEXT_COLOR);
	fNonFocusTextColor = settings.UIColor(B_WINDOW_INACTIVE_TEXT_COLOR);

	_DoLayout();

	textoffset=5;

	STRACE(("MacDecorator()\n"));
}


MacDecorator::~MacDecorator()
{
	STRACE(("~MacDecorator()\n"));
}


// TODO : Add GetSettings


void
MacDecorator::Draw(BRect update)
{
	STRACE(("MacDecorator: Draw(%.1f,%.1f,%.1f,%.1f)\n",
		update.left, update.top, update.right, update.bottom));

	// We need to draw a few things: the tab, the borders,
	// and the buttons
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(update);
	_DrawTab(update);
}


void
MacDecorator::Draw()
{
	STRACE("MacDecorator::Draw()\n");
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(fBorderRect);
	_DrawTab(fTitleBarRect);
}


// TODO : add GetSizeLimits


Decorator::Region
MacDecorator::RegionAt(BPoint where, int32& tab) const
{
	// Let the base class version identify hits of the buttons and the tab.
	Region region = Decorator::RegionAt(where, tab);
	if (region != REGION_NONE)
		return region;

	// check the resize corner
	if (fTopTab->look == B_DOCUMENT_WINDOW_LOOK && fResizeRect.Contains(where))
		return REGION_RIGHT_BOTTOM_CORNER;

	// hit-test the borders
	if (!(fTopTab->flags & B_NOT_RESIZABLE)
		&& (fTopTab->look == B_TITLED_WINDOW_LOOK
			|| fTopTab->look == B_FLOATING_WINDOW_LOOK
			|| fTopTab->look == B_MODAL_WINDOW_LOOK)
		&& fBorderRect.Contains(where) && !fFrame.Contains(where)) {
		return REGION_BOTTOM_BORDER;
			// TODO: Determine the actual border!
	}

	return REGION_NONE;
}


void
MacDecorator::_DoLayout()
{
	const int32 kDefaultBorderWidth = 6;
	STRACE(("MacDecorator: Do Layout\n"));
	// Here we determine the size of every rectangle that we use
	// internally when we are given the size of the client rectangle.

	bool hasTab = false;

	if (fTopTab) {
		switch (fTopTab->look) {
			case B_MODAL_WINDOW_LOOK:
				fBorderWidth = kDefaultBorderWidth;
				break;
	
			case B_TITLED_WINDOW_LOOK:
			case B_DOCUMENT_WINDOW_LOOK:
				hasTab = true;
				fBorderWidth = kDefaultBorderWidth;
				break;
			case B_FLOATING_WINDOW_LOOK:
				hasTab = true;
				fBorderWidth = 3;
				break;
	
			case B_BORDERED_WINDOW_LOOK:
				fBorderWidth = 1;
				break;
	
			default:
				fBorderWidth = 0;
		}
	} else
		fBorderWidth = 0;
	fBorderRect=fFrame;
	fBorderRect.InsetBy(-fBorderWidth, -fBorderWidth);

	// calculate our tab rect
	if (hasTab) {
		fBorderRect.top +=3;

		font_height fontHeight;
		fDrawState.Font().GetHeight(fontHeight);

		// TODO the tab is drawn in a fixed height for now
		fTitleBarRect.Set(fFrame.left - fBorderWidth,
			fFrame.top - 23,
			((fFrame.right - fFrame.left) < 32.0 ?
				fFrame.left + 32.0 : fFrame.right) + fBorderWidth,
			fFrame.top - 3);
			
		fTopTab->tabRect = fTitleBarRect; // TODO actually handle multiple tabs

		fTopTab->zoomRect=fTitleBarRect;
		fTopTab->zoomRect.left=fTopTab->zoomRect.right - 12;
		fTopTab->zoomRect.bottom=fTopTab->zoomRect.top + 12;
		fTopTab->zoomRect.OffsetBy(-4, 4);

		fTopTab->closeRect=fTopTab->zoomRect;
		fTopTab->minimizeRect=fTopTab->zoomRect;

		fTopTab->closeRect.OffsetTo(fTitleBarRect.left + 4,
			fTitleBarRect.top + 4);

		fTopTab->zoomRect.OffsetBy(0 - (fTopTab->zoomRect.Width() + 4), 0);
		if (Title(fTopTab) && fDrawingEngine) {
			titlepixelwidth=fDrawingEngine->StringWidth(Title(fTopTab),
				strlen(Title(fTopTab)));

			if (titlepixelwidth<(fTopTab->zoomRect.left
					- fTopTab->closeRect.right-10)) {
				// start with offset from closerect.right
				textoffset=int(((fTopTab->zoomRect.left - 5)
					- (fTopTab->closeRect.right + 5)) / 2);
				textoffset-=int(titlepixelwidth / 2);

				// now make it the offset from fTabRect.left
				textoffset+=int(fTopTab->closeRect.right + 5
					- fTitleBarRect.left);
			} else
				textoffset=int(fTopTab->closeRect.right) + 5;
		} else {
			textoffset = 0;
			titlepixelwidth = 0;
		}
	} else {
		// no tab
		if (fTopTab) {
			fTopTab->tabRect.Set(0.0, 0.0, -1.0, -1.0);
			fTopTab->closeRect.Set(0.0, 0.0, -1.0, -1.0);
			fTopTab->zoomRect.Set(0.0, 0.0, -1.0, -1.0);
			fTopTab->minimizeRect.Set(0.0, 0.0, -1.0, -1.0);
		}
	}
}


void
MacDecorator::_DrawFrame(BRect invalid)
{
	if (fTopTab->look == B_NO_BORDER_WINDOW_LOOK)
		return;

	if (fBorderWidth <= 0)
		return;

	BRect r = fBorderRect;
	switch (fTopTab->look) {
		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
		case B_MODAL_WINDOW_LOOK:
		{
			if (IsFocus(fTopTab)) {
				BPoint offset = r.LeftTop();
				BPoint pt2 = r.LeftBottom();

				// Draw the left side of the frame
				fDrawingEngine->StrokeLine(offset,pt2,frame_lowercol);
				offset.x++;
				pt2.x++;
				pt2.y--;

				fDrawingEngine->StrokeLine(offset,pt2,frame_highcol);
				offset.x++;
				pt2.x++;
				pt2.y--;

				fDrawingEngine->StrokeLine(offset,pt2,frame_midcol);
				offset.x++;
				pt2.x++;
				fDrawingEngine->StrokeLine(offset,pt2,frame_midcol);
				offset.x++;
				pt2.x++;
				pt2.y--;

				fDrawingEngine->StrokeLine(offset,pt2,frame_lowcol);
				offset.x++;
				offset.y+=2;
				BPoint topleftpt=offset;
				pt2.x++;
				pt2.y--;

				fDrawingEngine->StrokeLine(offset,pt2,frame_lowercol);


				offset=r.RightTop();
				pt2=r.RightBottom();

				// Draw the right side of the frame
				fDrawingEngine->StrokeLine(offset,pt2,frame_lowercol);
				offset.x--;
				pt2.x--;

				fDrawingEngine->StrokeLine(offset,pt2,frame_lowcol);
				offset.x--;
				pt2.x--;

				fDrawingEngine->StrokeLine(offset,pt2,frame_midcol);
				offset.x--;
				pt2.x--;
				fDrawingEngine->StrokeLine(offset,pt2,frame_midcol);
				offset.x--;
				pt2.x--;

				fDrawingEngine->StrokeLine(offset,pt2,frame_highcol);
				offset.x--;
				offset.y+=2;
				BPoint toprightpt=offset;
				pt2.x--;

				fDrawingEngine->StrokeLine(offset,pt2,frame_lowercol);

				// Draw the top side of the frame that is not in the tab
				offset=r.RightTop();
				pt2=r.RightBottom();

				fDrawingEngine->StrokeLine(topleftpt,toprightpt,frame_lowercol);
				topleftpt.y--;
				toprightpt.x++;
				toprightpt.y--;

				fDrawingEngine->StrokeLine(topleftpt,toprightpt,frame_lowcol);

				offset=r.LeftBottom();
				pt2=r.RightBottom();

				// Draw the bottom side of the frame
				fDrawingEngine->StrokeLine(offset,pt2,frame_lowercol);
				offset.x++;
				offset.y--;
				pt2.x--;
				pt2.y--;

				fDrawingEngine->StrokeLine(offset,pt2,frame_lowcol);
				offset.x++;
				offset.y--;
				pt2.x--;
				pt2.y--;

				fDrawingEngine->StrokeLine(offset,pt2,frame_midcol);
				offset.x++;
				offset.y--;
				pt2.x--;
				pt2.y--;

				fDrawingEngine->StrokeLine(offset,pt2,frame_midcol);
				offset.x++;
				offset.y--;
				pt2.x--;
				pt2.y--;

				fDrawingEngine->StrokeLine(offset,pt2,frame_highcol);
				offset.x+=2;
				offset.y--;
				pt2.x--;
				pt2.y--;

				fDrawingEngine->StrokeLine(offset,pt2,frame_lowercol);
				offset.y--;
				pt2.x--;
				pt2.y--;
			} else {
				fDrawingEngine->StrokeLine(r.LeftTop(), r.LeftBottom(), frame_lowcol);
				fDrawingEngine->StrokeLine(r.RightTop(), r.RightBottom(), frame_lowcol);
				fDrawingEngine->StrokeLine(r.LeftBottom(), r.RightBottom(), frame_lowcol);

				for (int i = 0; i < 4; i++) {
					r.InsetBy(1, 1);
					fDrawingEngine->StrokeLine(r.LeftTop(), r.LeftBottom(), frame_midcol);
					fDrawingEngine->StrokeLine(r.RightTop(), r.RightBottom(), frame_midcol);
					fDrawingEngine->StrokeLine(r.LeftBottom(), r.RightBottom(), frame_midcol);
				}

				r.InsetBy(1, 1);
				fDrawingEngine->StrokeLine(r.LeftTop(), r.LeftBottom(), frame_lowcol);
				fDrawingEngine->StrokeLine(r.RightTop(), r.RightBottom(), frame_lowcol);
				fDrawingEngine->StrokeLine(r.LeftBottom(), r.RightBottom(), frame_lowcol);
			}
			break;
		}
		case B_BORDERED_WINDOW_LOOK:
		fDrawingEngine->StrokeRect(r, frame_midcol);
		break;

		default:
		// don't draw a border frame
		break;
	}
}


void
MacDecorator::_DrawTab(BRect invalid)
{
	// If a window has a tab, this will draw it and any buttons which are
	// in it.
	if (!fTitleBarRect.IsValid() || !invalid.Intersects(fTitleBarRect))
		return;

	BRect rect(fTitleBarRect);
	fDrawingEngine->SetHighColor(RGBColor(frame_midcol));
	fDrawingEngine->FillRect(rect,frame_midcol);

	if (IsFocus(fTopTab)) {
		fDrawingEngine->StrokeLine(rect.LeftTop(),rect.RightTop(),frame_lowercol);
		fDrawingEngine->StrokeLine(rect.LeftTop(),rect.LeftBottom(),frame_lowercol);
		fDrawingEngine->StrokeLine(rect.RightBottom(),rect.RightTop(),frame_lowercol);

		rect.InsetBy(1,1);
		rect.bottom++;

		fDrawingEngine->StrokeLine(rect.LeftTop(),rect.RightTop(),frame_highcol);
		fDrawingEngine->StrokeLine(rect.LeftTop(),rect.LeftBottom(),frame_highcol);
		fDrawingEngine->StrokeLine(rect.RightBottom(),rect.RightTop(),frame_lowcol);

		// Draw the neat little lines on either side of the title if there's room
		if (fTitleBarRect.left + textoffset > fTopTab->closeRect.right + 5) {
			// Left side

			BPoint offset(fTopTab->closeRect.right+5,fTopTab->closeRect.top),
				pt2(fTitleBarRect.left+textoffset-5,fTopTab->closeRect.top);
			fDrawState.SetHighColor(RGBColor(frame_highcol));
			for (int32 i = 0; i < 6; i++) {
				fDrawingEngine->StrokeLine(offset,pt2,fDrawState.HighColor());
				offset.y+=2;
				pt2.y+=2;
			}

			offset.Set(fTopTab->closeRect.right+6,fTopTab->closeRect.top+1),
				pt2.Set(fTitleBarRect.left+textoffset-4,fTopTab->closeRect.top+1);
			fDrawState.SetHighColor(RGBColor(frame_lowcol));
			for (int32 i = 0; i < 6; i++) {
				fDrawingEngine->StrokeLine(offset, pt2, fDrawState.HighColor());
				offset.y += 2;
				pt2.y += 2;
			}

			// Right side

			offset.Set(fTitleBarRect.left + textoffset + titlepixelwidth + 6,
				fTopTab->zoomRect.top), pt2.Set(fTopTab->zoomRect.left - 6,
				fTopTab->zoomRect.top);
			if (offset.x < pt2.x) {
				fDrawState.SetHighColor(RGBColor(frame_highcol));
				for (int32 i = 0; i < 6; i++) {
					fDrawingEngine->StrokeLine(offset, pt2,
						fDrawState.HighColor());
					offset.y += 2;
					pt2.y += 2;
				}
				offset.Set(fTitleBarRect.left+textoffset + titlepixelwidth + 7,
					fTopTab->zoomRect.top + 1), pt2.Set(fTopTab->zoomRect.left - 5,
					fTopTab->zoomRect.top + 1);
				fDrawState.SetHighColor(frame_lowcol);
				for(int32 i = 0; i < 6; i++) {
					fDrawingEngine->StrokeLine(offset, pt2,
						fDrawState.HighColor());
					offset.y += 2;
					pt2.y += 2;
				}
			}
		}

		// Draw the buttons if we're supposed to
		if (!(fTopTab->flags & B_NOT_CLOSABLE))
			_DrawClose(fTopTab->closeRect);
		if (!(fTopTab->flags & B_NOT_ZOOMABLE))
			_DrawZoom(fTopTab->zoomRect);
	} else {
		// Not focused - Just draw a plain light grey area with the title in the middle
		fDrawingEngine->StrokeLine(rect.LeftTop(),rect.RightTop(),frame_lowcol);
		fDrawingEngine->StrokeLine(rect.LeftTop(),rect.LeftBottom(),frame_lowcol);
		fDrawingEngine->StrokeLine(rect.RightBottom(),rect.RightTop(),frame_lowcol);
	}

	_DrawTitle(fTitleBarRect);
}


void
MacDecorator::_DrawClose(BRect r)
{
	bool down = fTopTab->closePressed;

	// Just like DrawZoom, but for a close button
	BRect rect(r);

	BPoint offset(r.LeftTop()), pt2(r.RightTop());

	// Topleft dark grey border
	pt2.x--;
	fDrawingEngine->SetHighColor(RGBColor(136, 136, 136));
	fDrawingEngine->StrokeLine(offset, pt2);

	pt2 = r.LeftBottom();
	pt2.y--;
	fDrawingEngine->StrokeLine(offset, pt2);

	// Bottomright white border
	offset = r.RightBottom();
	pt2 = r.RightTop();
	pt2.y++;
	fDrawingEngine->SetHighColor(RGBColor(255, 255, 255));
	fDrawingEngine->StrokeLine(offset, pt2);

	pt2 = r.LeftBottom();
	pt2.x++;
	fDrawingEngine->StrokeLine(offset, pt2);

	// Black outline
	rect.InsetBy(1, 1);
	fDrawingEngine->SetHighColor(RGBColor(0, 0, 0));
	fDrawingEngine->StrokeRect(rect);

	// Double-shaded button
	rect.InsetBy(1, 1);
	_DrawBlendedRect(fDrawingEngine, rect, down);
	rect.InsetBy(1, 1);
	_DrawBlendedRect(fDrawingEngine, rect, !down);
}


void
MacDecorator::_DrawTitle(BRect rect)
{
	if (IsFocus(fTopTab))
		fDrawingEngine->SetHighColor(fFocusTextColor);
	else
		fDrawingEngine->SetHighColor(fNonFocusTextColor);

	fDrawingEngine->SetLowColor(frame_midcol);

	fTruncatedTitle = Title(fTopTab);
	fDrawState.Font().TruncateString(&fTruncatedTitle, B_TRUNCATE_END,
		(fTopTab->zoomRect.left - 5) - (fTopTab->closeRect.right + 5));
	fTruncatedTitleLength = fTruncatedTitle.Length();
	fDrawingEngine->SetFont(fDrawState.Font());

	fDrawingEngine->DrawString(fTruncatedTitle,fTruncatedTitleLength,
		BPoint(fTitleBarRect.left+textoffset,fTopTab->closeRect.bottom-1));
}


void
MacDecorator::_DrawZoom(BRect r)
{
	bool down = fTopTab->zoomPressed;

	// Just like DrawZoom, but for a close button
	BRect rect(r);
	BPoint offset(r.LeftTop()),pt2(r.RightTop());

	pt2.x--;
	fDrawState.SetHighColor(RGBColor(136, 136, 136));
	fDrawingEngine->StrokeLine(offset, pt2, fDrawState.HighColor());

	pt2 = r.LeftBottom();
	pt2.y--;
	fDrawingEngine->StrokeLine(offset, pt2, fDrawState.HighColor());

	offset = r.RightBottom();
	pt2 = r.RightTop();
	pt2.y++;
	fDrawState.SetHighColor(RGBColor(255, 255, 255));
	fDrawingEngine->StrokeLine(offset, pt2, fDrawState.HighColor());

	pt2 = r.LeftBottom();
	pt2.x++;
	fDrawingEngine->StrokeLine(offset, pt2, fDrawState.HighColor());

	rect.InsetBy(1, 1);
	fDrawState.SetHighColor(RGBColor(0, 0, 0));
	fDrawingEngine->StrokeRect(rect, fDrawState.HighColor());

	rect.InsetBy(1, 1);
	_DrawBlendedRect(fDrawingEngine, rect, down);
	rect.InsetBy(1,1);
	_DrawBlendedRect(fDrawingEngine, rect, !down);

	rect.top += 2;
	rect.left--;
	rect.right++;

	fDrawState.SetHighColor(RGBColor(0, 0, 0));
	fDrawingEngine->StrokeLine(rect.LeftTop(), rect.RightTop(),
		fDrawState.HighColor());
}


void
MacDecorator::_DrawMinimize(BRect r)
{
	bool down = fTopTab->minimizePressed;

	// Just like DrawZoom, but for a Minimize button
	BRect rect(r);
	BPoint offset(r.LeftTop()), pt2(r.RightTop());

	pt2.x--;
	fDrawState.SetHighColor(RGBColor(136, 136, 136));
	fDrawingEngine->StrokeLine(offset, pt2, fDrawState.HighColor());

	pt2 = r.LeftBottom();
	pt2.y--;
	fDrawingEngine->StrokeLine(offset, pt2, fDrawState.HighColor());

	offset = r.RightBottom();
	pt2 = r.RightTop();
	pt2.y++;
	fDrawState.SetHighColor(RGBColor(255, 255, 255));
	fDrawingEngine->StrokeLine(offset, pt2, fDrawState.HighColor());

	pt2 = r.LeftBottom();
	pt2.x++;
	fDrawingEngine->StrokeLine(offset, pt2, fDrawState.HighColor());

	rect.InsetBy(1, 1);
	fDrawState.SetHighColor(RGBColor(0, 0, 0));
	fDrawingEngine->StrokeRect(rect, fDrawState.HighColor());

	rect.InsetBy(1, 1);
	_DrawBlendedRect(fDrawingEngine, rect, down);
	rect.InsetBy(1, 1);
	_DrawBlendedRect(fDrawingEngine, rect, !down);

	rect.top += 4;
	rect.bottom -= 4;
	rect.InsetBy(-2, 0);

	fDrawState.SetHighColor(RGBColor(0, 0, 0));
	fDrawingEngine->StrokeRect(rect, fDrawState.HighColor());
}


void
MacDecorator::_SetTitle(Tab* tab, const char* string, BRegion* updateRegion)
{
	// TODO: we could be much smarter about the update region
	// TODO may this change the other tabs too ? (to make space for a longer
	// title ?)

	BRect rect = TabRect(fTopTab);

	_DoLayout();

	if (updateRegion == NULL)
		return;

	rect = rect | TabRect(fTopTab);

	rect.bottom++;
		// the border will look differently when the title is adjacent

	updateRegion->Include(rect);
}


void
MacDecorator::_FontsChanged(DesktopSettings& settings,
	BRegion* updateRegion)
{
	// get previous extent
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());

	_UpdateFont(settings);
	_DoLayout();

	_InvalidateFootprint();
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());
}


void
MacDecorator::_SetLook(DesktopSettings& settings, window_look look,
	BRegion* updateRegion)
{
	// TODO: we could be much smarter about the update region

	// get previous extent
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());

	fTopTab->look = look;

	_UpdateFont(settings);
	_DoLayout();

	_InvalidateFootprint();
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());
}


void
MacDecorator::_SetFlags(uint32 flags, BRegion* updateRegion)
{
	// TODO: we could be much smarter about the update region

	// get previous extent
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());

	_DoLayout();

	_InvalidateFootprint();
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());
}


// TODO : _SetFocus


void
MacDecorator::_MoveBy(BPoint offset)
{
	// TODO handle all tabs
	fFrame.OffsetBy(offset);
	fTopTab->closeRect.OffsetBy(offset);
	fTitleBarRect.OffsetBy(offset);
	fTopTab->tabRect = fTitleBarRect;
	fResizeRect.OffsetBy(offset);
	fTopTab->zoomRect.OffsetBy(offset);
	fBorderRect.OffsetBy(offset);
}


void
MacDecorator::_ResizeBy(BPoint offset, BRegion* dirty)
{
	// Move all internal rectangles the appropriate amount
	fFrame.right += offset.x;
	fFrame.bottom += offset.y;

	fTitleBarRect.right += offset.x;
	fBorderRect.right += offset.x;
	fBorderRect.bottom += offset.y;
	// fZoomRect.OffsetBy(offset.x, 0);
	// fMinimizeRect.OffsetBy(offset.x, 0);
	if (dirty) {
		dirty->Include(fTitleBarRect);
		dirty->Include(fBorderRect);
	}


	// TODO probably some other layouting stuff here
	_DoLayout();
}


// TODO : _SetSettings


void
MacDecorator::_GetFootprint(BRegion* region)
{
	// This function calculates the decorator's footprint in coordinates
	// relative to the view. This is most often used to set a Window
	// object's visible region.
	if (!region)
		return;

	region->MakeEmpty();

	if (fTopTab->look == B_NO_BORDER_WINDOW_LOOK)
		return;

	region->Set(fBorderRect);
	region->Exclude(fFrame);

	if (fTopTab->look == B_BORDERED_WINDOW_LOOK)
		return;
	region->Include(fTitleBarRect);
}


void
MacDecorator::_UpdateFont(DesktopSettings& settings)
{
	ServerFont font;
	if (fTopTab && fTopTab->look == B_FLOATING_WINDOW_LOOK)
		settings.GetDefaultPlainFont(font);
	else
		settings.GetDefaultBoldFont(font);

	font.SetFlags(B_FORCE_ANTIALIASING);
	font.SetSpacing(B_STRING_SPACING);
	fDrawState.SetFont(font);
}


/*!	\brief Draws a rectangle with a gradient.
  \param down The rectangle should be drawn recessed or not
*/
void
MacDecorator::_DrawBlendedRect(DrawingEngine* engine, BRect rect,
		bool down/*, bool focus*/)
{
	// figure out which colors to use
	rgb_color startColor, endColor;
	if (down) {
		startColor = fButtonLowColor;
		endColor = frame_highcol;
	} else {
		startColor = fButtonHighColor;
		endColor = frame_lowercol;
	}

	// fill
	BGradientLinear gradient;
	gradient.SetStart(rect.LeftTop());
	gradient.SetEnd(rect.RightBottom());
	gradient.AddColor(startColor, 0);
	gradient.AddColor(endColor, 255);

	engine->FillRect(rect, gradient);
}


Decorator::Tab*
MacDecorator::_AllocateNewTab()
{
	Decorator::Tab* tab = new(std::nothrow) MacDecorator::Tab;
	if (tab == NULL)
		return NULL;
	// Set appropriate colors based on the current focus value. In this case,
	// each decorator defaults to not having the focus.
	_SetFocus(tab);
	return tab;
}


bool
MacDecorator::_AddTab(DesktopSettings& settings, int32 index,
	BRegion* updateRegion)
{
	_UpdateFont(settings);

	_DoLayout();
	if (updateRegion != NULL)
		updateRegion->Include(fTitleBarRect);
	return true;
}


bool
MacDecorator::_RemoveTab(int32 index, BRegion* updateRegion)
{
	BRect oldTitle = fTitleBarRect;
	_DoLayout();
	if (updateRegion != NULL) {
		updateRegion->Include(oldTitle);
		updateRegion->Include(fTitleBarRect);
	}
	return true;
}


bool
MacDecorator::_MoveTab(int32 from, int32 to, bool isMoving,
	BRegion* updateRegion)
{
	return false; //TODO
	#if 0
	MacDecorator::Tab* toTab = _TabAt(to);
	if (toTab == NULL)
		return false;

	if (from < to) {
		fOldMovingTab.OffsetBy(toTab->tabRect.Width(), 0);
		toTab->tabRect.OffsetBy(-fOldMovingTab.Width(), 0);
	} else {
		fOldMovingTab.OffsetBy(-toTab->tabRect.Width(), 0);
		toTab->tabRect.OffsetBy(fOldMovingTab.Width(), 0);
	}

	toTab->tabOffset = uint32(toTab->tabRect.left - fLeftBorder.left);
	_LayoutTabItems(toTab, toTab->tabRect);

	_CalculateTabsRegion();

	if (updateRegion != NULL)
		updateRegion->Include(fTitleBarRect);
	return true;
	#endif
}


// #pragma mark -


extern "C" DecorAddOn*
instantiate_decor_addon(image_id id, const char* name)
{
	return new (std::nothrow)MacDecorAddOn(id, name);
}
