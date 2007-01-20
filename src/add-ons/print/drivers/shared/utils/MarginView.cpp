/*

MarginView.cpp

Copyright (c) 2002 OpenBeOS.

Authors: 
	Philippe Houdoin
	Simon Gauvin	
	Michael Pfeiffer

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

	Todo:

	2 Make Strings constants or UI resources
	
*/

#include "MarginView.h"

#include <AppKit.h>
#include <SupportKit.h>

#include <stdio.h>
#include <stdlib.h>

/*----------------- MarginView Private Constants --------------------*/

const int kOffsetY = 20;
const int kOffsetX = 10;
const int kStringSize = 50;
const int kWidth = 50;
const int kNumCount = 10;

const static float kPointUnits = 1; // 1 point = 1 point 
const static float kInchUnits = 72; // 1" = 72 points
const static float kCMUnits = 28.346; // 72/2.54 1cm = 28.346 points

const static float kMinFieldWidth = 100; // pixels
const static float kMinUnitHeight = 30; // pixels
	
const static float kUnitFormat[] = { kInchUnits, kCMUnits, kPointUnits }; 
const static char *kUnitNames[] = { "Inch", "cm", "Points", NULL };
const static MarginUnit kUnitMsg[] = { kUnitInch, 
								  kUnitCM, 
								  kUnitPoint };

const pattern kDots = {{ 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55 }};

const rgb_color kBlack 	= { 0,0,0,0 };
const rgb_color kRed 	= { 255,0,0,0 };
const rgb_color kWhite 	= { 255,255,255,0 };
const rgb_color kGray 	= { 220,220,220,0 };

/*----------------- MarginView Public Methods --------------------*/

/**
 * Constructor
 *
 * @param frame, BRect that is the size of the view passed to the superclase
 * @param pageWidth, float that is the points value of the page width
 * @param pageHeight, float that is the points value of the page height
 * @param margins, BRect values of margins
 * @param units, unit32 enum for units used in view
 * @return void
 */
MarginView::MarginView(BRect frame,
		int32 pageWidth,
		int32 pageHeight,
		BRect margins,
		MarginUnit units)
		
	   	:BBox(frame, NULL, B_FOLLOW_ALL)
{
	fMarginUnit = units;
	fUnitValue = kUnitFormat[units];
	
	SetLabel("Margins");
	
	fMaxPageHeight = frame.Height() - kMinUnitHeight - kOffsetY;
	fMaxPageWidth = frame.Width() - kMinFieldWidth - kOffsetX;

	fMargins = margins;
		
	fPageWidth = pageWidth;
	fPageHeight = pageHeight; 
}

/**
 * Destructor
 *
 * @param none
 * @return void
 */
MarginView::~MarginView() {
}

/*----------------- MarginView Public BeOS Hook Methods --------------------*/

/**
 * Draw
 *
 * @param BRect, the draw bounds
 * @return void
 */
void 
MarginView::Draw(BRect rect)
{
	BBox::Draw(rect);

	float y_offset = (float)kOffsetY;
	float x_offset = (float)kOffsetX;
	BRect r;

	// Calculate offsets depending on orientation
	if (fPageWidth < fPageHeight) { // Portrait
		x_offset = (fMaxPageWidth/2 + kOffsetX) - fViewWidth/2;
	} else { // landscape
		y_offset = (fMaxPageHeight/2 + kOffsetY) - fViewHeight/2;
	}
	
	// draw the page
	SetHighColor(kWhite);
	r = BRect(0, 0, fViewWidth, fViewHeight);
	r.OffsetBy(x_offset, y_offset);
	FillRect(r);
	SetHighColor(kBlack);
	StrokeRect(r);

	// draw margin
	SetHighColor(kRed);
	SetLowColor(kWhite);
	r.top += fMargins.top;
	r.right -= fMargins.right;
	r.bottom -= fMargins.bottom;
	r.left += fMargins.left;
	StrokeRect(r, kDots);

	// draw the page size label
	SetHighColor(kBlack);
	SetLowColor(kGray);
	char str[kStringSize];
	sprintf(str, "%2.1f x %2.1f", fPageWidth/fUnitValue, fPageHeight/fUnitValue); 
	SetFontSize(10);
	DrawString((const char *)str, BPoint(x_offset, fMaxPageHeight + 40));
}


