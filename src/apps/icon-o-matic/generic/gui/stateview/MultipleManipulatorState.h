/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef MULTIPLE_MANIPULATOR_STATE_H
#define MULTIPLE_MANIPULATOR_STATE_H

#include <List.h>

#include "ViewState.h"

class Manipulator;

class MultipleManipulatorState : public ViewState {
 public:
								MultipleManipulatorState(StateView* view);
	virtual						~MultipleManipulatorState();

	// ViewState interface
	virtual	void				Init();
	virtual	void				Cleanup();

	virtual	void				Draw(BView* into, BRect updateRect);
	virtual	bool				MessageReceived(BMessage* message,
												Command** _command);

	virtual	void				MouseDown(BPoint where,
										  uint32 buttons,
										  uint32 clicks);

	virtual	void				MouseMoved(BPoint where,
										   uint32 transit,
										   const BMessage* dragMessage);
	virtual	Command*			MouseUp();

	virtual	void				ModifiersChanged(uint32 modifiers);

	virtual	bool				HandleKeyDown(uint32 key, uint32 modifiers,
											  Command** _command);
	virtual	bool				HandleKeyUp(uint32 key, uint32 modifiers,
											Command** _command);

	virtual	bool				UpdateCursor();

	// MultipleManipulatorState
			bool				AddManipulator(Manipulator* manipulator);
			Manipulator*		RemoveManipulator(int32 index);
			void				DeleteManipulators();

			int32				CountManipulators() const;
			Manipulator*		ManipulatorAt(int32 index) const;
			Manipulator*		ManipulatorAtFast(int32 index) const;

 private:
			void				_UpdateCursor();
			void				_ShowContextMenu(BPoint where);


			BList				fManipulators;
			Manipulator*		fCurrentManipulator;
			Manipulator*		fPreviousManipulator;
};

#endif // MULTIPLE_MANIPULATOR_STATE_H
