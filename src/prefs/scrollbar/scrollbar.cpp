#include <Application.h>
#include "scrollbar.h"
#include "scrollbar_window.h"
#include "messages.h"

ScrollBarWindow *window;
ScrollBarApp MyApplication;
scrollbar_settings Settings;
scrollbar_settings RevertSettings;

ScrollBarApp::ScrollBarApp():BApplication( "application/x-vnd.OBOS-SCRL" )
{
	// Create Window
	window = new ScrollBarWindow();
	window->Show();
}

void ScrollBarApp::MessageReceived( BMessage *message )
{
	switch ( message->what )
	{
		case SCROLLBAR_ARROW_STYLE_DOUBLE:
			window->PostMessage( message );
			break;
		case SCROLLBAR_ARROW_STYLE_SINGLE:
			window->PostMessage( message );
			break;
		case SCROLLBAR_KNOB_TYPE_PROPORTIONAL:
			window->PostMessage( message );
			break;
		case SCROLLBAR_KNOB_TYPE_FIXED:
			window->PostMessage( message );
			break;
		case SCROLLBAR_KNOB_STYLE_FLAT:
			window->PostMessage( message );
			break;
		case SCROLLBAR_KNOB_STYLE_DOT:
			window->PostMessage( message );
			break;
		case SCROLLBAR_KNOB_STYLE_LINE:
			window->PostMessage( message );
			break;
		case SCROLLBAR_KNOB_SIZE_CHANGED:
			window->PostMessage( message );
			break;
		case TEST_MSG:
			window->PostMessage( message );
			break;
		default:
			BApplication::MessageReceived( message );
	}
}

int main()
{
	MyApplication.Run();
	
	return 0;
}