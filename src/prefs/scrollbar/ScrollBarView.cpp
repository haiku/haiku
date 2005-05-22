//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
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
//	File Name:		ScrollBarView.cpp
//	Author:			MrSiggler
//					DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	view which does all the work
//  
//------------------------------------------------------------------------------
#include "ScrollBarApp.h"
#include "ScrollBarView.h"
#include "fakescrollbar.h"
#include "knobsizeadjuster.h"
#include "messages.h"

// This is R5's default value
#define DEFAULT_KNOB_SIZE 15

ScrollBarView::ScrollBarView(void)
 : BView(	BRect(0,0,348,280),"scrollbarview", B_FOLLOW_LEFT | B_FOLLOW_TOP, 
 			B_WILL_DRAW)
{
	SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR) );

	get_scroll_bar_info(&gRevertSettings);
	gSettings=gRevertSettings;
		
	// Set up the top left box - single vs. double arrow selection
	BBox *arrowstyleBox = new BBox( BRect(12, 7, 169, 119) );
	arrowstyleBox->SetLabel( "Arrow Style" );

	float doubleWidth = StringWidth("Double:");
	BStringView *doubleStringView = new BStringView( BRect(25,20,25+doubleWidth,38),
													 "doubleStringView","Double:");
	
	fDouble=new FakeScrollBar(  BPoint(12,38), gSettings.double_arrows, gSettings.knob,
								!gSettings.proportional, true, gSettings.min_knob_size,
								new BMessage(SCROLLBAR_ARROW_STYLE_DOUBLE));
	arrowstyleBox->AddChild( fDouble );

	float singleWidth = StringWidth("Single:");
	BStringView *singleStringView = new BStringView( BRect(25,60,25+singleWidth,87),
													 "singleStringView", "Single:");

	fSingle=new FakeScrollBar( BPoint(12,singleStringView->Frame().bottom+1),
								!gSettings.double_arrows, gSettings.knob,
								!gSettings.proportional, false, gSettings.min_knob_size,
								new BMessage(SCROLLBAR_ARROW_STYLE_SINGLE));
	arrowstyleBox->AddChild( fSingle );
	
	BBox *knobtypeBox = new BBox( BRect(180, 7, 338, 119) );
	knobtypeBox->SetLabel( "Knob Type" );

	float proportionalWidth = StringWidth("Proportional:");
	BStringView *proportionalStringView = new BStringView(	BRect(193,20,193+proportionalWidth,38),
															"proportionalStringView",
															"Proportional:" );
	
	fProportional=new FakeScrollBar( BPoint(12,38),gSettings.proportional, gSettings.knob,
								false, gSettings.double_arrows, gSettings.min_knob_size,
								new BMessage(SCROLLBAR_KNOB_TYPE_PROPORTIONAL));
	knobtypeBox->AddChild( fProportional );

	float fixedWidth = StringWidth("Fixed:");
	BStringView *fixedStringView = new BStringView(	BRect(193,60,193+fixedWidth,87), 
													"fixedStringView", "Fixed:" );
	
	fFixed=new FakeScrollBar(	BPoint(12,87),!gSettings.proportional, gSettings.knob,
								true, gSettings.double_arrows, gSettings.min_knob_size,
								new BMessage(SCROLLBAR_KNOB_TYPE_FIXED));
	knobtypeBox->AddChild( fFixed );
	
	BBox *minknobsizeBox = new BBox( BRect(12, 123, 169, 232) );
	minknobsizeBox->SetLabel( "Minimum Knob Size:" );
	
	fSizeAdjuster = new KnobSizeAdjuster( BPoint(12,38), gSettings.min_knob_size );
	minknobsizeBox->AddChild( fSizeAdjuster );
	
	BBox *knobstyleBox = new BBox( BRect(180, 123, 338, 232) );
	knobstyleBox->SetLabel( "Knob Style:" );

	fFlat=new FakeScrollBar(	BPoint(12,22),gSettings.knob==THUMB_BLANK, THUMB_BLANK,
								!gSettings.proportional, gSettings.double_arrows, 
								gSettings.min_knob_size,
								new BMessage(SCROLLBAR_KNOB_STYLE_FLAT));
	knobstyleBox->AddChild( fFlat );

	fDots=new FakeScrollBar(	BPoint(12,52),gSettings.knob==THUMB_DOTS, THUMB_DOTS,
								!gSettings.proportional, gSettings.double_arrows, 
								gSettings.min_knob_size,
								new BMessage(SCROLLBAR_KNOB_STYLE_DOT));
	knobstyleBox->AddChild( fDots );

	fLines=new FakeScrollBar(	BPoint(12,82),gSettings.knob==THUMB_LINES, THUMB_LINES,
								!gSettings.proportional, gSettings.double_arrows, 
								gSettings.min_knob_size,
								new BMessage(SCROLLBAR_KNOB_STYLE_LINE));
	knobstyleBox->AddChild( fLines );

	fDefaultsButton = new BButton(	BRect(10, 242, 85, 265), "defaultsbutton", 
									"Defaults", new BMessage(SCROLLBAR_DEFAULTS) );
	
	// Disable defaults button if the current settings are already the defaults
	if(	gSettings.knob==THUMB_DOTS && gSettings.proportional==true &&
		gSettings.double_arrows==true && gSettings.min_knob_size==DEFAULT_KNOB_SIZE	)
	{
		fDefaultsButton->SetEnabled(false);
	}
	
	fRevertButton = new BButton( BRect( 95, 242, 170, 265), "revertbutton", "Revert",
									new BMessage(SCROLLBAR_REVERT) );
	fRevertButton->SetEnabled( false );
	
	AddChild( arrowstyleBox );
	AddChild( doubleStringView );
	AddChild( singleStringView );
	
	AddChild( knobtypeBox );
	AddChild( proportionalStringView );
	AddChild( fixedStringView );
	
	AddChild( minknobsizeBox );
	AddChild( knobstyleBox );
	
	AddChild( fDefaultsButton );
	AddChild( fRevertButton );
}

