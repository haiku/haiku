/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef VIEW_STATE_H
#define VIEW_STATE_H

#include <View.h>

class BMessage;
class Command;
class StateView;

struct mouse_info {
	mouse_info();

	uint32				buttons;
	BPoint				position;
	uint32				transit;
	uint32				modifiers;
};

class ViewState {
 public:
								ViewState(StateView* view);
								ViewState(const ViewState& other);
	virtual						~ViewState();

	// ViewState interface
	virtual	void				Init();
	virtual	void				Cleanup();

	virtual	void				Draw(BView* into, BRect updateRect);
	virtual	bool				MessageReceived(BMessage* message,
												Command** _command);

	// mouse tracking
	virtual	void				MouseDown(BPoint where,
										  uint32 buttons,
										  uint32 clicks);

	virtual	void				MouseMoved(BPoint where,
										   uint32 transit,
										   const BMessage* dragMessage);
	virtual	Command*			MouseUp();

	// modifiers
	virtual	void				ModifiersChanged(uint32 modifiers);

	// TODO: mouse wheel
	virtual	bool				HandleKeyDown(uint32 key, uint32 modifiers,
											  Command** _command);
	virtual	bool				HandleKeyUp(uint32 key, uint32 modifiers,
											Command** _command);

	virtual	bool				UpdateCursor();


	inline	uint32				PressedMouseButtons() const
									{ return fMouseInfo->buttons; }

	inline	bool				IsFirstButtonDown() const
									{ return fMouseInfo->buttons & B_PRIMARY_MOUSE_BUTTON; }
	inline	bool				IsSecondButtonDown() const
									{ return fMouseInfo->buttons & B_SECONDARY_MOUSE_BUTTON; }
	inline	bool				IsThirdButtonDown() const
									{ return fMouseInfo->buttons & B_TERTIARY_MOUSE_BUTTON; }

	inline	BPoint				MousePos() const
									{ return fMouseInfo->position; }

	inline	uint32				Modifiers() const
									{ return fMouseInfo->modifiers; }

 protected:
			StateView*			fView;

			// NOTE: the intention of using a pointer
			// to a mouse_info struct is that all
			// ViewStates belonging to the same StateView
			// should have the same pointer, so that
			// they will all be up to date with the same info
			const mouse_info*	fMouseInfo;
};

#endif // VIEW_STATE_H