/**
 * BeOS Hook Function, change the size of the margin display
 *
 * @param width of the page
 * @param  height the page
 * @return void
 */
void 
MarginView::FrameResized(float width, float height)
{
	fMaxPageHeight = height - kMinUnitHeight - kOffsetX;
	fMaxPageWidth = width - kMinFieldWidth - kOffsetY;

	CalculateViewSize(MARGIN_CHANGED);
	Invalidate();
}

/**
 * AttachToWindow
 *
 * @param none
 * @return void
 */
void 
MarginView::AttachedToWindow()
{
	if (Parent()) {
		SetViewColor(Parent()->ViewColor());
	}
	ConstructGUI();
}

/*----------------- MarginView Public Methods --------------------*/

/**
 * GetUnits
 *
 * @param none
 * @return uint32 enum, units in inches, cm, points
 */
MarginUnit 
MarginView::GetMarginUnit(void) {
	return fMarginUnit;
}

/**
 * UpdateView, recalculate and redraw the view
 *
 * @param msg is a message to the calculate size to tell which field caused 
 *		the update to occur, or it is a general update.
 * @return void
 */
void 
MarginView::UpdateView(uint32 msg)
{
	Window()->Lock();
	CalculateViewSize(msg);								// nur Preview in Margins BBox!
	Invalidate();
	Window()->Unlock();
}

/**
 * SetPageSize
 *
 * @param pageWidth, float that is the unit value of the page width
 * @param pageHeight, float that is the unit value of the page height
 * @return void
 */
void 
MarginView::SetPageSize(float pageWidth, float pageHeight)
{
	fPageWidth = pageWidth;
	fPageHeight = pageHeight; 
}

/**
 * GetPageSize
 *
 * @param none
 * @return BPoint, contains actual point values of page in x, y of point
 */
BPoint 
MarginView::GetPageSize(void) {
	return BPoint(fPageWidth, fPageHeight);
}

/**
 * GetMargin
 *
 * @param none
 * @return rect, return margin values always in points
 */
BRect 
MarginView::GetMargin(void) 
{
	BRect margin;

	// convert the field text to values
	float ftop 		= atof(fTop->Text());
	float fright 	= atof(fRight->Text());
	float fleft 	= atof(fLeft->Text());
	float fbottom 	= atof(fBottom->Text());

	// convert to units to points
	switch (fMarginUnit)  
	{	
		case kUnitInch:
			// convert to points
			ftop *= kInchUnits;
			fright *= kInchUnits;
			fleft *= kInchUnits;
			fbottom *= kInchUnits;
			break;
		case kUnitCM:
			// convert to points
			ftop *= kCMUnits;
			fright *= kCMUnits;
			fleft *= kCMUnits;
			fbottom *= kCMUnits;
			break;
		case kUnitPoint:
			break;
	}
	
	margin.Set(fleft, ftop, fright, fbottom);
	
	return margin;
}

/**
 * MesssageReceived()
 *
 * Receive messages for the view 
 *
 * @param BMessage* , the message being received
 * @return void
 */
