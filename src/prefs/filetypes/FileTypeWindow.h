#ifndef FILE_TYPE_WINDOW_H
#define FILE_TYPE_WINDOW_H

#include <EntryList.h>
#include <List.h>
#include <MenuBar.h>
#include <Window.h>

class FileTypeView;

class FileTypeWindow
	: public BWindow
{
public:
					FileTypeWindow(BEntryList entries);
					~FileTypeWindow();
	
	virtual bool 	QuitRequested();
	virtual void 	MessageReceived(BMessage * message);
	
private:
	status_t		SaveRequested();
	void			SetEntries(BEntryList entries);
	const char *	SummarizeEntries();

	BMenuBar		*fMenuBar;
	BMenu			*fFileMenu;
	BMenuItem       *fSaveItem;
	BMenuItem       *fCloseItem;

	FileTypeView	*fView;
	
	BEntryList		fEntries;
};

#endif // FILE_TYPE_WINDOW_H
