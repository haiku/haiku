/*

MarginView.cpp

Copyright (c) 2002 Haiku.

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
#include <GridView.h>
#include <GridLayout.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <SupportKit.h>
#include <TextControl.h>


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
const static char *kUnitNames[] = { "inch", "cm", "points", NULL };
const static MarginUnit kUnitMsg[] = { kUnitInch, kUnitCM, kUnitPoint };

const pattern kDots = {{ 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55 }};

const rgb_color kBlack 	= { 0,0,0,0 };
const rgb_color kRed 	= { 255,0,0,0 };
const rgb_color kWhite 	= { 255,255,255,0 };
const rgb_color kGray 	= { 220,220,220,0 };


PageView::PageView()
: BView("pageView", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
, fPageWidth(0)
, fPageHeight(0)
, fMargins(0, 0, 0, 0)
{

}


void
PageView::SetPageSize(float pageWidth, float pageHeight)
{
	fPageWidth = pageWidth;
	fPageHeight = pageHeight;
}


void
PageView::SetMargins(BRect margins)
{
	fMargins = margins;
}


void
PageView::Draw(BRect bounds)
{
	BRect frame(Frame());
	float totalWidth = frame.Width();
	float totalHeight = frame.Height();

	// fit page into available space
	// keeping the ratio fPageWidth : fPageHeight
	float pageWidth = totalWidth;
	float pageHeight = totalWidth * fPageHeight / fPageWidth;
	if (pageHeight > totalHeight) {
		pageHeight = totalHeight;
		pageWidth = totalHeight * fPageWidth / fPageHeight;
	}

	// center page
	BPoint offset(0, 0);
	offset.x = static_cast<int>((totalWidth - pageWidth) / 2);
	offset.y = static_cast<int>((totalHeight - pageHeight) / 2);

	// draw the page
	SetHighColor(kWhite);
	BRect r = BRect(0, 0, pageWidth, pageHeight);
	r.OffsetBy(offset);
	FillRect(r);
	SetHighColor(kBlack);
	StrokeRect(r);

	// draw margin
	SetHighColor(kRed);
	SetLowColor(kWhite);
	r.top += (fMargins.top / fPageHeight) * pageHeight;
	r.right -= (fMargins.right / fPageWidth) * pageWidth;
	r.bottom -= (fMargins.bottom / fPageHeight) * pageHeight;
	r.left += (fMargins.left / fPageWidth) * pageWidth;
	StrokeRect(r, kDots);
}


/**
 * Constructor
 *
 * @param pageWidth, float that is the points value of the page width
 * @param pageHeight, float that is the points value of the page height
 * @param margins, BRect values of margins
 * @param units, unit32 enum for units used in view
 * @return void
 */
