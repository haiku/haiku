/*
 * Copyright 2005-2008, Mikael Eiman.
 * Copyright 2005-2008, Michael Davidson.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *              Mikael Eiman <mikael@eiman.tv>
 *              Michael Davidson <slaad@bong.com.au>
 */

#ifndef APPGROUPVIEW_H
#define APPGROUPVIEW_H

#include <vector>

#include <String.h>
#include <View.h>

class NotificationWindow;
class NotificationView;

typedef std::vector<NotificationView *> infoview_t;

class AppGroupView : public BView {
	public:
							AppGroupView(NotificationWindow *win, const char *label);
							~AppGroupView(void);
	
		// Hooks
		void				AttachedToWindow(void);
		void				Draw(BRect bounds);
		void				MouseDown(BPoint point);
		void				GetPreferredSize(float *width, float *height);
		void				MessageReceived(BMessage *msg);
 	
		// Public
		void				AddInfo(NotificationView *view);
		void				ResizeViews(void);
		bool				HasChildren(void);
	
	private:
		BString				fLabel;
		NotificationWindow			*fParent;
		infoview_t			fInfo;
		bool				fCollapsed;
		BRect				fCloseRect;
		BRect				fCollapseRect;
};

#endif
