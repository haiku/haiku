#include "ScrollBarView.h"
#include "messages.h"

// our own homegrown private cheat function for controlling scrollbars
status_t control_scrollbar(scroll_bar_info *info, BScrollBar *sb);

ScrollBarView::ScrollBarView(void)
 : BView(BRect(0,0,348,280),"scrollbarview", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW)
{
	SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR) );

	arrowstyleBox = new BBox( BRect( 12, 7, 169, 119) );
	arrowstyleBox->SetLabel( "Arrow Style" );

	float doubleWidth = StringWidth("Double:");
	int32 tempint;
	bool tempbool;
	scroll_bar_info sbi;
	
	// We're going to need the basics here
	get_scroll_bar_info(&sbi);
	
	// save this for later use
	tempbool=sbi.double_arrows;
	
	doubleStringView = new BStringView( BRect(25,20,25+doubleWidth,38), "doubleStringView", "Double:");
	sbdouble=new BScrollBar(BRect(12,38,145,58),"doublearrowsbar",NULL,0,100,B_HORIZONTAL);
	arrowstyleBox->AddChild( sbdouble );

	sbi.double_arrows=true;
	control_scrollbar(&sbi,sbdouble);
	
	float singleWidth = StringWidth("Single:");
	singleStringView = new BStringView( BRect(25,60,25+singleWidth,87), "singleStringView", "Single:");
	sbsingle=new BScrollBar(BRect(12,82,145,102),"singlearrowsbar",NULL,0,100,B_HORIZONTAL);
	arrowstyleBox->AddChild( sbsingle );
	
	sbi.double_arrows=false;
	control_scrollbar(&sbi,sbsingle);
	
	// Restore the original value so we don't screw up the other ones
	sbi.double_arrows=tempbool;
	
	tempbool=sbi.proportional;
	
	knobtypeBox = new BBox( BRect( 180, 7, 338, 119) );
	knobtypeBox->SetLabel( "Knob Type" );

	float proportionalWidth = StringWidth("Proportional");
	proportionalStringView = new BStringView( BRect(193,20,193+proportionalWidth,38), "proportionalStringView", "Proportional" );
	sbproportional=new BScrollBar(BRect(12,38,145,58),"proportionalbar",NULL,0,100,B_HORIZONTAL);
	knobtypeBox->AddChild( sbproportional );

	sbi.proportional=true;
	control_scrollbar(&sbi,sbproportional);

	float fixedWidth = StringWidth("Fixed");
	fixedStringView = new BStringView( BRect(193,60,193+fixedWidth,87), "fixedStringView", "Fixed" );
	sbfixed=new BScrollBar(BRect(12,87,145,105),"fixedbar",NULL,0,100,B_HORIZONTAL);
	knobtypeBox->AddChild( sbfixed );
	
	sbi.proportional=false;
	control_scrollbar(&sbi,sbfixed);
	
	minknobsizeBox = new BBox( BRect( 12, 123, 169, 232) );
	minknobsizeBox->SetLabel( "Minimum Knob Size" );
	
	sbi.proportional=tempbool;
	
	knobstyleBox = new BBox( BRect( 180, 123, 338, 232) );
	knobstyleBox->SetLabel( "Knob Style" );

	tempint=sbi.knob;
	sbflat=new BScrollBar(BRect(12,22,145,42),"flatbar",NULL,0,100,B_HORIZONTAL);
	knobstyleBox->AddChild( sbflat );
	sbi.knob=0;
	control_scrollbar(&sbi,sbflat);
	
	sbdots=new BScrollBar(BRect(12,52,145,72),"dotbar",NULL,0,100,B_HORIZONTAL);
	knobstyleBox->AddChild( sbdots );
	sbi.knob=1;
	control_scrollbar(&sbi,sbdots);

	sblines=new BScrollBar(BRect(12,82,145,102),"linebar",NULL,0,100,B_HORIZONTAL);
	knobstyleBox->AddChild( sblines );
	sbi.knob=2;
	control_scrollbar(&sbi,sblines);
	

	defaultsButton = new BButton( BRect( 10, 242, 85, 265), "defaultsButton", "Defaults", NULL );
	revertButton = new BButton( BRect( 95, 242, 170, 265), "revertButton", "Revert", NULL );
	revertButton->SetEnabled( FALSE );
	
	AddChild( arrowstyleBox );
	AddChild( doubleStringView );
	AddChild( singleStringView );
	
	AddChild( knobtypeBox );
	AddChild( proportionalStringView );
	AddChild( fixedStringView );
	
	AddChild( minknobsizeBox );
	AddChild( knobstyleBox );
	
	AddChild( defaultsButton );
	AddChild( revertButton );
}

ScrollBarView::~ScrollBarView(void)
{
}

void ScrollBarView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		default:
			break;
	}
}
