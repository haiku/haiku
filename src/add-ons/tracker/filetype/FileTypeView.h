#ifndef FILE_TYPE_VIEW_H
#define FILE_TYPE_VIEW_H

#include <Box.h>
#include <Button.h>
#include <MenuField.h>
#include <String.h>
#include <TextControl.h>
#include <View.h>

class FileTypeView : public BBox {
public:
	FileTypeView(BRect viewFrame);
	~FileTypeView();

	void SetFileType(const char * fileType);
	void SetPreferredApplication(const char * preferredApplication);

	bool IsClean() const;
	const char * GetFileType() const;
	const char * GetPreferredApplication() const;
private:
	BString fFileType;
	BString fPreferredApp;
	
	BBox			* fFileTypeBox;
	BTextControl	* fFileTypeTextControl;
	BButton			* fFileTypeSelectButton;
	BButton			* fFileTypeSameAsButton;
	BBox			* fPreferredAppBox;
	BMenu			* fPreferredAppMenu;
	BMenuItem		* fPreferredAppMenuItemNone;
	BMenuField		* fPreferredAppMenuField;
	BButton			* fPreferredAppSelectButton;
	BButton			* fPreferredAppSameAsButton;
};

#endif // FILE_TYPE_VIEW_H
