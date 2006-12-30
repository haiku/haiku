#ifndef DESKBAR_VIEW_H
#define DESKBAR_VIEW_H
/* DeskbarView - main_daemon's deskbar menu and view
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/

#include <View.h>

#include "NavMenu.h"


enum MDDeskbarIcon {
	NO_MAIL = 0,
	NEW_MAIL,
};

enum MDDeskbarMessages {
	MD_CHECK_SEND_NOW = 'MDra',
	MD_CHECK_FOR_MAILS,
	MD_SEND_MAILS,
	MD_OPEN_NEW,
	MD_OPEN_PREFS,
	MD_REFRESH_QUERY
};

class BPopUpMenu;
class BQuery;
class BDirectory;
class BEntry;
class BPath;

class _EXPORT DeskbarView : public BView {
	public:
		DeskbarView (BRect frame);
		DeskbarView (BMessage *data);	

		virtual ~DeskbarView();

		virtual void		Draw(BRect updateRect);
		virtual void		AttachedToWindow();
		static DeskbarView *Instantiate(BMessage *data);
		virtual	status_t	Archive(BMessage *data, bool deep = true) const;
		virtual void	 	MouseDown(BPoint);
		virtual void	 	MouseUp(BPoint);
		virtual void		MessageReceived(BMessage *message);
		virtual void		Pulse();

		void				ChangeIcon(int32 icon);

	private:
		void		RefreshMailQuery();
		bool		CreateMenuLinks(BDirectory &,BPath &);
		void		CreateNewMailQuery(BEntry &);
		BPopUpMenu	*BuildMenu();
	
		BBitmap		*fIcon;
		int32		fCurrentIconState;
		
		BList		fNewMailQueries;
		int32		fNewMessages;

		int32		fLastButtons;
};

#endif	/* DESKBAR_VIEW_H */