void 
MarginView::MessageReceived(BMessage *msg)
{
	switch (msg->what) 
	{	
		case CHANGE_PAGE_SIZE: {
				float w;
				float h;
				msg->FindFloat("width", &w);
				msg->FindFloat("height", &h);
				SetPageSize(w, h);
				UpdateView(MARGIN_CHANGED);
			}
			break;
	
		case FLIP_PAGE: {	
				BPoint p;
				p = GetPageSize();
				SetPageSize(p.y, p.x);
				UpdateView(MARGIN_CHANGED);
			}
			break;
			
		case MARGIN_CHANGED:
			UpdateView(MARGIN_CHANGED);
			break;
		
		case TOP_MARGIN_CHANGED:
			UpdateView(TOP_MARGIN_CHANGED);
			break;
		
		case LEFT_MARGIN_CHANGED:
			UpdateView(LEFT_MARGIN_CHANGED);
			break;
		
		case RIGHT_MARGIN_CHANGED:
			UpdateView(RIGHT_MARGIN_CHANGED);
			break;
		
		case BOTTOM_MARGIN_CHANGED:
			UpdateView(BOTTOM_MARGIN_CHANGED);
			break;
		
		case MARGIN_UNIT_CHANGED:
		 	{
		 		int32 marginUnit;
		 		if (msg->FindInt32("marginUnit", &marginUnit) == B_OK) {
					SetMarginUnit((MarginUnit)marginUnit);
				}
			}
			break;
			
		default:
			BView::MessageReceived(msg);
			break;
	}
}
/*----------------- MarginView Private Methods --------------------*/

/**
 * ConstructGUI()
 *
 * Creates the GUI for the View. MUST be called AFTER the View is attached to
 *	the Window, or will crash and/or create strange behaviour
 *
 * @param none
 * @return void
 */
void 
MarginView::ConstructGUI()
{
	BMessage *msg;
	BString str;
	BMenuItem *item;
	BMenuField *mf;
	BPopUpMenu *menu;

// Create text fields
	msg = new BMessage(MARGIN_CHANGED);
	BRect r(Frame().Width() - be_plain_font->StringWidth("Top#") - kWidth, 
			kOffsetY, Frame().Width() - kOffsetX, kWidth);
	
	// top	
	str << fMargins.top/fUnitValue;
	fTop = new BTextControl( r, "top", "Top:", str.String(), NULL,
				B_FOLLOW_RIGHT);
	
	fTop->SetModificationMessage(new BMessage(TOP_MARGIN_CHANGED));
	fTop->SetDivider(be_plain_font->StringWidth("Top#"));
	fTop->SetTarget(this);
	AllowOnlyNumbers(fTop, kNumCount);
	AddChild(fTop);



	
	//left
	r.OffsetBy(0, kOffsetY);
	r.left = Frame().Width() - be_plain_font->StringWidth("Left#") - kWidth;
    str = "";	
	str << fMargins.left/fUnitValue;
	fLeft = new BTextControl( r, "left", "Left:", str.String(), NULL,
				B_FOLLOW_RIGHT);	

	fLeft->SetModificationMessage(new BMessage(LEFT_MARGIN_CHANGED));
	fLeft->SetDivider(be_plain_font->StringWidth("Left#"));
	fLeft->SetTarget(this);
	AllowOnlyNumbers(fLeft, kNumCount);
	AddChild(fLeft);
	
	
	
	
	//bottom
	r.OffsetBy(0, kOffsetY);
	r.left = Frame().Width() - be_plain_font->StringWidth("Bottom#") - kWidth;
    str = "";	
	str << fMargins.bottom/fUnitValue;
	fBottom = new BTextControl( r, "bottom", "Bottom:", str.String(), NULL,
				B_FOLLOW_RIGHT);
	
	fBottom->SetModificationMessage(new BMessage(BOTTOM_MARGIN_CHANGED));
	fBottom->SetDivider(be_plain_font->StringWidth("Bottom#"));
	fBottom->SetTarget(this);
	
	AllowOnlyNumbers(fBottom, kNumCount);
	AddChild(fBottom);
	
	
	
	
	//right
	r.OffsetBy(0, kOffsetY);
	r.left = Frame().Width() - be_plain_font->StringWidth("Right#") - kWidth;
    str = "";	
	str << fMargins.right/fUnitValue;
	fRight = new BTextControl( r, "right", "Right:", str.String(), NULL,
				B_FOLLOW_RIGHT);
	
	fRight->SetModificationMessage(new BMessage(RIGHT_MARGIN_CHANGED));
	fRight->SetDivider(be_plain_font->StringWidth("Right#"));
	fRight->SetTarget(this);
	AllowOnlyNumbers(fRight, kNumCount);
	AddChild(fRight);



// Create Units popup
	r.OffsetBy(-kOffsetX, kOffsetY);
	r.right += kOffsetY;

	menu = new BPopUpMenu("units");
	mf = new BMenuField(r, "units", "Units", menu,
			B_FOLLOW_BOTTOM|B_FOLLOW_RIGHT|B_WILL_DRAW);
	mf->ResizeToPreferred();
	mf->SetDivider(be_plain_font->StringWidth("Units#"));
	
	// Construct menu items 
	for (int i=0; kUnitNames[i] != NULL; i++ ) 
	{
		msg = new BMessage(MARGIN_UNIT_CHANGED);
		msg->AddInt32("marginUnit", kUnitMsg[i]);
		menu->AddItem(item = new BMenuItem(kUnitNames[i], msg));
		item->SetTarget(this);
		if (fMarginUnit == kUnitMsg[i]) {
			item->SetMarked(true);
		}
	}
	AddChild(mf);

	// calculate the sizes for drawing page view
	CalculateViewSize(MARGIN_CHANGED);
}


