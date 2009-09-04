/*
 * Copyright 2001-2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (burton666@libero.it)
 */
#ifndef MENU_WINDOW_H
#define MENU_WINDOW_H


#include <Window.h>

class BMenu;


namespace BPrivate {

class BMenuFrame;
class BMenuScroller;


class BMenuWindow : public BWindow {
public:
							BMenuWindow(const char* name);
	virtual					~BMenuWindow();

	virtual	void			DispatchMessage(BMessage* message,
								BHandler* handler);

			void			AttachMenu(BMenu* menu);
			void			DetachMenu();

			void			AttachScrollers();
			void			DetachScrollers();

			void			SetSmallStep(float step);
			void			GetSteps(float* _smallStep, float* _largeStep) const;
			bool			HasScrollers() const;
			bool			CheckForScrolling(const BPoint& cursor);
			bool			TryScrollBy(const float& step);

private:
			bool			_Scroll(const BPoint& cursor);
			void			_ScrollBy(const float& step);

			BMenu*			fMenu;
			BMenuFrame*		fMenuFrame;
			BMenuScroller*	fUpperScroller;
			BMenuScroller*	fLowerScroller;

			float			fScrollStep;
			float			fValue;
			float			fLimit;
};

}	// namespace BPrivate

#endif	// MENU_WINDOW_H
