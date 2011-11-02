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

#include <Box.h>
#include <String.h>

class BGroupView;

class NotificationWindow;
class NotificationView;

typedef std::vector<NotificationView*> infoview_t;

class AppGroupView : public BBox {
public:
								AppGroupView(NotificationWindow* win, const char* label);
								~AppGroupView();

	virtual	void				MouseDown(BPoint point);
	virtual	void				MessageReceived(BMessage* msg);

			bool				HasChildren();

			void				AddInfo(NotificationView* view);

private:
			void				_ResizeViews();

			BString				fLabel;
			NotificationWindow*	fParent;
			BGroupView*			fView;
			infoview_t			fInfo;
			bool				fCollapsed;
			BRect				fCloseRect;
			BRect				fCollapseRect;
};

#endif	// _APP_GROUP_VIEW_H
