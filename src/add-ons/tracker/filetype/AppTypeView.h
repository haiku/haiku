#ifndef APP_TYPE_VIEW_H
#define APP_TYPE_VIEW_H

#include <TextControl.h>
#include <View.h>

class AppTypeAppFlagsView;
class AppTypeSupportedTypesView;
class AppTypeVersionInfoView;

class AppTypeView : public BView {
public:
	AppTypeView(BRect viewFrame);
	~AppTypeView();

	bool IsClean() const;
private:
	
	BTextControl				* fSignatureTextControl;
	AppTypeAppFlagsView			* fAppFlagsView;
	AppTypeSupportedTypesView	* fSupportedTypesView;
	AppTypeVersionInfoView		* fVersionInfoView;
};

#endif // APP_TYPE_VIEW_H
