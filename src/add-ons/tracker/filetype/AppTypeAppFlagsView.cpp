#include <AppTypeAppFlagsView.h>

AppTypeAppFlagsView::AppTypeAppFlagsView(BRect viewFrame)
	: BView(viewFrame, "AppTypeAppFlagsView", B_FOLLOW_ALL,
	        B_FRAME_EVENTS|B_WILL_DRAW)
{
	SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR) );
	
	BRect appFlagsBoxFrame = Bounds();
	fAppFlagsBox = new BBox(appFlagsBoxFrame,"box",
	                        B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
	AddChild(fAppFlagsBox);
	
	const char * appFlagsCheckBoxLabel = "Application Flags";
	float appFlagsCheckBoxStringWidth = StringWidth(appFlagsCheckBoxLabel);
	fAppFlagsCheckBox = new BCheckBox(appFlagsBoxFrame,
	                                  appFlagsCheckBoxLabel,
	                                  appFlagsCheckBoxLabel,NULL,
	                                  B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
	fAppFlagsBox->SetLabel(fAppFlagsCheckBox);
}

AppTypeAppFlagsView::~AppTypeAppFlagsView()
{
}

bool
AppTypeAppFlagsView::IsClean() const
{
	return true;
}
