#ifndef APP_TYPE_VIEW_H
#define APP_TYPE_VIEW_H

#include <Box.h>
#include <TextControl.h>

class AppTypeAppFlagsView;
class AppTypeSupportedTypesView;
class AppTypeVersionInfoView;

class AppTypeView : public BBox {
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
