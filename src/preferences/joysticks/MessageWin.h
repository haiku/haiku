/*
 * Copyright 2007 Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *		Ryan Leavengood, leavengood@gmail.com
 */
#ifndef _MESSAGE_WIN_H
#define _MESSAGE_WIN_H


#include <Window.h>

class BBox;
class BButton;
class BCheckBox;
class BStringView;
class BView;
class BTextView;


class MessageWin : public BWindow
{
	public:
		MessageWin(BRect parent_frame, const char *title,
			window_look look,
			window_feel feel,
			uint32 flags,
			uint32 workspace = B_CURRENT_WORKSPACE);

		void			SetText(const char* str);
		virtual	void	MessageReceived(BMessage *message);
		virtual	bool	QuitRequested();

	protected:
		BBox*			fBox;
		BTextView*	 	fText;
};

#endif	/* _MESSAGE_WIN_H */

