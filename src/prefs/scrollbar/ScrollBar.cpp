#include <Be.h>

class ScrollBarApp : public BApplication
{
	public:
		ScrollBarApp();
};

int main(int, char**)
{
	ScrollBarApp myApplication;
	
	myApplication.Run();
	
	return 0;
}

ScrollBarApp::ScrollBarApp():BApplication("application/x-vnd.scrollbarpref")
{
	BWindow* myWindow = new BWindow( BRect(50,50,398,325), "Scroll Bar", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE );
	BBox* bigBox = new BBox( BRect(0,0,348,275), NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_PLAIN_BORDER );
	BView* mainView = new BView( BRect(0,0,348,280),"mainView",B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW );
	mainView->SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR) );
	
	BBox* arrowstyleBox = new BBox( BRect( 12, 7, 169, 119) );
	arrowstyleBox->SetLabel( "Arrow Style" );
	BStringView* doubleStringView = new BStringView( BRect(25,20,60,38), "doubleStringView", "Double:" );
	BView* doublearrowView = new BView( BRect(24,38,157,58), "doublearrowView", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW );
	BStringView* singleStringView = new BStringView( BRect(25,60,60,87), "singleStringView", "Single:" );
	BView* singlearrowView = new BView( BRect(24,87,157,107), "singlearrowView", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW );
	
	BBox* knobtypeBox = new BBox( BRect( 180, 7, 338, 119) );
	knobtypeBox->SetLabel( "Knob Type" );
	BStringView* proportionalStringView = new BStringView( BRect(193,20,260,38), "proportionalStringView", "Proportional:" );
	BView* proportionalknobView = new BView( BRect(192,38,326,58), "proportionalknobView", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW );
	BStringView* fixedStringView = new BStringView( BRect(193,60,250,87), "fixedStringView", "Fixed:" );
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
	
	myWindow->AddChild( mainView );
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
		
	myWindow->Show();
}