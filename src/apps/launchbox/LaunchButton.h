/*
 * Copyright 2006-2011, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LAUNCH_BUTTON_H
#define LAUNCH_BUTTON_H


#include <IconButton.h>
#include <List.h>
#include <String.h>


enum {
	MSG_ADD_SLOT				= 'adsl',
	MSG_CLEAR_SLOT				= 'clsl',
	MSG_REMOVE_SLOT				= 'rmsl',
	MSG_LAUNCH					= 'lnch',
	MSG_OPEN_CONTAINING_FOLDER		= 'opcf',
};


class LaunchButton : public BIconButton {
public:
								LaunchButton(const char* name,
									const char* label = NULL,
									BMessage* message = NULL,
									BHandler* target = NULL);
	virtual						~LaunchButton();

	// BIconButton interface
	virtual	void				AttachedToWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);

	virtual	BSize				MinSize();
	virtual	BSize				PreferredSize();
	virtual	BSize				MaxSize();

	// LaunchButton
			void				SetTo(const entry_ref* ref);
			entry_ref*			Ref() const;

			void				SetTo(const char* appSig, bool updateIcon);
			const char*			AppSignature() const
									{ return fAppSig; }

			void				SetDescription(const char* text);
			const char*			Description() const
									{ return fDescription.String(); }

			void				SetIconSize(uint32 size);
			uint32				IconSize() const
									{ return fIconSize; }

	static	void				SetIgnoreDoubleClick(bool refuse);
	static	bool				IgnoreDoubleClick()
									{ return sIgnoreDoubleClick; }

private:
			void				_UpdateToolTip();
			void				_UpdateIcon(const entry_ref* ref);

			void				_NotifySettingsChanged();

			void				_DrawFrame(BRect frame,
									rgb_color left, rgb_color top,
									rgb_color right, rgb_color bottom);

private:
			entry_ref*			fRef;
			char*				fAppSig;
			BString				fDescription;

			bool				fAnticipatingDrop;
			bigtime_t			fLastClickTime;
			BPoint				fDragStart;

			uint32				fIconSize;

	static	bigtime_t			sClickSpeed;
	static	bool				sIgnoreDoubleClick;
};

#endif // LAUNCH_BUTTON_H