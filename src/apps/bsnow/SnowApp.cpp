#include <Application.h>
#include <Catalog.h>
#include <Dragger.h>
#include <Window.h>

#include "SnowView.h"


int main(int argc, char **argv)
{
	BApplication app(APP_SIG);
	BWindow* win;
	bool draggersShown = BDragger::AreDraggersDrawn();
	win = new BWindow(BRect(SNOW_VIEW_RECT), B_TRANSLATE_SYSTEM_NAME("BSnow"),
		B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE | B_NOT_RESIZABLE);
	SnowView* view = new SnowView();
	win->AddChild(view);
	win->MoveTo(50, 50);
	win->Show();
	win->SetPulseRate(500000);
	BDragger::ShowAllDraggers();
	app.Run();
	if (!draggersShown)
		BDragger::HideAllDraggers();
	return 0;
}

