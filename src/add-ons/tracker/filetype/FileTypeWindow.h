#ifndef FILE_TYPE_WINDOW_H
#define FILE_TYPE_WINDOW_H

#include <EntryList.h>
#include <List.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Window.h>

class FileTypeView;

class FileTypeWindow
	: public BWindow
{
public:
					FileTypeWindow(const BList * entryList);
					~FileTypeWindow();
	
	virtual bool 	QuitRequested();
	virtual void 	MessageReceived(BMessage * message);
	
	status_t		InitCheck() const;
private:
	status_t		SaveRequested();
	void			SetEntries(const BList * entryList);
	const char *	SummarizeEntries();

	BMenuBar		* fMenuBar;
	BMenu			* fFileMenu;
	BMenuItem       * fSaveItem;
	BMenuItem       * fCloseItem;

	FileTypeView	* fView;
	
	BList			* fEntryList;
	status_t		initStatus;
};

#endif // FILE_TYPE_WINDOW_H
