#include <AppTypeSupportedTypesView.h>

AppTypeSupportedTypesView::AppTypeSupportedTypesView(BRect viewFrame)
	: BView(viewFrame, "AppTypeSupportedTypesView", B_FOLLOW_ALL,
	        B_FRAME_EVENTS|B_WILL_DRAW)
{
	float width = 0, height = 0;
	SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR) );
	
	fSupportedTypesBox = new BBox(Bounds(),"box",
	                              B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
	fSupportedTypesBox->SetLabel("Supported Types:");
	AddChild(fSupportedTypesBox);
	
	const char * addButtonLabel = "Add...";
	float addButtonStringWidth = StringWidth(addButtonLabel);
	BRect addButtonFrame = fSupportedTypesBox->Bounds();
	addButtonFrame.top = 10;
	addButtonFrame.left = 10;
	addButtonFrame.right -= 10;
	fSupportedTypesAddButton = new BButton(addButtonFrame,
	                                       addButtonLabel,
	                                       addButtonLabel,NULL);
	fSupportedTypesBox->AddChild(fSupportedTypesAddButton);
	fSupportedTypesAddButton->GetPreferredSize(&width,&height);
	fSupportedTypesAddButton->ResizeTo(width,height);
	float leftWidth = width;
	
	
}

AppTypeSupportedTypesView::~AppTypeSupportedTypesView()
{
}

bool
AppTypeSupportedTypesView::IsClean() const
{
	return true;
}
