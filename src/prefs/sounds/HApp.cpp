#include "HApp.h"
#include "HWindow.h"
#include "HAboutWindow.h"
#include "RectUtils.h"

#define APP_SIG "application/x-vnd.openSounds"

/***********************************************************
 * Constructor
 ***********************************************************/
HApp::HApp() :BApplication(APP_SIG)
{
	BRect rect;
	RectUtils utils;
	if(utils.LoadRectFromApp("window_rect",&rect) == false)
	{
		rect.Set(50,50,450,400);
	}
	
	HWindow *win = new HWindow(rect,"OpenSounds");
	win->Show();
}	

/***********************************************************
 * Destructor
 ***********************************************************/
HApp::~HApp()
{

}

/***********************************************************
 * AboutRequested
 ***********************************************************/
void
HApp::AboutRequested()
{
	(new HAboutWindow("openSounds",
			__DATE__,
"Created by Atsushi Takamatsu @ Sapporo,Japan.\nOpenBeOS Development by Oliver Ruiz Dorantes @ Tarragona,Spain",
"http://anas.worldonline.es/urnenfel/beos/openbeos",
"E-Mail: urnenfelder@worldonline.es"))->Show();
}
