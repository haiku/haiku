#ifndef APP_TYPE_WINDOW_H
#define APP_TYPE_WINDOW_H

#include <EntryList.h>
#include <List.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Window.h>

class AppTypeView;

class AppTypeWindow
	: public BWindow
{
public:
					AppTypeWindow(const BEntry * entry);
					~AppTypeWindow();
	
	virtual void	Quit();
	virtual bool 	QuitRequested();
	virtual void 	MessageReceived(BMessage * message);
	
	status_t		InitCheck() const;
private:
	status_t		SaveRequested();
	void			SetEntry(const BEntry * entry);

	BMenuBar		* fMenuBar;
	BMenu			* fFileMenu;
	BMenuItem       * fSaveItem;
	BMenuItem       * fCloseItem;

	AppTypeView	* fView;
	
	BEntry			* fEntry;
	status_t		initStatus;
};

#endif // APP_TYPE_WINDOW_H
