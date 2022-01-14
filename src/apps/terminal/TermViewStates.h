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


#include "HyperLink.h"
#include "TerminalCharClassifier.h"
#include "TermView.h"


class TermView::State {
public:
								State(TermView* view);
	virtual						~State();

	virtual	void				Entered();
	virtual	void				Exited();

	virtual	bool				MessageReceived(BMessage* message);
									// returns true, if handled

	virtual	void				ModifiersChanged(int32 oldModifiers,
									int32 modifiers);
	virtual void				KeyDown(const char* bytes, int32 numBytes);

	virtual void				MouseDown(BPoint where, int32 buttons,
									int32 modifiers);
	virtual void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage, int32 modifiers);
	virtual void				MouseUp(BPoint where, int32 buttons);

	virtual	void				WindowActivated(bool active);

	virtual	void				VisibleTextBufferChanged();

protected:
			TermView*			fView;
};


class TermView::StandardBaseState : public TermView::State {
public:
								StandardBaseState(TermView* view);

protected:
			bool				_StandardMouseMoved(BPoint where,
									int32 modifiers);
};


class TermView::DefaultState : public TermView::StandardBaseState {
public:
								DefaultState(TermView* view);

	virtual	void				ModifiersChanged(int32 oldModifiers,
									int32 modifiers);

	virtual void				KeyDown(const char* bytes, int32 numBytes);

	virtual void				MouseDown(BPoint where, int32 buttons,
									int32 modifiers);
	virtual void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage, int32 modifiers);
	virtual void				MouseUp(BPoint where, int32 buttons);

	virtual	void				WindowActivated(bool active);

private:
			bool				_CheckEnterHyperLinkState(int32 modifiers);
};


class TermView::SelectState : public TermView::StandardBaseState {
public:
								SelectState(TermView* view);

			void				Prepare(BPoint where, int32 modifiers);

	virtual	bool				MessageReceived(BMessage* message);

	virtual void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage, int32 modifiers);
	virtual void				MouseUp(BPoint where, int32 buttons);

private:
			void				_AutoScrollUpdate();

private:
			int32				fSelectGranularity;
			bool				fCheckMouseTracking;
			bool				fMouseTracking;
};


class TermView::HyperLinkState : public TermView::State,
	private TermViewHighlighter {
public:
								HyperLinkState(TermView* view);

	virtual	void				Entered();
	virtual	void				Exited();

	virtual	void				ModifiersChanged(int32 oldModifiers,
									int32 modifiers);

	virtual void				MouseDown(BPoint where, int32 buttons,
									int32 modifiers);
	virtual void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage, int32 modifiers);

	virtual	void				WindowActivated(bool active);

	virtual	void				VisibleTextBufferChanged();

private:
	// TermViewHighlighter
	virtual	rgb_color			ForegroundColor();
	virtual	rgb_color			BackgroundColor();
	virtual	uint32				AdjustTextAttributes(uint32 attributes);

private:
			struct CharPosition {
				int32	index;
				TermPos	position;
			};

private:
			bool				_GetHyperLinkAt(BPoint where,
									bool pathPrefixOnly, HyperLink& _link,
									TermPos& _start, TermPos& _end);
			bool				_EntryExists(const BString& path,
									BString& _actualPath) const;

			void				_UpdateHighlight();
			void				_UpdateHighlight(BPoint where, int32 modifiers);
			void				_ActivateHighlight(const TermPos& start,
									const TermPos& end);
			void				_DeactivateHighlight();

private:
			DefaultCharClassifier fURLCharClassifier;
			DefaultCharClassifier fPathComponentCharClassifier;
			BString				fCurrentDirectory;
			TermViewHighlight	fHighlight;
			bool				fHighlightActive;
};


class TermView::HyperLinkMenuState : public TermView::State {
public:
								HyperLinkMenuState(TermView* view);

			void				Prepare(BPoint point, const HyperLink& link);

	virtual	void				Exited();

	virtual	bool				MessageReceived(BMessage* message);

private:
			class PopUpMenu;

private:
			HyperLink			fLink;
};


#endif // TERMVIEW_STATES_H
