#include "APRWindow.h"
#include "APRView.h"

APRWindow::APRWindow(BRect frame)
	: BWindow(frame, "Appearance", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
	APRView *v=new APRView(Bounds(),"AppearanceView",B_FOLLOW_ALL, B_WILL_DRAW);
	AddChild(v);
}

bool APRWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(true);
}
