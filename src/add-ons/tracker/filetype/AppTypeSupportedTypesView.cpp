#include "AppTypeSupportedTypesView.h"
#include <SupportDefs.h>

AppTypeSupportedTypesView::AppTypeSupportedTypesView(BRect viewFrame)
	: BView(viewFrame, "AppTypeSupportedTypesView", 
	        B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP,
	        B_FRAME_EVENTS|B_WILL_DRAW)
{
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	float lineHeight = fontHeight.ascent+fontHeight.descent+fontHeight.leading;
	SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR) );
	
	fBox = new BBox(Bounds(),"box",B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
	fBox->SetLabel("Supported Types:");
	AddChild(fBox);
	
	const char * addButtonLabel = "Add...";
	fAddButton = new BButton(Bounds(),addButtonLabel,addButtonLabel,NULL);
	float addButtonWidth = 0, addButtonHeight = 0;
	fAddButton->GetPreferredSize(&addButtonWidth,&addButtonHeight);

	const char * removeButtonLabel = "Remove";
	fRemoveButton = new BButton(Bounds(),removeButtonLabel,removeButtonLabel,NULL);
	float removeButtonWidth = 0, removeButtonHeight = 0;
	fRemoveButton->GetPreferredSize(&removeButtonWidth,&removeButtonHeight);
	
	float buttonWidth = max_c(addButtonWidth,removeButtonWidth);
	float buttonHeight = max_c(addButtonHeight,removeButtonHeight);
	fRemoveButton->ResizeTo(buttonWidth,buttonHeight);
	fAddButton->ResizeTo(buttonWidth,buttonHeight);
	
	fAddButton->MoveTo(fBox->Bounds().Width() - 60 - fAddButton->Bounds().Width(),14);
	fRemoveButton->MoveTo(fAddButton->Frame().left,fAddButton->Frame().bottom);
	
	fListView = new BListView(Bounds(),"listview");
	fListView->ResizeTo(fAddButton->Frame().left - 20 - B_V_SCROLL_BAR_WIDTH,
	                    max_c(1*lineHeight,2*buttonHeight - 2));
	fListView->MoveTo(12,fAddButton->Frame().top+2);
	fScrollView = new BScrollView("scrollview",fListView,B_FOLLOW_ALL,
	                              B_FRAME_EVENTS|B_WILL_DRAW,false,true);
	fBox->AddChild(fScrollView);
	fBox->AddChild(fAddButton);
	fBox->AddChild(fRemoveButton);
	fBox->ResizeTo(Bounds().Width(),fScrollView->Frame().bottom+8);
	ResizeTo(fBox->Bounds().Width(),fBox->Bounds().Height());
}

AppTypeSupportedTypesView::~AppTypeSupportedTypesView()
{
}

bool
AppTypeSupportedTypesView::IsClean() const
{
	return true;
}
