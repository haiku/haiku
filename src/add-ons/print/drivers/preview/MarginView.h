/*

MarginView.h

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

	Documentation:
	
	The MarginView is designed to be a self contained component that manages
	the display of a BBox control that shows a graphic of a page and its'
	margings. The component also includes text fields that are used to mofify
	the margin values and a popup to change the units used for the margins.

	There are two interfaces for the MarginView component:
	
	1) Set methods:
		- page size
		- orientation
	   
	   Get methods to retrieve:
		- margins
		- page size
		
		The method interface is available for the parent Component to call on 
		the MarginView in response to the Window receiveing messages from
		other BControls that it contains, such as a Page Size popup. The
		Get methods are used to extract the page size and margins so that 
		the printer driver may put these values into a BMessage for printing.
		
	2) 'Optional' Message interface:
		- Set Page Size
		- Flip Orientation
	
		The message interface is available for GUI Controls, BPopupMenu to send 
		messages to the MarginView if the parent Window is not used to handle
		the messages. 

	General Use of MarginView component:
	
		1) Simply construct a new MarginView object with the margins 
			you want as defaults and add this view to the parent view 
			of the dialog.

			MarginView *mv;
			mv = new MarginView(viewSizeRect, pageWidth, pageHeight);
			parentView->AddChild(mv);

			* you can also set the margins in the constructor, and the units:
			
			mv = new MarginView(viewSizeRect, pageWidth, pageHeight
						marginRect, UNIT_POINTS);

			! but remeber to have the marginRect values match the UNITS :-)
			
		2) Set Page Size with methods:

			mv-SetPageSize( pageWidth, pageHeight );
			mv->UpdateView();
		
		3) Set Page Size with BMessage:

			BMessage* msg = new BMessage(CHANGE_PAGE_SIZE);
			msg->AddFloat("width", pageWidth);
			msg->AddFloat("height", pageHeight);
			mv->PostMessage(msg);

		4) Flip Page with methods:
		
			mv-SetPageSize( pageHeight, pageWidth );
			mv->UpdateView();
		
		5) Flip Page with BMessage: 
		
			BMessage* msg = new BMessage(FLIP_PAGE);
			mv->Looper()->PostMessage(msg);

		Note: the MarginView DOES NOT keep track of the orientation. This
				should be done by the code for the Page setup dialog.

		6) Get Page Size

			BPoint pageSize = mv->GetPageSize();
			
		7) Get Margins

			BRect margins = mv->GetMargins();
		
		8) Get Units
			
			uint32 units = mv->GetUnits();

			where units is one of: 
				UNIT_INCH,  72 points/in
				UNIT_CM,    28.346 points/cm
				UNIT_POINT, 1 point/point
*/

#ifndef MARGIN_VIEW_H
#define MARGIN_VIEW_H

#include <InterfaceKit.h>
#include <Looper.h>

class MarginManager;

// Messages that the MarginManager accepts
const uint32 TOP_MARGIN_CHANGED    = 'tchg';
const uint32 RIGHT_MARGIN_CHANGED  = 'rchg';
const uint32 LEFT_MARGIN_CHANGED   = 'lchg';
const uint32 BOTTOM_MARGIN_CHANGED = 'bchg';
const uint32 MARGIN_CHANGED        = 'mchg';
const uint32 CHANGE_PAGE_SIZE      = 'chps';
const uint32 FLIP_PAGE             = 'flip';

/**
 * Class MarginView
 */
class MarginView : public BBox 
{
friend MarginManager;

public:
	// used to index unitFormat array
	typedef enum {
		UNIT_INCH = 0,
		UNIT_CM,
		UNIT_POINT
	};

private:

	// GUI components
	BTextControl *fTop, *fBottom, *fLeft, *fRight;

	// rect that holds the margins for the page as a set of point offsets
	BRect fMargins;

	// the maximum size of the page view calculated from the view size
	float fMaxPageWidth;
	float fMaxPageHeight;

	// the actual size of the page in points
	float fPageHeight;
	float fPageWidth;

	// the units used to calculate the page size
	uint32 fUnits;
	float fUnitValue;

	// the size of the drawing area we have to draw the view in pixels
	float fViewHeight;
	float fViewWidth;

	// Calculate the view size for the margins
	void CalculateViewSize(uint32 msg);

	// performed internally using the supplied popup
	void  SetUnits(uint32 unit);

	// performed internally using text fields
	void  SetMargin(BRect margin);

	// utility method
	void AllowOnlyNumbers(BTextControl *textControl, int maxNum);
	
public:
	MarginView(BRect rect,
		int32 pageWidth = 0,
		int32 pageHeight = 0,
		BRect margins = BRect(1, 1, 1, 1), // default to 1 inch
		uint32 units = UNIT_INCH);

	~MarginView();

	/// all the GUI construction code
	void ConstructGUI();
	
	// page size
	void SetPageSize(float pageWidth, float pageHeight);
	// point.x = width, point.y = height
	BPoint GetPageSize(void);

	// margin
	BRect GetMargin(void);

	// orientation
	// None, this state should be saved elsewhere in the page setup code
	//	and not here. See the FLIP_PAGE message to perform this function.
		
	// units
	uint32 GetUnits(void);

	// will cause a recalc and redraw
	void UpdateView(uint32 msg);

	// BeOS Hook methods
	virtual void AttachedToWindow(void);
	void Draw(BRect rect);
	void FrameResized(float width, float height);
	void MessageReceived(BMessage *msg);
};

#endif //MARGIN_VIEW_H
