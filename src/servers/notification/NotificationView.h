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

#include <Bitmap.h>
#include <Entry.h>
#include <Message.h>
#include <MessageRunner.h>
#include <MimeType.h>
#include <Notification.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>
#include <TextView.h>
#include <View.h>

class NotificationWindow;

const uint32 kRemoveView = 'ReVi';

class NotificationView : public BView {
public:
								NotificationView(NotificationWindow* win,
									notification_type type,
									const char* app, const char* title, 
									const char* text, BMessage* details);
	virtual						~NotificationView();

	virtual	void 				AttachedToWindow();
	virtual	void 				MessageReceived(BMessage* message);
	virtual	void 				GetPreferredSize(float* width, float* height);
	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint point);
	virtual	void				FrameResized(float width, float height);

	virtual	BHandler*			ResolveSpecifier(BMessage* msg, int32 index,
									BMessage* specifier, int32 form,
									const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* msg);

			const char*			Application() const;
			const char*			Title() const;
			const char*			Text() const;

			void 				SetText(const char* app, const char* title,
									const char* text, float newMaxWidth = -1);
			bool				HasMessageID(const char* id);
			const char*			MessageID();
			void				SetPosition(bool first, bool last);

private:
			BBitmap*			_ReadNodeIcon(const char* fileName,
									icon_size size);
			void				_LoadIcon();

private:
			struct LineInfo {
				BFont	font;
				BString	text;
				BPoint	location;
			};

			typedef std::list<LineInfo*> LineInfoList;


			NotificationWindow*	fParent;

			notification_type	fType;
			BMessageRunner*		fRunner;
			float				fProgress;
			BString				fMessageID;

			BMessage*			fDetails;
			BBitmap*			fBitmap;

			LineInfoList		fLines;

			BString				fApp;
			BString				fTitle;
			BString				fText;

			float				fHeight;

			bool				fIsFirst;
			bool				fIsLast;
};

#endif	// _NOTIFICATION_VIEW_H
