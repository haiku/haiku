/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef BREAKPOINT_EDIT_WINDOW_H
#define BREAKPOINT_EDIT_WINDOW_H


#include <Window.h>

#include "Team.h"

#include "types/Types.h"


class BButton;
class BRadioButton;
class BTextControl;
class Team;
class UserBreakpoint;
class UserInterfaceListener;


class BreakpointEditWindow : public BWindow {
public:
								BreakpointEditWindow(
									::Team* team,
									UserBreakpoint* breakpoint,
									UserInterfaceListener* listener,
									BHandler* target);

								~BreakpointEditWindow();

	static	BreakpointEditWindow* Create(::Team* team,
									UserBreakpoint* breakpoint,
									UserInterfaceListener* listener,
									BHandler* target);
									// throws

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				Show();

private:
			void	 			_Init();
			void				_UpdateState();

private:
			::Team*				fTeam;
			UserInterfaceListener* fListener;
			UserBreakpoint* 	fTargetBreakpoint;
			BTextControl*		fConditionInput;
			BButton*			fSaveButton;
			BButton*			fCancelButton;
			BRadioButton*		fAlwaysRadio;
			BRadioButton*		fConditionRadio;
			BHandler*			fTarget;
};


#endif // BREAKPOINT_EDIT_WINDOW
