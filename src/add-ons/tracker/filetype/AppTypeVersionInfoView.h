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
	
	BBox			* fVersionInfoBox;
	BMenu			* fVersionKindMenu;
	BMenuItem		* fVersionApplicationMenuItem;
	BMenuItem		* fVersionSystemMenuItem;
	BMenuField		* fVersionKindMenuField;
	BStringView		* fVersionStringView;
	BTextControl	* fVersionMajorTextControl;
	BTextControl	* fVersionMiddleTextControl;
	BTextControl	* fVersionMinorTextControl;
	BMenu			* fVarietyMenu;
	BMenuItem		* fVarietyDevelopmentMenuItem;
	BMenuItem		* fVarietyAlphaMenuItem;
	BMenuItem		* fVarietyBetaMenuItem;
	BMenuItem		* fVarietyGammaMenuItem;
	BMenuItem		* fVarietyGoldenMasterMenuItem;
	BMenuItem		* fVarietyFinalMenuItem;
	BMenuField		* fVarietyMenuField;
	BStringView		* fSlashStringView;
	BTextControl	* fInternalTextControl;
	BStringView		* fShortDescriptionStringView;
	BTextControl	* fShortDescriptionTextControl;
	BStringView		* fLongDescriptionStringView;
	BTextView		* fLongDescriptionTextView;
};

#endif // APP_TYPE_VERSION_INFO_VIEW_H
