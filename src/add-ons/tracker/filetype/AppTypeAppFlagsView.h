#ifndef APP_TYPE_APP_FLAGS_VIEW_H
#define APP_TYPE_APP_FLAGS_VIEW_H

#include <Box.h>
#include <CheckBox.h>
#include <RadioButton.h>
#include <View.h>

class AppTypeAppFlagsView : public BView {
public:
	AppTypeAppFlagsView(BRect viewFrame);
	~AppTypeAppFlagsView();

	bool IsClean() const;
private:
	
	BBox			* fAppFlagsBox;
	BCheckBox		* fAppFlagsCheckBox;
	BRadioButton	* fAppFlagsSingleRadioButton;
	BRadioButton	* fAppFlagsMultipleRadioButton;
	BRadioButton	* fAppFlagsExclusiveRadioButton;
	BCheckBox		* fAppFlagsArgvOnlyCheckBox;
	BCheckBox		* fAppFlagsBackgroundCheckBox;
};

#endif // APP_TYPE_APP_FLAGS_VIEW_H