ScrollBarView::~ScrollBarView(void)
{
}

void
ScrollBarView::AttachedToWindow(void)
{
	fSingle->SetTarget(this);
	fDouble->SetTarget(this);
	fProportional->SetTarget(this);
	fFixed->SetTarget(this);
	fFlat->SetTarget(this);
	fDots->SetTarget(this);
	fLines->SetTarget(this);
	
	fSizeAdjuster->SetTarget(this);
	
	fDefaultsButton->SetTarget(this);
	fRevertButton->SetTarget(this);
}

void
ScrollBarView::HandleButtonStates(void)
{
	if(	gSettings.knob!=gRevertSettings.knob ||
		gSettings.proportional!=gRevertSettings.proportional ||
		gSettings.double_arrows!=gRevertSettings.double_arrows ||
		gSettings.min_knob_size!=gRevertSettings.min_knob_size	)
	{
		fDefaultsButton->SetEnabled(false);
		fRevertButton->SetEnabled(true);
	}
	else
	{
		fDefaultsButton->SetEnabled(true);
		fRevertButton->SetEnabled(false);
	}
}

void
ScrollBarView::SetAllScrollBars(void)
{
	fSingle->SetFromScrollBarInfo(gSettings);
	fSingle->SetDoubleArrows(false);
	fDouble->SetFromScrollBarInfo(gSettings);
	fDouble->SetDoubleArrows(true);
	
	fProportional->SetFromScrollBarInfo(gSettings);
	fProportional->SetFixedThumb(false);
	fFixed->SetFromScrollBarInfo(gSettings);
	fFixed->SetFixedThumb(true);
	
	fFlat->SetFromScrollBarInfo(gSettings);
	fFlat->SetKnobStyle(THUMB_BLANK);
	fDots->SetFromScrollBarInfo(gSettings);
	fDots->SetKnobStyle(THUMB_DOTS);
	fLines->SetFromScrollBarInfo(gSettings);
	fLines->SetKnobStyle(THUMB_LINES);
	
	fSingle->SetSelected( !gSettings.double_arrows );
	fDouble->SetSelected( gSettings.double_arrows );
	
	fProportional->SetSelected( gSettings.proportional );
	fFixed->SetSelected( !gSettings.proportional );
	
	fFlat->SetSelected(gSettings.knob==THUMB_BLANK);
	fDots->SetSelected(gSettings.knob==THUMB_DOTS);
	fLines->SetSelected(gSettings.knob==THUMB_LINES);
}

