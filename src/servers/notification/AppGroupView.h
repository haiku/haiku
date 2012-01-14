/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2008-2009, Pier Luigi Fiorini. All Rights Reserved.
 * Copyright 2004-2008, Michael Davidson. All Rights Reserved.
 * Copyright 2004-2007, Mikael Eiman. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _APP_GROUP_VIEW_H
#define _APP_GROUP_VIEW_H

#include <vector>

#include <GroupView.h>
#include <String.h>

class BGroupView;

class NotificationWindow;
class NotificationView;

typedef std::vector<NotificationView*> infoview_t;

class AppGroupView : public BGroupView {
public:
								AppGroupView(NotificationWindow* win, const char* label);

	virtual	void				MouseDown(BPoint point);
	virtual	void				MessageReceived(BMessage* msg);
			void				Draw(BRect updateRect);

			bool				HasChildren();

			void				AddInfo(NotificationView* view);

			const BString&		Group() const;

private:
			BString				fLabel;
			NotificationWindow*	fParent;
			infoview_t			fInfo;
			bool				fCollapsed;
			BRect				fCloseRect;
			BRect				fCollapseRect;
};

#endif	// _APP_GROUP_VIEW_H
