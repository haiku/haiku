#ifndef APP_TYPE_SUPPORTED_TYPES_VIEW_H
#define APP_TYPE_SUPPORTED_TYPES_VIEW_H

#include <Box.h>
#include <Button.h>
#include <ListView.h>
#include <ScrollView.h>
#include <View.h>

class AppTypeSupportedTypesView : public BView {
public:
	AppTypeSupportedTypesView(BRect viewFrame);
	~AppTypeSupportedTypesView();

	bool IsClean() const;
private:
	
	BBox			* fBox;
	BListView		* fListView;
	BScrollView		* fScrollView;
	BButton			* fAddButton;
	BButton			* fRemoveButton;
};

#endif // APP_TYPE_SUPPORTED_TYPES_VIEW_H
