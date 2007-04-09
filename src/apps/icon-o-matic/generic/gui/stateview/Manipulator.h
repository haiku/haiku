/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef MANIPULATOR_H
#define MANIPULATOR_H

#include <Rect.h>

#include "Observer.h"

class BMessage;
class BView;
class Command;

// TODO: merge ViewState and Manipulator

class Manipulator : public Observer {
 public:
								Manipulator(Observable* object);
	virtual						~Manipulator();

	// Manipulator interface
	virtual	void				Draw(BView* into, BRect updateRect);

	virtual	bool				MouseDown(BPoint where);
	virtual	void				MouseMoved(BPoint where);
	virtual	Command*			MouseUp();
	virtual	bool				MouseOver(BPoint where);
	virtual	bool				DoubleClicked(BPoint where);

	virtual	bool				ShowContextMenu(BPoint where);

	virtual	bool				MessageReceived(BMessage* message,
												Command** _command);

	virtual	void				ModifiersChanged(uint32 modifiers);
	virtual	bool				HandleKeyDown(uint32 key, uint32 modifiers,
											  Command** _command);
	virtual	bool				HandleKeyUp(uint32 key, uint32 modifiers,
											Command** _command);

	virtual	bool				UpdateCursor();

	virtual	BRect				Bounds() = 0;
		// the area that the manipulator is
		// occupying in the "parent" view
	virtual	BRect				TrackingBounds(BView* withinView);
		// the area within "view" in which the
		// Manipulator wants to receive MouseOver()
		// events

	virtual	void				AttachedToView(BView* view);
	virtual	void				DetachedFromView(BView* view);

 protected:
			Observable*			fManipulatedObject;
};

#endif // MANIPULATOR_H
