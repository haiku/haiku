#include <AppTypeVersionInfoView.h>

AppTypeVersionInfoView::AppTypeVersionInfoView(BRect viewFrame)
	: BView(viewFrame, "AppTypeVersionInfoView", B_FOLLOW_ALL,
	        B_FRAME_EVENTS|B_WILL_DRAW)
{
	SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR) );
}

AppTypeVersionInfoView::~AppTypeVersionInfoView()
{
}

bool
AppTypeVersionInfoView::IsClean() const
{
	return true;
}
