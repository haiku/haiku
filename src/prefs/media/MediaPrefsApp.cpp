#include "MediaPrefsApp.h"


MediaPrefsApp::MediaPrefsApp()
	: BApplication( "application/x-vnd.OBeOS.Media" )
{
	BRect r;

	r.Set( 100, 100, 200, 300 );
	window = new MediaPrefsWindow( r );
	window->Show();
}


int
main( int argc, char **argv )
{
	MediaPrefsApp app;
	
	app.Run();
	return 0;
}
