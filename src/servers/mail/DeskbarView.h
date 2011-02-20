/* DeskbarView - mail_daemon's deskbar menu and view
 *
 * Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
 */
#ifndef DESKBAR_VIEW_H
#define DESKBAR_VIEW_H


#include <View.h>

#include "NavMenu.h"


enum {
	kStatusNoMail = 0,
	kStatusNewMail,
	kStatusCount
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
						DeskbarView(BRect frame);
						DeskbarView(BMessage* data);
	virtual				~DeskbarView();

	virtual void		Draw(BRect updateRect);
	virtual void		AttachedToWindow();
	static DeskbarView*	Instantiate(BMessage* data);
	virtual	status_t	Archive(BMessage* data, bool deep = true) const;
	virtual void	 	MouseDown(BPoint);
	virtual void	 	MouseUp(BPoint);
	virtual void		MessageReceived(BMessage* message);
	virtual void		Pulse();

private:
	bool				_EntryInTrash(const entry_ref*);
	void				_RefreshMailQuery();
	bool				_CreateMenuLinks(BDirectory&, BPath&);
	void				_CreateNewMailQuery(BEntry&);
	BPopUpMenu*			_BuildMenu();
	void				_InitBitmaps();
	status_t			_GetNewQueryRef(entry_ref& ref);

	BBitmap*			fBitmaps[kStatusCount];
	int32				fStatus;

	BList				fNewMailQueries;
	int32				fNewMessages;

	int32				fLastButtons;
};

#endif	/* DESKBAR_VIEW_H */
