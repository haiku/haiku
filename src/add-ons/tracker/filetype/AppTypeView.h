#ifndef APP_TYPE_VIEW_H
#define APP_TYPE_VIEW_H

#include <Box.h>
#include <Button.h>
#include <MenuField.h>
#include <String.h>
#include <TextControl.h>
#include <View.h>

class AppTypeVersionInfoView;

class AppTypeView : public BView {
public:
	AppTypeView(BRect viewFrame);
	~AppTypeView();

	bool IsClean() const;
private:
	
	BTextControl	* fSignatureTextControl;
	
	BBox			* fAppFlagsBox;
	BCheckBox		* fAppFlagsCheckBox;
	BRadioButton	* fAppFlagsSingleRadioButton;
	BRadioButton	* fAppFlagsMultipleRadioButton;
	BRadioButton	* fAppFlagsExclusiveRadioButton;
	BCheckBox		* fAppFlagsArgvOnlyCheckBox;
	BCheckBox		* fAppFlagsBackOnlyCheckBox;
	
	BBox			* fSupportedTypesBox;
	BListView		* fSupportedTypesListView;
	BScrollView		* fSupportedTypesScrollView;
	BButton			* fSupportedTypesAddButton;
	BButton			* fSupportedTypesRemoveButton;
	
	AppTypeVersionInfoView * fVersionInfoView;
};

#endif // APP_TYPE_VIEW_H
