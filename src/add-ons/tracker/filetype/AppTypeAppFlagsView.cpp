#include "AppTypeAppFlagsView.h"

AppTypeAppFlagsView::AppTypeAppFlagsView(BRect viewFrame)
	: BView(viewFrame, "AppTypeAppFlagsView", B_FOLLOW_ALL,
	        B_FRAME_EVENTS|B_WILL_DRAW)
{
	float width = 0, height = 0;
	SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR) );
	
	fAppFlagsBox = new BBox(Bounds(),"box",
	                        B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
	AddChild(fAppFlagsBox);
	
	const char * appFlagsCheckBoxLabel = "Application Flags";
	float appFlagsCheckBoxStringWidth = StringWidth(appFlagsCheckBoxLabel);
	fAppFlagsCheckBox = new BCheckBox(fAppFlagsBox->Bounds(),
	                                  appFlagsCheckBoxLabel,
	                                  appFlagsCheckBoxLabel,NULL,
	                                  B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
	fAppFlagsBox->SetLabel(fAppFlagsCheckBox);

	const char * singleRadioButtonLabel = "Single Launch";
	float singleRadioButtonStringWidth = StringWidth(singleRadioButtonLabel);
	BRect singleRadioButtonFrame = fAppFlagsBox->Bounds();
	singleRadioButtonFrame.top = fAppFlagsCheckBox->Bounds().Height()/2 + 10;
	singleRadioButtonFrame.left = 10;
	singleRadioButtonFrame.right -= 10;	
	fAppFlagsSingleRadioButton = new BRadioButton(singleRadioButtonFrame,
	                                              singleRadioButtonLabel,
	                                              singleRadioButtonLabel,NULL);
	fAppFlagsBox->AddChild(fAppFlagsSingleRadioButton);
	fAppFlagsSingleRadioButton->GetPreferredSize(&width,&height);
	fAppFlagsSingleRadioButton->ResizeTo(width,height);
	float leftWidth = width;

	const char * multipleRadioButtonLabel = "Multiple Launch";
	float multipleRadioButtonStringWidth = StringWidth(multipleRadioButtonLabel);
	BRect multipleRadioButtonFrame = fAppFlagsBox->Bounds();
	multipleRadioButtonFrame.top = fAppFlagsSingleRadioButton->Frame().bottom;
	multipleRadioButtonFrame.left = 10;
	multipleRadioButtonFrame.right -= 10;	
	fAppFlagsMultipleRadioButton = new BRadioButton(multipleRadioButtonFrame,
	                                                multipleRadioButtonLabel,
	                                                multipleRadioButtonLabel,NULL);
	fAppFlagsBox->AddChild(fAppFlagsMultipleRadioButton);
	fAppFlagsMultipleRadioButton->GetPreferredSize(&width,&height);
	fAppFlagsMultipleRadioButton->ResizeTo(width,height);
	if (width > leftWidth) leftWidth = width;

	const char * exclusiveRadioButtonLabel = "Exclusive Launch";
	float exclusiveRadioButtonStringWidth = StringWidth(exclusiveRadioButtonLabel);
	BRect exclusiveRadioButtonFrame = fAppFlagsBox->Bounds();
	exclusiveRadioButtonFrame.top = fAppFlagsMultipleRadioButton->Frame().bottom;
	exclusiveRadioButtonFrame.left = 10;
	exclusiveRadioButtonFrame.right -= 10;	
	fAppFlagsExclusiveRadioButton = new BRadioButton(exclusiveRadioButtonFrame,
	                                                 exclusiveRadioButtonLabel,
	                                                 exclusiveRadioButtonLabel,NULL);
	fAppFlagsBox->AddChild(fAppFlagsExclusiveRadioButton);
	fAppFlagsExclusiveRadioButton->GetPreferredSize(&width,&height);
	fAppFlagsExclusiveRadioButton->ResizeTo(width,height);
	if (width > leftWidth) leftWidth = width;

	const char * argvOnlyCheckBoxLabel = "Argv Only";
	float argvOnlyCheckBoxStringWidth = StringWidth(argvOnlyCheckBoxLabel);
	BRect argvOnlyCheckBoxFrame = fAppFlagsBox->Bounds();
	argvOnlyCheckBoxFrame.top = fAppFlagsSingleRadioButton->Frame().top - 1;
	argvOnlyCheckBoxFrame.left = 10 + leftWidth;
	argvOnlyCheckBoxFrame.right -= 10;	
	fAppFlagsArgvOnlyCheckBox = new BCheckBox(argvOnlyCheckBoxFrame,
	                                          argvOnlyCheckBoxLabel,
	                                          argvOnlyCheckBoxLabel,NULL);
	fAppFlagsBox->AddChild(fAppFlagsArgvOnlyCheckBox);
	fAppFlagsArgvOnlyCheckBox->GetPreferredSize(&width,&height);
	fAppFlagsArgvOnlyCheckBox->ResizeTo(width,height);
	float rightWidth = width;
	
	const char * backgroundCheckBoxLabel = "Background App";
	float backgroundCheckBoxStringWidth = StringWidth(backgroundCheckBoxLabel);
	BRect backgroundCheckBoxFrame = fAppFlagsBox->Bounds();
	backgroundCheckBoxFrame.top = fAppFlagsArgvOnlyCheckBox->Frame().bottom - 1;
	backgroundCheckBoxFrame.left = 10 + leftWidth;
	backgroundCheckBoxFrame.right -= 10;
	fAppFlagsBackgroundCheckBox = new BCheckBox(backgroundCheckBoxFrame,
	                                            backgroundCheckBoxLabel,
	                                            backgroundCheckBoxLabel,NULL);
	fAppFlagsBox->AddChild(fAppFlagsBackgroundCheckBox);
	fAppFlagsBackgroundCheckBox->GetPreferredSize(&width,&height);
	fAppFlagsBackgroundCheckBox->ResizeTo(width,height);
	if (width > rightWidth) rightWidth = width;
	
	width = 10 + leftWidth + rightWidth + 5;
	height = fAppFlagsExclusiveRadioButton->Frame().bottom + 5;
	fAppFlagsBox->ResizeTo(width,height);
	ResizeTo(width,height);
}

AppTypeAppFlagsView::~AppTypeAppFlagsView()
{
}

bool
AppTypeAppFlagsView::IsClean() const
{
	return true;
}
