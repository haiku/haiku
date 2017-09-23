/*
 * Copyright 2010-2017, Haiku, Inc. All Rights Reserved.
 * Copyright 2008-2009, Pier Luigi Fiorini. All Rights Reserved.
 * Copyright 2004-2008, Michael Davidson. All Rights Reserved.
 * Copyright 2004-2007, Mikael Eiman. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _APP_GROUP_VIEW_H
#define _APP_GROUP_VIEW_H

#include <vector>

#include <GroupView.h>
#include <Messenger.h>
#include <String.h>

class BGroupView;

class NotificationWindow;
class NotificationView;

typedef std::vector<NotificationView*> infoview_t;

class AppGroupView : public BGroupView {
public:
								AppGroupView(const BMessenger& messenger, const char* label);

	virtual	void				MouseDown(BPoint point);
	virtual	void				MessageReceived(BMessage* msg);
			void				Draw(BRect updateRect);

			bool				HasChildren();
			int32				ChildrenCount();

			void				AddInfo(NotificationView* view);
			void				SetPreviewModeOn(bool enabled);

			const BString&		Group() const;
			void				SetGroup(const char* group);

private:
			void				_DrawCloseButton(const BRect& updateRect);

			BString				fLabel;
			BMessenger			fMessenger;
			infoview_t			fInfo;
			bool				fCollapsed;
			BRect				fCloseRect;
			BRect				fCollapseRect;
			float				fHeaderSize;
			bool				fCloseClicked;
			bool				fPreviewModeOn;
};

#endif	// _APP_GROUP_VIEW_H
