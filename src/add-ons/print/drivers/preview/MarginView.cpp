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

#ifndef MARGIN_VIEW_H
#include "MarginView.h"
#endif

#include <AppKit.h>
#include <SupportKit.h>

#include <stdio.h>
#include <stdlib.h>

/*----------------- MarginView Private Constants --------------------*/

const int Y_OFFSET = 20;
const int X_OFFSET = 10;
const int STRING_SIZE = 50;
const int _WIDTH = 50;
const int NUM_COUNT = 10;

const static float _pointUnits = 1; // 1 point = 1 point 
const static float _inchUnits = 72; // 1" = 72 points
const static float _cmUnits = 28.346; // 72/2.54 1cm = 28.346 points

const static float _minFieldWidth = 100; // pixels
const static float _minUnitHeight = 30; // pixels
const static float _drawInset = 10; // pixels
	
const static float unitFormat[] = { _inchUnits, _cmUnits, _pointUnits }; 
const static char *unitNames[] = { "Inch", "cm", "Points", NULL };
const static uint32 unitMsg[] = { MarginView::UNIT_INCH, 
								  MarginView::UNIT_CM, 
								  MarginView::UNIT_POINT };

const pattern dots = {{ 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55 }};

const rgb_color black 	= { 0,0,0,0 };
const rgb_color red 	= { 255,0,0,0 };
const rgb_color white 	= { 255,255,255,0 };
const rgb_color gray 	= { 220,220,220,0 };

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
		uint32 units)
		
	   	:BBox(frame, NULL, B_FOLLOW_ALL)
{
	fUnits = units;
	fUnitValue = unitFormat[units];
	
	SetLabel("Margins");
	
	fMaxPageHeight = frame.Height() - _minUnitHeight - Y_OFFSET;
	fMaxPageWidth = frame.Width() - _minFieldWidth - X_OFFSET;

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
void MarginView::Draw(BRect rect)
{
	BBox::Draw(rect);

	float y_offset = (float)Y_OFFSET;
	float x_offset = (float)X_OFFSET;
	BRect r;

	// Calculate offsets depending on orientation
	if (fPageWidth < fPageHeight) { // Portrait
		x_offset = (fMaxPageWidth/2 + X_OFFSET) - fViewWidth/2;
	} else { // landscape
		y_offset = (fMaxPageHeight/2 + Y_OFFSET) - fViewHeight/2;
	}
	
	// draw the page
	SetHighColor(white);
	r = BRect(0, 0, fViewWidth, fViewHeight);
	r.OffsetBy(x_offset, y_offset);
	FillRect(r);
	SetHighColor(black);
	StrokeRect(r);

	// draw margin
	SetHighColor(red);
	SetLowColor(white);
	r.top += fMargins.top;
	r.right -= fMargins.right;
	r.bottom -= fMargins.bottom;
	r.left += fMargins.left;
	StrokeRect(r, dots);

	// draw the page size label
	SetHighColor(black);
	SetLowColor(gray);
	char str[STRING_SIZE];
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
void MarginView::FrameResized(float width, float height)
{
	fMaxPageHeight = height - _minUnitHeight - X_OFFSET;
	fMaxPageWidth = width - _minFieldWidth - Y_OFFSET;

	CalculateViewSize(MARGIN_CHANGED);
	Invalidate();
}

/**
 * AttachToWindow
 *
 * @param none
 * @return void
 */
void MarginView::AttachedToWindow()
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
uint32 MarginView::GetUnits(void) {
	return fUnits;
}

/**
 * UpdateView, recalculate and redraw the view
 *
 * @param msg is a message to the calculate size to tell which field caused 
 *		the update to occur, or it is a general update.
 * @return void
 */
void MarginView::UpdateView(uint32 msg)
{
	Window()->Lock();
	CalculateViewSize(msg);
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
void MarginView::SetPageSize(float pageWidth, float pageHeight)
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
BPoint MarginView::GetPageSize(void) {
	return BPoint(fPageWidth, fPageHeight);
}

/**
 * GetMargin
 *
 * @param none
 * @return rect, return margin values always in points
 */
BRect MarginView::GetMargin(void) 
{
	BRect margin;

	// convert the field text to values
	float ftop 		= atof(fTop->Text());
	float fright 	= atof(fRight->Text());
	float fleft 	= atof(fLeft->Text());
	float fbottom 	= atof(fBottom->Text());

	// convert to units to points
	switch (fUnits)  
	{	
		case UNIT_INCH:
			// convert to points
			ftop *= _inchUnits;
			fright *= _inchUnits;
			fleft *= _inchUnits;
			fbottom *= _inchUnits;
			break;
		case UNIT_CM:
			// convert to points
			ftop *= _cmUnits;
			fright *= _cmUnits;
			fleft *= _cmUnits;
			fbottom *= _cmUnits;
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
void MarginView::MessageReceived(BMessage *msg)
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
		
		case UNIT_INCH:
		case UNIT_CM:
		case UNIT_POINT:
			SetUnits(msg->what);
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
void MarginView::ConstructGUI()
{
	BMessage *msg;
	BString str;
	BMenuItem *item;
	BMenuField *mf;
	BPopUpMenu *menu;

// Create text fields
	msg = new BMessage(MARGIN_CHANGED);
	BRect r(Frame().Width() - be_plain_font->StringWidth("Top#") - _WIDTH, 
			Y_OFFSET, Frame().Width() - X_OFFSET, _WIDTH);
	
	// top	
	msg = new BMessage(TOP_MARGIN_CHANGED);
	str << fMargins.top/fUnitValue;
	fTop = new BTextControl( r, "top", "Top", str.String(), msg,
				B_FOLLOW_RIGHT);
	fTop->SetDivider(be_plain_font->StringWidth("Top#"));
	fTop->SetTarget(this);
	AllowOnlyNumbers(fTop, NUM_COUNT);
	AddChild(fTop);
	
	//left
	r.OffsetBy(0, Y_OFFSET);
	r.left = Frame().Width() - be_plain_font->StringWidth("Left#") - _WIDTH;
    str = "";	
	str << fMargins.left/fUnitValue;
	msg = new BMessage(LEFT_MARGIN_CHANGED);
	fLeft = new BTextControl( r, "left", "Left", str.String(), msg,
				B_FOLLOW_RIGHT);	
	fLeft->SetDivider(be_plain_font->StringWidth("Left#"));
	fLeft->SetTarget(this);
	AllowOnlyNumbers(fLeft, NUM_COUNT);
	AddChild(fLeft);
	
	//bottom
	r.OffsetBy(0, Y_OFFSET);
	r.left = Frame().Width() - be_plain_font->StringWidth("Bottom#") - _WIDTH;
    str = "";	
	str << fMargins.bottom/fUnitValue;
	msg = new BMessage(BOTTOM_MARGIN_CHANGED);
	fBottom = new BTextControl( r, "bottom", "Bottom", str.String(), msg,
				B_FOLLOW_RIGHT);
	fBottom->SetDivider(be_plain_font->StringWidth("Bottom#"));
	fBottom->SetTarget(this);
	
	AllowOnlyNumbers(fBottom, NUM_COUNT);
	AddChild(fBottom);
	
	//right
	r.OffsetBy(0, Y_OFFSET);
	r.left = Frame().Width() - be_plain_font->StringWidth("Right#") - _WIDTH;
    str = "";	
	str << fMargins.right/fUnitValue;
	msg = new BMessage(RIGHT_MARGIN_CHANGED);
	fRight = new BTextControl( r, "right", "Right", str.String(), msg,
				B_FOLLOW_RIGHT);
	fRight->SetDivider(be_plain_font->StringWidth("Right#"));
	fRight->SetTarget(this);
	AllowOnlyNumbers(fRight, NUM_COUNT);
	AddChild(fRight);

// Create Units popup
	r.OffsetBy(-X_OFFSET,Y_OFFSET);
	r.right += Y_OFFSET;

	menu = new BPopUpMenu("units");
	mf = new BMenuField(r, "units", "Units", menu,
			B_FOLLOW_BOTTOM|B_FOLLOW_RIGHT|B_WILL_DRAW);
	mf->ResizeToPreferred();
	mf->SetDivider(be_plain_font->StringWidth("Units#"));
	
	// Construct menu items 
	for (int i=0; unitNames[i] != NULL; i++ ) 
	{
		msg = new BMessage(unitMsg[i]);
		menu->AddItem(item = new BMenuItem(unitNames[i], msg));
		item->SetTarget(this);
		if (fUnits == unitMsg[i]) {
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
void MarginView::AllowOnlyNumbers(BTextControl *textControl, int maxNum)
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
void  MarginView::SetMargin(BRect margin) {
	fMargins = margin;
}

/**
 * SetUnits, called by the MarginMgr when the units popup is selected
 *
 * @param uint32, the enum that identifies the units requested to change to.
 * @return void
 */
void MarginView::SetUnits(uint32 unit)
{
	// do nothing if the current units are the same as requested
	if (unit == fUnits) {
		return;
	}
	
	// set the units Format
	fUnitValue = unitFormat[unit];

	// convert the field text to values
	float ftop 		= atof(fTop->Text());
	float fright 	= atof(fRight->Text());
	float fleft 	= atof(fLeft->Text());
	float fbottom 	= atof(fBottom->Text());

	// convert to target units
	switch (fUnits)  
	{	
		case UNIT_INCH:
			// convert to points
			ftop *= _inchUnits;
			fright *= _inchUnits;
			fleft *= _inchUnits;
			fbottom *= _inchUnits;
			// check for target unit is cm
			if (unit == UNIT_CM) {
				ftop /= _cmUnits;
				fright /= _cmUnits;
				fleft /= _cmUnits;
				fbottom /= _cmUnits;
			}
			break;
		case UNIT_CM:
			// convert to points
			ftop *= _cmUnits;
			fright *= _cmUnits;
			fleft *= _cmUnits;
			fbottom *= _cmUnits;
			// check for target unit is inches
			if (unit == UNIT_INCH) {
				ftop /= _inchUnits;
				fright /= _inchUnits;
				fleft /= _inchUnits;
				fbottom /= _inchUnits;
			}
			break;
		case UNIT_POINT:
			// check for target unit is cm
			if (unit == UNIT_CM) {
				ftop /= _cmUnits;
				fright /= _cmUnits;
				fleft /= _cmUnits;
				fbottom /= _cmUnits;
			}
			// check for target unit is inches
			if (unit == UNIT_INCH) {
				ftop /= _inchUnits;
				fright /= _inchUnits;
				fleft /= _inchUnits;
				fbottom /= _inchUnits;
			}
			break;
	}
	fUnits = unit;
	
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
void MarginView::CalculateViewSize(uint32 msg)
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
	float ph = fPageHeight;
	float pw = fPageWidth;
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