MarginView::MarginView(int32 pageWidth, int32 pageHeight,
	BRect margins, MarginUnit units)
	: BBox("marginView")
{
	fMarginUnit = units;
	fUnitValue = kUnitFormat[units];

	SetLabel("Margins");

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
MarginView::~MarginView()
{
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
	if (Parent())
		SetViewColor(Parent()->ViewColor());

	_ConstructGUI();
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
	switch (msg->what) {
		case CHANGE_PAGE_SIZE: {
				float w;
				float h;
				msg->FindFloat("width", &w);
				msg->FindFloat("height", &h);
				SetPageSize(w, h);
				UpdateView(MARGIN_CHANGED);
			}	break;

		case FLIP_PAGE: {
				BPoint p = PageSize();
				SetPageSize(p.y, p.x);
				UpdateView(MARGIN_CHANGED);
			}	break;

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

		case MARGIN_UNIT_CHANGED: {
		 		int32 marginUnit;
		 		if (msg->FindInt32("marginUnit", &marginUnit) == B_OK)
					_SetMarginUnit((MarginUnit)marginUnit);
			}	break;

		default:
			BView::MessageReceived(msg);
			break;
	}
}


/*----------------- MarginView Public Methods --------------------*/

/**
 * PageSize
 *
 * @param none
 * @return BPoint, contains actual point values of page in x, y of point
 */
BPoint
MarginView::PageSize() const
{
	return BPoint(fPageWidth, fPageHeight);
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
 * Margin
 *
 * @param none
 * @return rect, return margin values always in points
 */
BRect
MarginView::Margin() const
{
	BRect margin;

	// convert the field text to values
	float top = atof(fTop->Text());
	float right = atof(fRight->Text());
	float left = atof(fLeft->Text());
	float bottom = atof(fBottom->Text());

	// convert to units to points
	switch (fMarginUnit) {
		case kUnitInch:
			// convert to points
			top *= kInchUnits;
			right *= kInchUnits;
			left *= kInchUnits;
			bottom *= kInchUnits;
			break;
		case kUnitCM:
			// convert to points
			top *= kCMUnits;
			right *= kCMUnits;
			left *= kCMUnits;
			bottom *= kCMUnits;
			break;
		case kUnitPoint:
			break;
	}

	margin.Set(left, top, right, bottom);

	return margin;
}



/**
 * Unit
 *
 * @param none
 * @return uint32 enum, units in inches, cm, points
 */
MarginUnit
MarginView::Unit() const
{
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

	{
		char pageSize[kStringSize];
		sprintf(pageSize, "%2.1f x %2.1f",
			fPageWidth / fUnitValue,
			fPageHeight / fUnitValue);
		fPageSize->SetText(pageSize);

		fPage->SetPageSize(fPageWidth, fPageHeight);
		fPage->SetMargins(Margin());
		fPage->Invalidate();
	}

	Invalidate();
	Window()->Unlock();
}


/*----------------- MarginView Private Methods --------------------*/

/**
 * _ConstructGUI()
 *
 * Creates the GUI for the View. MUST be called AFTER the View is attached to
 *	the Window, or will crash and/or create strange behaviour
 *
 * @param none
 * @return void
 */
void
MarginView::_ConstructGUI()
{
	fPage = new PageView();
	fPage->SetViewColor(ViewColor());

	fPageSize = new BStringView("pageSize", "?x?");

	BString str;
	// Create text fields

	// top
	str << fMargins.top/fUnitValue;
	fTop = new BTextControl("top", "Top:", str.String(), NULL);

	fTop->SetModificationMessage(new BMessage(TOP_MARGIN_CHANGED));
	fTop->SetTarget(this);
	_AllowOnlyNumbers(fTop, kNumCount);

	//left
    str = "";
	str << fMargins.left/fUnitValue;
	fLeft = new BTextControl("left", "Left:", str.String(), NULL);

	fLeft->SetModificationMessage(new BMessage(LEFT_MARGIN_CHANGED));
	fLeft->SetTarget(this);
	_AllowOnlyNumbers(fLeft, kNumCount);

	//bottom
    str = "";
	str << fMargins.bottom/fUnitValue;
	fBottom = new BTextControl("bottom", "Bottom:", str.String(), NULL);

	fBottom->SetModificationMessage(new BMessage(BOTTOM_MARGIN_CHANGED));
	fBottom->SetTarget(this);
	_AllowOnlyNumbers(fBottom, kNumCount);

	//right
    str = "";
	str << fMargins.right/fUnitValue;
	fRight = new BTextControl("right", "Right:", str.String(), NULL);

	fRight->SetModificationMessage(new BMessage(RIGHT_MARGIN_CHANGED));
	fRight->SetTarget(this);
	_AllowOnlyNumbers(fRight, kNumCount);

	// Create Units popup

	BPopUpMenu *menu = new BPopUpMenu("units");
	BMenuField *units = new BMenuField("units", "Units:", menu);

	BMenuItem *item;
	// Construct menu items
	for (int32 i = 0; kUnitNames[i] != NULL; i++) {
		BMessage *msg = new BMessage(MARGIN_UNIT_CHANGED);
		msg->AddInt32("marginUnit", kUnitMsg[i]);
		menu->AddItem(item = new BMenuItem(kUnitNames[i], msg));
		item->SetTarget(this);
		if (fMarginUnit == kUnitMsg[i])
			item->SetMarked(true);
	}

	BGridView* settings = new BGridView();
	BGridLayout* settingsLayout = settings->GridLayout();
	settingsLayout->AddItem(fTop->CreateLabelLayoutItem(), 0, 0);
	settingsLayout->AddItem(fTop->CreateTextViewLayoutItem(), 1, 0);
	settingsLayout->AddItem(fLeft->CreateLabelLayoutItem(), 0, 1);
	settingsLayout->AddItem(fLeft->CreateTextViewLayoutItem(), 1, 1);
	settingsLayout->AddItem(fBottom->CreateLabelLayoutItem(), 0, 2);
	settingsLayout->AddItem(fBottom->CreateTextViewLayoutItem(), 1, 2);
	settingsLayout->AddItem(fRight->CreateLabelLayoutItem(), 0, 3);
	settingsLayout->AddItem(fRight->CreateTextViewLayoutItem(), 1, 3);
	settingsLayout->AddItem(units->CreateLabelLayoutItem(), 0, 4);
	settingsLayout->AddItem(units->CreateMenuBarLayoutItem(), 1, 4);
	settingsLayout->SetSpacing(0, 0);

	BGroupView* groupView = new BGroupView(B_HORIZONTAL, 10);
	BGroupLayout* groupLayout = groupView->GroupLayout();
	groupLayout->AddView(BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(fPage)
		.Add(fPageSize)
		.SetInsets(0, 0, 0, 0)
		.TopView()
	);
	groupLayout->AddView(settings);
	groupLayout->SetInsets(5, 5, 5, 5);

	AddChild(groupView);

	UpdateView(MARGIN_CHANGED);
}


/**
 * AllowOnlyNumbers()
 *
 * @param BTextControl, the control we want to only allow numbers
 * @param maxNum, the maximun number of characters allowed
 * @return void
 */
void
MarginView::_AllowOnlyNumbers(BTextControl *textControl, int32 maxNum)
{
	BTextView *tv = textControl->TextView();

	for (int32 i = 0; i < 256; i++)
		tv->DisallowChar(i);

	for (int32 i = '0'; i <= '9'; i++)
		tv->AllowChar(i);

	tv->AllowChar(B_BACKSPACE);
	// TODO internationalization; e.g. "." or ","
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
MarginView::_SetMargin(BRect margin)
{
	fMargins = margin;
}


/**
 * SetUnits, called by the MarginMgr when the units popup is selected
 *
 * @param uint32, the enum that identifies the units requested to change to.
 * @return void
 */
void
MarginView::_SetMarginUnit(MarginUnit unit)
{
	// do nothing if the current units are the same as requested
	if (unit == fMarginUnit) {
		return;
	}

	// set the units Format
	fUnitValue = kUnitFormat[unit];

	// convert the field text to values
	float top = atof(fTop->Text());
	float right = atof(fRight->Text());
	float left = atof(fLeft->Text());
	float bottom = atof(fBottom->Text());

	// convert to target units
	switch (fMarginUnit)
	{
		case kUnitInch:
			// convert to points
			top *= kInchUnits;
			right *= kInchUnits;
			left *= kInchUnits;
			bottom *= kInchUnits;
			// check for target unit is cm
			if (unit == kUnitCM) {
				top /= kCMUnits;
				right /= kCMUnits;
				left /= kCMUnits;
				bottom /= kCMUnits;
			}
			break;
		case kUnitCM:
			// convert to points
			top *= kCMUnits;
			right *= kCMUnits;
			left *= kCMUnits;
			bottom *= kCMUnits;
			// check for target unit is inches
			if (unit == kUnitInch) {
				top /= kInchUnits;
				right /= kInchUnits;
				left /= kInchUnits;
				bottom /= kInchUnits;
			}
			break;
		case kUnitPoint:
			// check for target unit is cm
			if (unit == kUnitCM) {
				top /= kCMUnits;
				right /= kCMUnits;
				left /= kCMUnits;
				bottom /= kCMUnits;
			}
			// check for target unit is inches
			if (unit == kUnitInch) {
				top /= kInchUnits;
				right /= kInchUnits;
				left /= kInchUnits;
				bottom /= kInchUnits;
			}
			break;
	}
	fMarginUnit = unit;

	// lock Window since these changes are from another thread
	Window()->Lock();

	// set the fields to new units
	BString str;
	str << top;
	fTop->SetText(str.String());

	str = "";
	str << left;
	fLeft->SetText(str.String());

	str = "";
	str << right;
	fRight->SetText(str.String());

	str = "";
	str << bottom;
	fBottom->SetText(str.String());

	// update UI
	Invalidate();

	Window()->Unlock();
}

