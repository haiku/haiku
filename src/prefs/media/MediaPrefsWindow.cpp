#include "MediaPrefsApp.h"
#include "VolumeControl.h"


MediaPrefsWindow::MediaPrefsWindow( BRect wRect )
	: BWindow( wRect, "Media",  B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS )
{
	float fLeft, fRight;
	int32 iLeft, iRight;

	wRect.OffsetTo( B_ORIGIN );
	view = new MediaPrefsView( wRect, "MainView" );
	view->SetViewColor( ui_color( B_PANEL_BACKGROUND_COLOR ) );

	this->AddChild( view );

	slider = new MediaPrefsSlider( wRect, "MasterVol", "Master",
								new BMessage( 'mVol' ), 2,
								B_FOLLOW_LEFT|B_FOLLOW_TOP, B_WILL_DRAW );
	slider->SetLimits( 0, 100 );
	slider->SetLimitLabels( "0", "100" );

	view->AddChild( slider );

	// use private API
	if ( MediaKitPrivate::GetMasterVolume( &fLeft, &fRight ) == B_OK ) {
		// left,right range is 0..9.999
		iLeft = (int32)( fLeft / vI2F );
		iRight = (int32)( fRight / vI2F );
	} else {
		iLeft = 0;
		iRight = 0;
	}

	slider->SetValueFor( 0, iLeft );
	slider->SetValueFor( 1, iRight );
}


bool MediaPrefsWindow::QuitRequested()
{
	be_app->PostMessage( B_QUIT_REQUESTED );
	return true;
}


void MediaPrefsWindow::MessageReceived( BMessage *msg )
{
	switch ( msg->what ) {
		case 'mVol':
			{
				int32 iLeft = slider->ValueFor( 0 );
				int32 iRight = slider->ValueFor( 1 );

				float fLeft = float( iLeft ) * vI2F;
				float fRight = float( iRight ) * vI2F;

				MediaKitPrivate::SetMasterVolume( fLeft, fRight );
			}
			break;

		default:
			BWindow::MessageReceived( msg );
	}
}
