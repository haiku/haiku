#include <AppTypeSupportedTypesView.h>

AppTypeSupportedTypesView::AppTypeSupportedTypesView(BRect viewFrame)
	: BView(viewFrame, "AppTypeSupportedTypesView", B_FOLLOW_ALL,
	        B_FRAME_EVENTS|B_WILL_DRAW)
{
	SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR) );
}

AppTypeSupportedTypesView::~AppTypeSupportedTypesView()
{
}

bool
AppTypeSupportedTypesView::IsClean() const
{
	return true;
}