/**
 * AllowOnlyNumbers()
 *
 * @param BTextControl, the control we want to only allow numbers
 * @param maxNum, the maximun number of characters allowed
 * @return void
 */
void 
MarginView::AllowOnlyNumbers(BTextControl *textControl, int maxNum)
{
	BTextView *tv = textControl->TextView();

	for (long i = 0; i < 256; i++) {
		tv->DisallowChar(i); 
	}
	for (long i = '0'; i <= '9'; i++) {
		tv->AllowChar(i); 
	}
	tv->AllowChar(B_BACKSPACE);
	tv->AllowChar('.');
	tv->SetMaxBytes(maxNum);
}

/**
 * SetMargin
 *
 * @param brect, margin values in rect
 * @return void
 */
void  
MarginView::SetMargin(BRect margin) {
	fMargins = margin;
}

/**
 * SetUnits, called by the MarginMgr when the units popup is selected
 *
 * @param uint32, the enum that identifies the units requested to change to.
 * @return void
 */
void 
MarginView::SetMarginUnit(MarginUnit unit)
{
	// do nothing if the current units are the same as requested
	if (unit == fMarginUnit) {
		return;
	}
	
	// set the units Format
	fUnitValue = kUnitFormat[unit];

	// convert the field text to values
	float ftop 		= atof(fTop->Text());
	float fright 	= atof(fRight->Text());
	float fleft 	= atof(fLeft->Text());
	float fbottom 	= atof(fBottom->Text());

	// convert to target units
	switch (fMarginUnit)  
	{	
		case kUnitInch:
			// convert to points
			ftop *= kInchUnits;
			fright *= kInchUnits;
			fleft *= kInchUnits;
			fbottom *= kInchUnits;
			// check for target unit is cm
			if (unit == kUnitCM) {
				ftop /= kCMUnits;
				fright /= kCMUnits;
				fleft /= kCMUnits;
				fbottom /= kCMUnits;
			}
			break;
		case kUnitCM:
			// convert to points
			ftop *= kCMUnits;
			fright *= kCMUnits;
			fleft *= kCMUnits;
			fbottom *= kCMUnits;
			// check for target unit is inches
			if (unit == kUnitInch) {
				ftop /= kInchUnits;
				fright /= kInchUnits;
				fleft /= kInchUnits;
				fbottom /= kInchUnits;
			}
			break;
		case kUnitPoint:
			// check for target unit is cm
			if (unit == kUnitCM) {
				ftop /= kCMUnits;
				fright /= kCMUnits;
				fleft /= kCMUnits;
				fbottom /= kCMUnits;
			}
			// check for target unit is inches
			if (unit == kUnitInch) {
				ftop /= kInchUnits;
				fright /= kInchUnits;
				fleft /= kInchUnits;
				fbottom /= kInchUnits;
			}
			break;
	}
	fMarginUnit = unit;
	
	// lock Window since these changes are from another thread
	Window()->Lock();
	
	// set the fields to new units
	BString str;
	str << ftop; 
	fTop->SetText(str.String());
	
	str = "";	
	str << fleft; 
	fLeft->SetText(str.String());
	
	str = "";	
	str << fright; 
	fRight->SetText(str.String());
	
	str = "";	
	str << fbottom; 
	fBottom->SetText(str.String());

	// update UI
	CalculateViewSize(MARGIN_CHANGED);
	Invalidate();
	
	Window()->Unlock();
}

