#include "ScrollBarView.h"
#include "messages.h"

// our own homegrown private cheat function for controlling scrollbars
status_t control_scrollbar(int8 command, void *data, BScrollBar *sb);

// Magic numbers for control_scrollbar()
#define CONTROL_SET_DOUBLE 1
#define CONTROL_SET_PROPORTIONAL 2
#define CONTROL_SET_STYLE 3

ScrollBarView::ScrollBarView(void)
 : BView(BRect(0,0,348,280),"scrollbarview", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW)
{
	SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR) );

	arrowstyleBox = new BBox( BRect( 12, 7, 169, 119) );
	arrowstyleBox->SetLabel( "Arrow Style" );

	float doubleWidth = StringWidth("Double:");
	bool tempbool;
	int8 tempint;
	
	doubleStringView = new BStringView( BRect(25,20,25+doubleWidth,38), "doubleStringView", "Double:");
	sbdouble=new BScrollBar(BRect(12,38,145,58),"doublearrowsbar",NULL,0,100,B_HORIZONTAL);
	arrowstyleBox->AddChild( sbdouble );

	tempbool=true;
	control_scrollbar(CONTROL_SET_DOUBLE,&tempbool,sbdouble);
	
	float singleWidth = StringWidth("Single:");
	singleStringView = new BStringView( BRect(25,60,25+singleWidth,87), "singleStringView", "Single:");
	sbsingle=new BScrollBar(BRect(12,82,145,102),"singlearrowsbar",NULL,0,100,B_HORIZONTAL);
	arrowstyleBox->AddChild( sbsingle );

	tempbool=false;
	control_scrollbar(CONTROL_SET_DOUBLE,&tempbool,sbsingle);


	
	knobtypeBox = new BBox( BRect( 180, 7, 338, 119) );
	knobtypeBox->SetLabel( "Knob Type" );

	float proportionalWidth = StringWidth("Proportional");
	proportionalStringView = new BStringView( BRect(193,20,193+proportionalWidth,38), "proportionalStringView", "Proportional" );
	sbproportional=new BScrollBar(BRect(12,38,145,58),"proportionalbar",NULL,0,100,B_HORIZONTAL);
	knobtypeBox->AddChild( sbproportional );

	tempbool=true;
	control_scrollbar(CONTROL_SET_PROPORTIONAL, &tempbool,sbdouble);

	float fixedWidth = StringWidth("Fixed");
	fixedStringView = new BStringView( BRect(193,60,193+fixedWidth,87), "fixedStringView", "Fixed" );
	sbfixed=new BScrollBar(BRect(12,87,145,105),"fixedbar",NULL,0,100,B_HORIZONTAL);
	knobtypeBox->AddChild( sbfixed );
	
	tempbool=false;
	control_scrollbar(CONTROL_SET_PROPORTIONAL,&tempbool,sbfixed);
	
	minknobsizeBox = new BBox( BRect( 12, 123, 169, 232) );
	minknobsizeBox->SetLabel( "Minimum Knob Size" );
	
	knobstyleBox = new BBox( BRect( 180, 123, 338, 232) );
	knobstyleBox->SetLabel( "Knob Style" );

	sbflat=new BScrollBar(BRect(12,22,145,42),"flatbar",NULL,0,100,B_HORIZONTAL);
	knobstyleBox->AddChild( sbflat );
	tempint=0;
	control_scrollbar(CONTROL_SET_STYLE,&tempbool,sbflat);
	
	sbdots=new BScrollBar(BRect(12,52,145,72),"dotbar",NULL,0,100,B_HORIZONTAL);
	knobstyleBox->AddChild( sbdots );
	tempint=1;
	control_scrollbar(CONTROL_SET_STYLE,&tempbool,sbdots);

	sblines=new BScrollBar(BRect(12,82,145,102),"linebar",NULL,0,100,B_HORIZONTAL);
	knobstyleBox->AddChild( sblines );
	tempint=2;
	control_scrollbar(CONTROL_SET_STYLE,&tempbool,sblines);

	

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
