#ifndef FILE_TYPE_VIEW_H
#define FILE_TYPE_VIEW_H

#include <Box.h>
#include <Button.h>
#include <PopUpMenu.h>
#include <String.h>
#include <View.h>

class FileTypeView : public BView {
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
	BString fPreferredApplication;
	
	BBox		* fFileTypeBox;
	BTextView	* fFileTypeTextView;
	BButton		* fFileTypeSelectButton;
	BButton		* fFileTypeSameAsButton;
	BBox		* fPreferredApplicationBox;
	BPopUpMenu	* fPreferredApplicationMenu;
	BButton		* fPreferredApplicationSelectButton;
	BButton		* fPreferredApplicationSameAsButton;
};

#endif // FILE_TYPE_VIEW_H