/**
 * CalculateViewSize
 *
 * calculate the size of the view that is used
 *	to show the page inside the margin box. This is dependent 
 *	on the size of the box and the room we have to show it and
 *	the units that we are using and the orientation of the page.
 *	
 * @param msg, the message for which field changed to check value bounds 
 * @return void
 */
void 
MarginView::CalculateViewSize(uint32 msg)
{
	// determine page orientation 	
	if (fPageHeight < fPageWidth) { // LANDSCAPE
		fViewWidth = fMaxPageWidth;
		fViewHeight = fPageHeight * (fViewWidth/fPageWidth);
		float hdiff = fViewHeight - fMaxPageHeight;
		if (hdiff > 0) {
			fViewHeight -= hdiff;
			fViewWidth -= hdiff;
		}
	} else { // PORTRAIT
		fViewHeight = fMaxPageHeight;
		fViewWidth = fPageWidth * (fViewHeight/fPageHeight);
		float wdiff = fViewWidth - fMaxPageWidth;
		if (wdiff > 0) {
			fViewHeight -= wdiff;
			fViewWidth -= wdiff;
		}
	}

	// calculate margins based on view size
	
	// find the length of 1 pixel in points
	// 	ex: 80px/800pt = 0.1px/pt 
	float pixelLength = fViewHeight/fPageHeight;
	
	// convert the margins to points
	// The text field will have a number that us in the current unit
	//  ex 0.2" * 72pt = 14.4pts
	float ftop = atof(fTop->Text()) * fUnitValue;
	float fright = atof(fRight->Text()) * fUnitValue;
	float fbottom = atof(fBottom->Text()) * fUnitValue;
	float fleft = atof(fLeft->Text()) * fUnitValue;

	// Check that the margins don't overlap each other...
 	float delta = 72.0;	
 		// minimum printable rect = 1 inch * 1 inch
	float ph = fPageHeight-delta;
	float pw = fPageWidth-delta;
 	BString str;

	//  Bounds calculation rules: 	
	if (msg == TOP_MARGIN_CHANGED) 
	{
		//	top must be <= bottom
		if (ftop > (ph - fbottom)) {
			ftop = ph - fbottom;
			str = "";	
			str << ftop / fUnitValue;
			Window()->Lock();
			fTop->SetText(str.String());
			Window()->Unlock();	
		}

	}

	if (msg == BOTTOM_MARGIN_CHANGED) 
	{
		//	bottom must be <= pageHeight 
		if (fbottom > (ph - ftop)) {
			fbottom = ph - ftop;
			str = "";	
			str << fbottom / fUnitValue;
			Window()->Lock();
			fBottom->SetText(str.String());
			Window()->Unlock();	
		}
	}
	
	if (msg == LEFT_MARGIN_CHANGED) 
	{
		// 	left must be <= right  
		if (fleft > (pw - fright)) {
			fleft = pw - fright;
			str = "";	
			str << fleft / fUnitValue;
			Window()->Lock();
			fLeft->SetText(str.String());
			Window()->Unlock();	
		}
	}
	
	if (msg == RIGHT_MARGIN_CHANGED) 
	{
		//	right must be <= fPageWidth 
		if (fright > (pw - fleft)) {
			fright = pw - fleft;
			str = "";	
			str << fright / fUnitValue;
			Window()->Lock();
			fRight->SetText(str.String());
			Window()->Unlock();	
		}
	}
	
	// convert the unit value to pixels
	//  ex: 14.4pt * 0.1px/pt = 1.44px
	fMargins.top = ftop * pixelLength;
	fMargins.right = fright * pixelLength;
	fMargins.bottom = fbottom * pixelLength;
	fMargins.left = fleft * pixelLength;
}


