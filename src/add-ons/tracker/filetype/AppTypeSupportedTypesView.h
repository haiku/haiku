#ifndef APP_TYPE_SUPPORTED_TYPES_VIEW_H
#define APP_TYPE_SUPPORTED_TYPES_VIEW_H

#include <Box.h>
#include <Button.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <String.h>
#include <StringView.h>
#include <TextControl.h>
#include <TextView.h>
#include <View.h>

class AppTypeSupportedTypesView : public BView {
public:
	AppTypeSupportedTypesView(BRect viewFrame);
	~AppTypeSupportedTypesView();

	bool IsClean() const;
private:
	
	BBox			* fSupportedTypesBox;
	BListView		* fSupportedTypesListView;
	BScrollView		* fSupportedTypesScrollView;
	BButton			* fSupportedTypesAddButton;
	BButton			* fSupportedTypesRemoveButton;
};

#endif // APP_TYPE_SUPPORTED_TYPES_VIEW_H
