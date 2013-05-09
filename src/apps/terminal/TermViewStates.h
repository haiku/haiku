/*
 * Copyright 2001-2013, Haiku, Inc.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai.
 *
 * Distributed under the terms of the MIT license.
 * Authors:
 *		Stefano Ceccherini, stefano.ceccherini@gmail.com
 *		Kian Duffy, myob@users.sourceforge.net
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 *		Siarzhuk Zharski, zharik@gmx.li
 */
#ifndef TERMVIEW_STATES_H
#define TERMVIEW_STATES_H


#include "TermView.h"


class TermView::State {
public:
								State(TermView* view);
	virtual						~State();

	virtual	void				Entered();
	virtual	void				Exited();

	virtual	bool				MessageReceived(BMessage* message);
									// returns true, if handled

	virtual void				KeyDown(const char* bytes, int32 numBytes);

	virtual void				MouseDown(BPoint where, int32 buttons,
									int32 modifiers);
	virtual void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* message);
	virtual void				MouseUp(BPoint where, int32 buttons);

protected:
			TermView*			fView;
};


class TermView::StandardBaseState : public TermView::State {
public:
								StandardBaseState(TermView* view);

protected:
			bool				_StandardMouseMoved(BPoint where);
};


class TermView::DefaultState : public TermView::StandardBaseState {
public:
								DefaultState(TermView* view);

	virtual void				KeyDown(const char* bytes, int32 numBytes);

	virtual void				MouseDown(BPoint where, int32 buttons,
									int32 modifiers);
	virtual void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* message);
};


class TermView::SelectState : public TermView::StandardBaseState {
public:
								SelectState(TermView* view);

			void				Prepare(BPoint where, int32 modifiers);

	virtual	bool				MessageReceived(BMessage* message);

	virtual void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* message);
	virtual void				MouseUp(BPoint where, int32 buttons);

private:
			void				_AutoScrollUpdate();

private:
			int32				fSelectGranularity;
			bool				fCheckMouseTracking;
			bool				fMouseTracking;
};


#endif // TERMVIEW_STATES_H
