#include <AppTypeView.h>
#include <AppTypeVersionInfoView.h>

AppTypeView::AppTypeView(BRect viewFrame)
	: BView(viewFrame, "AppTypeView", B_FOLLOW_ALL,
	        B_FRAME_EVENTS|B_WILL_DRAW)
{
	fVersionInfoView = 0;
	SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR) );
	
	BRect versionInfoViewFrame = Bounds();
	versionInfoViewFrame.top = Bounds().Height()/2;
	fVersionInfoView = new AppTypeVersionInfoView(versionInfoViewFrame);
	AddChild(fVersionInfoView);
}

AppTypeView::~AppTypeView()
{
	delete fVersionInfoView;
}

bool
AppTypeView::IsClean() const
{
	if (!fVersionInfoView->IsClean()) {
		return false;
	}
	return true;
}
