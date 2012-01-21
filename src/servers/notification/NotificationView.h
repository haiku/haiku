/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2008-2009, Pier Luigi Fiorini. All Rights Reserved.
 * Copyright 2004-2008, Michael Davidson. All Rights Reserved.
 * Copyright 2004-2007, Mikael Eiman. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NOTIFICATION_VIEW_H
#define _NOTIFICATION_VIEW_H

#include <list>

#include <String.h>
#include <View.h>


class BBitmap;
class BMessageRunner;
class BNotification;

class NotificationWindow;

const uint32 kRemoveView = 'ReVi';


class NotificationView : public BView {
public:
								NotificationView(NotificationWindow* win,
									BNotification* notification,
									bigtime_t timeout = -1);
	virtual						~NotificationView();

	virtual	void 				AttachedToWindow();
	virtual	void 				MessageReceived(BMessage* message);
	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint point);

/*
	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize 				PreferredSize();
*/

	virtual	BHandler*			ResolveSpecifier(BMessage* msg, int32 index,
									BMessage* specifier, int32 form,
									const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* msg);

			void 				SetText(float newMaxWidth = -1);

			const char*			MessageID() const;

private:
			BSize				_CalculateSize();

			struct LineInfo {
				BFont	font;
				BString	text;
				BPoint	location;
			};

			typedef std::list<LineInfo*> LineInfoList;

			NotificationWindow*	fParent;
			BNotification*		fNotification;
			bigtime_t			fTimeout;

			BMessageRunner*		fRunner;

			BBitmap*			fBitmap;

			LineInfoList		fLines;

			float				fHeight;
};

#endif	// _NOTIFICATION_VIEW_H
