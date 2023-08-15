/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#ifndef STATE_VIEW_H
#define STATE_VIEW_H

#include <View.h>

#include "ViewState.h"

class BMessageFilter;
class Command;
class CommandStack;
class RWLocker;

class StateView : public BView {
 public:
								StateView(BRect frame, const char* name,
										  uint32 resizingMode, uint32 flags);
	virtual						~StateView();

	// BView interface
	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				MessageReceived(BMessage* message);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
										   const BMessage* dragMessage);
	virtual	void				MouseUp(BPoint where);

	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				KeyUp(const char* bytes, int32 numBytes);

	virtual	status_t			Perform(perform_code code, void* data);
		// Avoids warning about hiding BView::Perform().

	virtual	void				GetPreferredSize(float* width, float* height);

	// StateView interface
			void				SetState(ViewState* state);
			void				UpdateStateCursor();

			void				Draw(BView* into, BRect updateRect);

	virtual	bool				MouseWheelChanged(BPoint where,
												  float x, float y);

			bool				HandleKeyDown(uint32 key, uint32 modifiers);
			bool				HandleKeyUp(uint32 key, uint32 modifiers);

			const mouse_info*	MouseInfo() const
									{ return &fMouseInfo; }

	virtual	void				FilterMouse(BPoint* where) const;

	virtual	ViewState*			StateForDragMessage(const BMessage* message);

			void				SetLocker(RWLocker* locker);
			RWLocker*			Locker() const
									{ return fLocker; }

			void				SetCommandStack(::CommandStack* stack);
			::CommandStack*		CommandStack() const
									{ return fCommandStack; }

			void				SetUpdateTarget(BHandler* target,
												uint32 command);

			void				SetCatchAllEvents(bool catchAll);

			status_t			Perform(Command* command);

 protected:
	virtual	bool				_HandleKeyDown(uint32 key, uint32 modifiers);
	virtual	bool				_HandleKeyUp(uint32 key, uint32 modifiers);

			void				_InstallEventFilter();
			void				_RemoveEventFilter();

			void				_TriggerUpdate();

			BRect				fStartingRect;

			ViewState*			fCurrentState;
			ViewState*			fDropAnticipatingState;
				// the drop anticipation state is some
				// kind of "temporary" state that is
				// used on top of the current state (it
				// doesn't replace it)
			mouse_info			fMouseInfo;

			::CommandStack*		fCommandStack;
			RWLocker*			fLocker;

			BMessageFilter*		fEventFilter;
			bool				fCatchAllEvents;

			BHandler*			fUpdateTarget;
			uint32				fUpdateCommand;
};

#endif // STATE_VIEW_H