void
ScrollBarView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case SCROLLBAR_DEFAULTS:
		{
			gSettings.double_arrows=true;
			gSettings.proportional=true;
			gSettings.knob=THUMB_DOTS;
			gSettings.min_knob_size=DEFAULT_KNOB_SIZE;
			
			SetAllScrollBars();
			HandleButtonStates();
			break;
		}
		case SCROLLBAR_REVERT:
		{
			gSettings=gRevertSettings;
			
			fSingle->SetFromScrollBarInfo(gSettings);
			fDouble->SetFromScrollBarInfo(gSettings);
			fProportional->SetFromScrollBarInfo(gSettings);
			fFixed->SetFromScrollBarInfo(gSettings);
			fFlat->SetFromScrollBarInfo(gSettings);
			fDots->SetFromScrollBarInfo(gSettings);
			fLines->SetFromScrollBarInfo(gSettings);
			
			SetAllScrollBars();
			HandleButtonStates();
			break;
		}
		
		case SCROLLBAR_ARROW_STYLE_DOUBLE:
		{
			fDouble->SetSelected(true);
			fSingle->SetSelected(false);
			
			fFixed->SetDoubleArrows(true);
			fProportional->SetDoubleArrows(true);
			
			fFlat->SetDoubleArrows(true);
			fDots->SetDoubleArrows(true);
			fLines->SetDoubleArrows(true);
			
			gSettings.double_arrows=true;
			HandleButtonStates();
			break;
		}
		case SCROLLBAR_ARROW_STYLE_SINGLE:
		{
			fDouble->SetSelected(false);
			fSingle->SetSelected(true);
			
			fFixed->SetDoubleArrows(false);
			fProportional->SetDoubleArrows(false);
			
			fFlat->SetDoubleArrows(false);
			fDots->SetDoubleArrows(false);
			fLines->SetDoubleArrows(false);
		
			gSettings.double_arrows=false;
			HandleButtonStates();
			break;
		}
		case SCROLLBAR_KNOB_TYPE_PROPORTIONAL:
		{
			fDouble->SetFixedThumb(false);
			fSingle->SetFixedThumb(false);
			
			fFixed->SetSelected(false);
			fProportional->SetSelected(true);
			
			fFlat->SetFixedThumb(false);
			fDots->SetFixedThumb(false);
			fLines->SetFixedThumb(false);
			
			gSettings.proportional=true;
			HandleButtonStates();
			break;
		}
		case SCROLLBAR_KNOB_TYPE_FIXED:
		{
			fDouble->SetFixedThumb(true);
			fSingle->SetFixedThumb(true);
			
			fFixed->SetSelected(true);
			fProportional->SetSelected(false);
			
			fFlat->SetFixedThumb(true);
			fDots->SetFixedThumb(true);
			fLines->SetFixedThumb(true);
			
			gSettings.proportional=false;
			HandleButtonStates();
			break;
		}
		
		case SCROLLBAR_KNOB_STYLE_FLAT:
		{
			fDouble->SetKnobStyle(THUMB_BLANK);
			fSingle->SetKnobStyle(THUMB_BLANK);
			
			fFixed->SetKnobStyle(THUMB_BLANK);
			fProportional->SetKnobStyle(THUMB_BLANK);
			
			fFlat->SetSelected(true);
			fDots->SetSelected(false);
			fLines->SetSelected(false);
			
			gSettings.knob=THUMB_BLANK;
			HandleButtonStates();
			break;
		}
		case SCROLLBAR_KNOB_STYLE_DOT:
		{
			fDouble->SetKnobStyle(THUMB_DOTS);
			fSingle->SetKnobStyle(THUMB_DOTS);
			
			fFixed->SetKnobStyle(THUMB_DOTS);
			fProportional->SetKnobStyle(THUMB_DOTS);
			
			fFlat->SetSelected(false);
			fDots->SetSelected(true);
			fLines->SetSelected(false);
			
			gSettings.knob=THUMB_DOTS;
			HandleButtonStates();
			break;
		}
		case SCROLLBAR_KNOB_STYLE_LINE:
		{
			fDouble->SetKnobStyle(THUMB_LINES);
			fSingle->SetKnobStyle(THUMB_LINES);
			
			fFixed->SetKnobStyle(THUMB_LINES);
			fProportional->SetKnobStyle(THUMB_LINES);
			
			fFlat->SetSelected(false);
			fDots->SetSelected(false);
			fLines->SetSelected(true);
			
			gSettings.knob=THUMB_LINES;
			HandleButtonStates();
			break;
		}
		
		case SCROLLBAR_KNOB_SIZE_CHANGED:
		{
			fDouble->SetKnobSize(fSizeAdjuster->KnobSize());
			fSingle->SetKnobSize(fSizeAdjuster->KnobSize());
			
			fFixed->SetKnobSize(fSizeAdjuster->KnobSize());
			fProportional->SetKnobSize(fSizeAdjuster->KnobSize());
			
			fFlat->SetKnobSize(fSizeAdjuster->KnobSize());
			fDots->SetKnobSize(fSizeAdjuster->KnobSize());
			fLines->SetKnobSize(fSizeAdjuster->KnobSize());
			
			gSettings.min_knob_size=fSizeAdjuster->KnobSize();
			HandleButtonStates();
			break;
		}
		
		default:
			BView::MessageReceived(msg);
			break;
	}
}
