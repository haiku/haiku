#include <AppTypeView.h>
#include <AppTypeAppFlagsView.h>
#include <AppTypeSupportedTypesView.h>
#include <AppTypeVersionInfoView.h>

AppTypeView::AppTypeView(BRect viewFrame)
	: BView(viewFrame, "AppTypeView", B_FOLLOW_ALL,
	        B_FRAME_EVENTS|B_WILL_DRAW)
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
	
	BRect versionInfoViewFrame = Bounds();
	versionInfoViewFrame.top = Bounds().Height()/2;
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
