#include <AppTypeView.h>
#include <AppTypeAppFlagsView.h>
#include <AppTypeSupportedTypesView.h>
#include <AppTypeVersionInfoView.h>

AppTypeView::AppTypeView(BRect viewFrame)
	: BBox(viewFrame, "AppTypeView", B_FOLLOW_ALL,
	        B_FRAME_EVENTS|B_WILL_DRAW, B_PLAIN_BORDER)
{
	SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR) );
	
	BRect signatureTextControlFrame = Bounds();
	signatureTextControlFrame.top += 10;
	signatureTextControlFrame.left += 10;
	signatureTextControlFrame.right -= 60;
	const char * signatureTextControlLabel = "Signature:";
	float signatureTextControlStringWidth = StringWidth(signatureTextControlLabel);	
	fSignatureTextControl = new BTextControl(signatureTextControlFrame,
	                                         signatureTextControlLabel,
	                                         signatureTextControlLabel,NULL,NULL,
	                                         B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
	fSignatureTextControl->SetDivider(signatureTextControlStringWidth+5);
	AddChild(fSignatureTextControl);
	
	BRect appFlagsViewFrame = Bounds();
	appFlagsViewFrame.top = fSignatureTextControl->Bounds().bottom + 10;
	appFlagsViewFrame.left += 10;
	appFlagsViewFrame.right -= 60;
	fAppFlagsView = new AppTypeAppFlagsView(appFlagsViewFrame);
	AddChild(fAppFlagsView);
	
	BRect appSupportedTypesViewFrame = Bounds();
	appSupportedTypesViewFrame.top = fAppFlagsView->Frame().bottom + 10;
	appSupportedTypesViewFrame.left = appFlagsViewFrame.left;
	appSupportedTypesViewFrame.right -= 10;
	fSupportedTypesView = new AppTypeSupportedTypesView(appSupportedTypesViewFrame);
	AddChild(fSupportedTypesView);
		
	BRect versionInfoViewFrame = Bounds();
	versionInfoViewFrame.top = fSupportedTypesView->Frame().bottom + 10;
	versionInfoViewFrame.left = appSupportedTypesViewFrame.left;
	versionInfoViewFrame.right = appSupportedTypesViewFrame.right;
	versionInfoViewFrame.bottom -= 10;
	fVersionInfoView = new AppTypeVersionInfoView(versionInfoViewFrame);
	AddChild(fVersionInfoView);
}

AppTypeView::~AppTypeView()
{
}

bool
AppTypeView::IsClean() const
{
	if (!fVersionInfoView->IsClean()) {
		return false;
	}
	return true;
}
