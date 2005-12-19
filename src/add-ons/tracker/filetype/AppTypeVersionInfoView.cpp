#include "AppTypeVersionInfoView.h"
#include <SupportDefs.h>

#include <stdio.h>
AppTypeVersionInfoView::AppTypeVersionInfoView(BRect viewFrame)
	: BView(viewFrame, "AppTypeVersionInfoView", B_FOLLOW_ALL,
	        B_FRAME_EVENTS|B_WILL_DRAW)
{
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	float lineHeight = fontHeight.ascent+fontHeight.descent+fontHeight.leading;
	SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR) );

	fBox = new BBox(Bounds(),"box",B_FOLLOW_ALL);
	fBox->SetLabel("Version Info:");
	AddChild(fBox);
	
	// Version kind row

	fKindMenu = new BMenu("kind");
	fApplicationMenuItem = new BMenuItem("Application",NULL);
	fKindMenu->AddItem(fApplicationMenuItem);
	fSystemMenuItem = new BMenuItem("System",NULL);
	fKindMenu->AddItem(fSystemMenuItem);
	fKindMenu->SetRadioMode(true);
	fKindMenu->SetLabelFromMarked(true);
	fApplicationMenuItem->SetMarked(true);

	fKindMenuField = new BMenuField(Bounds(),"kindField",NULL,fKindMenu);
	const char * kindMenuLabel = "Version kind:";
	float kindMenuStringWidth = StringWidth(kindMenuLabel);
	fKindMenuField->SetLabel(kindMenuLabel);
	fKindMenuField->SetDivider(kindMenuStringWidth+5);
	float kindMenuFieldWidth = 0, kindMenuFieldHeight = 0;
	fKindMenuField->GetPreferredSize(&kindMenuFieldWidth,&kindMenuFieldHeight);
	fKindMenuField->ResizeTo(kindMenuFieldWidth,lineHeight+8);
	fKindMenuField->MoveTo(8,10);
	fBox->AddChild(fKindMenuField);
	
	// Version row
	
	fStringView = new BStringView(Bounds(),"version","Version:");
	float stringViewWidth = 0, stringViewHeight = 0;
	fStringView->GetPreferredSize(&stringViewWidth,&stringViewHeight);
	fStringView->ResizeTo(stringViewWidth,stringViewHeight);
	fStringView->MoveTo(9,fKindMenuField->Frame().bottom+5);
	fBox->AddChild(fStringView);
	
	fMajorTextControl = new BTextControl(BRect(0,0,21,21),"major",NULL,NULL,NULL);
	float majorTextControlWidth = 0, majorTextControlHeight = 0;
	fMajorTextControl->GetPreferredSize(&majorTextControlWidth,&majorTextControlHeight);
	fMajorTextControl->ResizeTo(20,majorTextControlHeight);
	fMajorTextControl->MoveTo(fStringView->Frame().right,fStringView->Frame().top-2);
	fBox->AddChild(fMajorTextControl);

	fDot1StringView = new BStringView(Bounds(),"dot1",".");
	float dot1stringViewWidth = 0, dot1stringViewHeight = 0;
	fDot1StringView->GetPreferredSize(&dot1stringViewWidth,&dot1stringViewHeight);
	fDot1StringView->ResizeTo(dot1stringViewWidth-2,dot1stringViewHeight);
	fDot1StringView->MoveTo(fMajorTextControl->Frame().right+3,fStringView->Frame().top);
	fBox->AddChild(fDot1StringView);
	
	fMiddleTextControl = new BTextControl(BRect(0,0,21,21),"middle",NULL,NULL,NULL);
	float middleTextControlWidth = 0, middleTextControlHeight = 0;
	fMiddleTextControl->GetPreferredSize(&middleTextControlWidth,&middleTextControlHeight);
	fMiddleTextControl->ResizeTo(20,middleTextControlHeight);
	fMiddleTextControl->MoveTo(fDot1StringView->Frame().right,fStringView->Frame().top-2);
	fBox->AddChild(fMiddleTextControl);

	fDot2StringView = new BStringView(Bounds(),"dot2",".");
	float dot2stringViewWidth = 0, dot2stringViewHeight = 0;
	fDot2StringView->GetPreferredSize(&dot2stringViewWidth,&dot2stringViewHeight);
	fDot2StringView->ResizeTo(dot2stringViewWidth-2,dot2stringViewHeight);
	fDot2StringView->MoveTo(fMiddleTextControl->Frame().right+3,fStringView->Frame().top);
	fBox->AddChild(fDot2StringView);

	fMinorTextControl = new BTextControl(BRect(0,0,21,21),"minor",NULL,NULL,NULL);
	float minorTextControlWidth = 0, minorTextControlHeight = 0;
	fMinorTextControl->GetPreferredSize(&minorTextControlWidth,&minorTextControlHeight);
	fMinorTextControl->ResizeTo(20,minorTextControlHeight);
	fMinorTextControl->MoveTo(fDot2StringView->Frame().right,fStringView->Frame().top-2);
	fBox->AddChild(fMinorTextControl);

	float varietyMenuFieldWidth = 0;
	fVarietyMenu = new BMenu("variety");
	const char * developmentMenuItemLabel = "Development";
	varietyMenuFieldWidth = max_c(StringWidth(developmentMenuItemLabel),varietyMenuFieldWidth);
	fDevelopmentMenuItem = new BMenuItem(developmentMenuItemLabel,NULL);
	fVarietyMenu->AddItem(fDevelopmentMenuItem);
	const char * alphaMenuItemLabel = "Alpha";
	varietyMenuFieldWidth = max_c(StringWidth(alphaMenuItemLabel),varietyMenuFieldWidth);
	fAlphaMenuItem = new BMenuItem(alphaMenuItemLabel,NULL);
	fVarietyMenu->AddItem(fAlphaMenuItem);
	const char * betaMenuItemLabel = "Beta";
	varietyMenuFieldWidth = max_c(StringWidth(betaMenuItemLabel),varietyMenuFieldWidth);
	fBetaMenuItem = new BMenuItem(betaMenuItemLabel,NULL);
	fVarietyMenu->AddItem(fBetaMenuItem);
	const char * gammaMenuItemLabel = "Gamma";
	varietyMenuFieldWidth = max_c(StringWidth(gammaMenuItemLabel),varietyMenuFieldWidth);
	fGammaMenuItem = new BMenuItem(gammaMenuItemLabel,NULL);
	fVarietyMenu->AddItem(fGammaMenuItem);
	const char * goldenMasterMenuItemLabel = "Golden master";
	varietyMenuFieldWidth = max_c(StringWidth(goldenMasterMenuItemLabel),varietyMenuFieldWidth);
	fGoldenMasterMenuItem = new BMenuItem(goldenMasterMenuItemLabel,NULL);
	fVarietyMenu->AddItem(fGoldenMasterMenuItem);
	const char * finalMenuItemLabel = "Final";
	varietyMenuFieldWidth = max_c(StringWidth(finalMenuItemLabel),varietyMenuFieldWidth);
	fFinalMenuItem = new BMenuItem(finalMenuItemLabel,NULL);
	fVarietyMenu->AddItem(fFinalMenuItem);
	fVarietyMenu->SetRadioMode(true);
	fVarietyMenu->SetLabelFromMarked(true);
	fDevelopmentMenuItem->SetMarked(true);

	fVarietyMenuField = new BMenuField(Bounds(),"varietyField",NULL,fVarietyMenu);
	fVarietyMenuField->ResizeTo(varietyMenuFieldWidth+18,lineHeight+8);
	fVarietyMenuField->MoveTo(fMinorTextControl->Frame().right+5,fStringView->Frame().top-3);
	fBox->AddChild(fVarietyMenuField);
	
	fSlashStringView = new BStringView(Bounds(),"slash","/");
	float slashStringViewWidth = 0, slashStringViewHeight = 0;
	fSlashStringView->GetPreferredSize(&slashStringViewWidth,&slashStringViewHeight);
	fSlashStringView->ResizeTo(slashStringViewWidth,slashStringViewHeight);
	fSlashStringView->MoveTo(fVarietyMenuField->Frame().right+5,fStringView->Frame().top);
	fBox->AddChild(fSlashStringView);

	fInternalTextControl = new BTextControl(BRect(0,0,21,21),"internal",NULL,NULL,NULL);
	float internalTextControlWidth = 0, internalTextControlHeight = 0;
	fInternalTextControl->GetPreferredSize(&internalTextControlWidth,&internalTextControlHeight);
	fInternalTextControl->ResizeTo(20,internalTextControlHeight);
	fInternalTextControl->MoveTo(fSlashStringView->Frame().right-2,fStringView->Frame().top-2);
	fBox->AddChild(fInternalTextControl);

	// Short description row

	const char * shortTextControlLabel = "Short Description:";
	float shortTextControlStringWidth = StringWidth(shortTextControlLabel);	
	fShortTextControl = new BTextControl(BRect(0,0,fBox->Bounds().Width()-16,21),
	                                     shortTextControlLabel,
	                                     shortTextControlLabel,NULL,NULL,
	                                     B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
	float shortTextControlWidth = 0, shortTextControlHeight = 0;
	fShortTextControl->GetPreferredSize(&shortTextControlWidth,&shortTextControlHeight);
	fShortTextControl->ResizeTo(fBox->Bounds().Width()-17,shortTextControlHeight);
	fShortTextControl->MoveTo(8,fVarietyMenuField->Frame().bottom+4);
	fShortTextControl->SetDivider(shortTextControlStringWidth+5);
	fBox->AddChild(fShortTextControl);

	const char * longStringViewLabel = "Long Description:";
	fLongStringView = new BStringView(Bounds(),longStringViewLabel,longStringViewLabel);
	float longStringViewWidth = 0, longStringViewHeight = 0;
	fLongStringView->GetPreferredSize(&longStringViewWidth,&longStringViewHeight);
	fLongStringView->ResizeTo(longStringViewWidth,longStringViewHeight);
	fLongStringView->MoveTo(9,fShortTextControl->Frame().bottom);
	fBox->AddChild(fLongStringView);
	
	BRect leftovers = fBox->Bounds();
	leftovers.InsetBy(8,8);
	leftovers.left += 1;
	leftovers.top = fLongStringView->Frame().bottom;
	BBox * textViewBox = new BBox(leftovers,"textbox",B_FOLLOW_ALL);
	fBox->AddChild(textViewBox);

	BRect textViewFrame = textViewBox->Bounds();
	textViewFrame.InsetBy(2,2);
	BRect textFrame = leftovers;
	textFrame.OffsetTo(0,0);
	fLongTextView = new BTextView(textViewFrame,"description",textFrame,B_FOLLOW_ALL,
	                              B_NAVIGABLE|B_WILL_DRAW|B_PULSE_NEEDED|B_FRAME_EVENTS);
	textViewBox->AddChild(fLongTextView);
}

AppTypeVersionInfoView::~AppTypeVersionInfoView()
{
}

bool
AppTypeVersionInfoView::IsClean() const
{
	return true;
}
