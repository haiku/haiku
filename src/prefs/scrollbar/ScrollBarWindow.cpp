#include <Window.h>
#include <View.h>
#include <Box.h>
#include <Button.h>
#include <StringView.h>
#include <ScrollBarWindow.h>
#include <ScrollBarApp.h>

ScrollBarWindow::ScrollBarWindow(void) 
	: BWindow( BRect(50,50,398,325), "Scroll Bar", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
	BBox* bigBox = new BBox( BRect(0,0,348,275), NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_PLAIN_BORDER );
	BView* mainView = new BView( BRect(0,0,348,280),"mainView",B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW );
	mainView->SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR) );
	
	BBox* arrowstyleBox = new BBox( BRect( 12, 7, 169, 119) );
	arrowstyleBox->SetLabel( "Arrow Style" );
	char * doubleLabel = "Double:";
	float doubleWidth = mainView->StringWidth(doubleLabel);
	BStringView* doubleStringView = new BStringView( BRect(25,20,25+doubleWidth,38), "doubleStringView", doubleLabel );
	BView* doublearrowView = new BView( BRect(24,38,157,58), "doublearrowView", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW );
	char * singleLabel = "Single:";
	float singleWidth = mainView->StringWidth(singleLabel);
	BStringView* singleStringView = new BStringView( BRect(25,60,25+singleWidth,87), "singleStringView", singleLabel );
	BView* singlearrowView = new BView( BRect(24,87,157,107), "singlearrowView", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW );
	
	BBox* knobtypeBox = new BBox( BRect( 180, 7, 338, 119) );
	knobtypeBox->SetLabel( "Knob Type" );
	char * proportionalLabel = "Proportional:";
	float proportionalWidth = mainView->StringWidth(proportionalLabel);
	BStringView* proportionalStringView = new BStringView( BRect(193,20,193+proportionalWidth,38), "proportionalStringView", proportionalLabel );
	BView* proportionalknobView = new BView( BRect(192,38,326,58), "proportionalknobView", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW );
	char * fixedLabel = "Fixed:";
	float fixedWidth = mainView->StringWidth(fixedLabel);
	BStringView* fixedStringView = new BStringView( BRect(193,60,193+fixedWidth,87), "fixedStringView", fixedLabel );
	BView* fixedknobView = new BView( BRect(192,87,326,107), "fixedknobView", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW );
	
	
	BBox* minknobsizeBox = new BBox( BRect( 12, 123, 169, 232) );
	minknobsizeBox->SetLabel( "Minimum Knob Size" );
	
	BBox* knobstyleBox = new BBox( BRect( 180, 123, 338, 232) );
	knobstyleBox->SetLabel( "Knob Style" );
	BView* flatknobView = new BView( BRect(192,142,326,162), "flatknobView", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW );
	BView* dotknobView = new BView( BRect(192,173,326,193), "dotknobView", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW );
    BView* lineknobView = new BView( BRect(192,203,326,223), "lineknobView", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW );

	BButton* defaultsButton = new BButton( BRect( 10, 242, 85, 265), "defaultsButton", "Defaults", NULL );
	BButton* revertButton = new BButton( BRect( 95, 242, 170, 265), "revertButton", "Revert", NULL );
	revertButton->SetEnabled( FALSE );
	
	AddChild( mainView );
	mainView->AddChild( bigBox );
	
	mainView->AddChild( arrowstyleBox );
	mainView->AddChild( doubleStringView );
	mainView->AddChild( doublearrowView );
	mainView->AddChild( singleStringView );
	mainView->AddChild( singlearrowView );
	
	mainView->AddChild( knobtypeBox );
	mainView->AddChild( proportionalStringView );
	mainView->AddChild( proportionalknobView );	
	mainView->AddChild( fixedStringView );
	mainView->AddChild( fixedknobView );
	
	mainView->AddChild( minknobsizeBox );
	mainView->AddChild( knobstyleBox );
	mainView->AddChild( flatknobView );
	mainView->AddChild( lineknobView );
	mainView->AddChild( dotknobView );	
	
	mainView->AddChild( defaultsButton );
	mainView->AddChild( revertButton );
}

ScrollBarWindow::~ScrollBarWindow(void)
{
}

bool
ScrollBarWindow::QuitRequested(void)
{
	scroll_bar_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
