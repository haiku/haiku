#include <Application.h>
#include <Window.h>
#include <Alert.h>

#include "NetworkWindow.h"

#define SOFTWARE_EDITOR			"obos"
#define SOFTWARE_NAME			"Network"
#define SOFTWARE_VERSION_LABEL	"0.1.0 alpha"

const char * APPLICATION_SIGNATURE = "application/x-vnd." SOFTWARE_EDITOR "-" SOFTWARE_NAME;

const char * STR_ABOUT_TITLE =	"About " SOFTWARE_NAME;
const char * STR_ABOUT_BUTTON =	"Big Deal";	// "Hum?"
const char * STR_ABOUT_TEXT = 	SOFTWARE_NAME " " SOFTWARE_VERSION_LABEL "\n" \
								"A little Network setup tool.\n\n" \
								"The " SOFTWARE_NAME " Team:\n\n" \
								"OpenBeOS Networking Team\n" \
								"\n" \
								"Thanks to:\n" \
								"Be Inc.\n" \
								"Internet\n" \
								"Life\n";

class Application : public BApplication 
{
	public:
		// Constructors, destructors, operators...
		Application();

	public:
		// Virtual function overrides
		void			ReadyToRun(void);
		// virtual	void	MessageReceived(BMessage * msg);	
		virtual void	AboutRequested();
};


// --------------------------------------------------------------
int main()
{
	// Create an application instance
	Application * app;

	app = new Application();
	app->Run();
	delete app;
	
	return 0;
}



// --------------------------------------------------------------
Application::Application()
	: BApplication(APPLICATION_SIGNATURE)
{
}



// -------------------------------------------------
void Application::ReadyToRun(void)
{
	// Open at least one window!
	NetworkWindow *window;
	
	window = new NetworkWindow(SOFTWARE_NAME);
	window->Show();
}
	
	
// -------------------------------------------------
void Application::AboutRequested(void)
{
	BAlert * about_box;
	
	about_box = new BAlert(STR_ABOUT_TITLE, STR_ABOUT_TEXT, STR_ABOUT_BUTTON);
	about_box->Go();
}

