/*
 * Copyright 2010 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef URL_INPUT_GROUP_H
#define URL_INPUT_GROUP_H

#include <GroupView.h>
#include <MessageRunner.h>

class BButton;
class BTextView;


class URLInputGroup : public BGroupView {
private:
	static	const	uint32		MSG_LOCK_TIMEOUT = 'loti';
	static	const	bigtime_t	LOCK_TIMEOUT = 1000000;
		// Lock will timeout in one second

public:
								URLInputGroup(BMessage* goMessage);
	virtual						~URLInputGroup();

	virtual	void				AttachedToWindow();
	virtual	void				WindowActivated(bool active);
	virtual	void				Draw(BRect updateRect);
	virtual	void				MakeFocus(bool focus = true);

			BTextView*			TextView() const;
			void				SetText(const char* text);
			const char*			Text() const;

			BButton*			GoButton() const;

			void				SetPageIcon(const BBitmap* icon);

	virtual	void				LockURLInput(bool lock = true);
	virtual	void				MessageReceived(BMessage* message);

private:
			class PageIconView;
			class URLTextView;

			BMessageRunner*		fURLLockTimeout;
			PageIconView*		fIconView;
			URLTextView*		fTextView;
			BButton*			fGoButton;
			bool				fWindowActive;
			bool				fURLLocked;
};

#endif // URL_INPUT_GROUP_H

