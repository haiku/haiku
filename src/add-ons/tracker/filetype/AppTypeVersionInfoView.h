#ifndef APP_TYPE_VERSION_INFO_VIEW_H
#define APP_TYPE_VERSION_INFO_VIEW_H

#include <Box.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <String.h>
#include <StringView.h>
#include <TextControl.h>
#include <TextView.h>
#include <View.h>

class AppTypeVersionInfoView : public BView {
public:
	AppTypeVersionInfoView(BRect viewFrame);
	~AppTypeVersionInfoView();

	bool IsClean() const;
private:
	
	BBox			* fBox;
	BMenu			* fKindMenu;
	BMenuItem		* fApplicationMenuItem;
	BMenuItem		* fSystemMenuItem;
	BMenuField		* fKindMenuField;
	BStringView		* fStringView;
	BTextControl	* fMajorTextControl;
	BStringView		* fDot1StringView;
	BTextControl	* fMiddleTextControl;
	BStringView		* fDot2StringView;
	BTextControl	* fMinorTextControl;
	BMenu			* fVarietyMenu;
	BMenuItem		* fDevelopmentMenuItem;
	BMenuItem		* fAlphaMenuItem;
	BMenuItem		* fBetaMenuItem;
	BMenuItem		* fGammaMenuItem;
	BMenuItem		* fGoldenMasterMenuItem;
	BMenuItem		* fFinalMenuItem;
	BMenuField		* fVarietyMenuField;
	BStringView		* fSlashStringView;
	BTextControl	* fInternalTextControl;
	BStringView		* fShortStringView;
	BTextControl	* fShortTextControl;
	BStringView		* fLongStringView;
	BTextView		* fLongTextView;
};

#endif // APP_TYPE_VERSION_INFO_VIEW_H
