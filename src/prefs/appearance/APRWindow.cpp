#include "APRWindow.h"
#include "APRView.h"
#include "DecView.h"

APRWindow::APRWindow(BRect frame)
	: BWindow(frame, "Appearance", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
	tabview=new BTabView(Bounds(),"TabView");

	BTab *tab=NULL;

	APRView *v=new APRView(Bounds(),"Colors",B_FOLLOW_ALL, B_WILL_DRAW);
	tab=new BTab(v);
	tabview->AddTab(v,tab);

	DecView *dv=new DecView(Bounds(),"Decorator",B_FOLLOW_ALL, B_WILL_DRAW);
	tab=new BTab(dv);
	tabview->AddTab(dv,tab);

	AddChild(tabview);
}

bool APRWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(true);
}
