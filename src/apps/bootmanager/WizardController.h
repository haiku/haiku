/*
 * Copyright 2008-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */
#ifndef WIZARD_CONTROLLER_H
#define WIZARD_CONTROLLER_H


#include <SupportDefs.h>


class WizardView;
class WizardPageView;


class WizardController {
public:
								WizardController();
	virtual						~WizardController();

	virtual	void				Initialize(WizardView* wizard);
	virtual	void				Next(WizardView* wizard);
	virtual	void				Previous(WizardView* wizard);

protected:
	virtual	int32				InitialState() = 0;
	virtual	int32				NextState(int32 state) = 0;
	virtual	WizardPageView*		CreatePage(int32 state, WizardView* wizard) = 0;

			int32				CurrentState() const;

private:
	class StateStack {
	public:
		StateStack(int32 state, StateStack* next)
			:
			fState(state),
			fNext(next)
		{
		}

		int32 State()
		{
			return fState;
		}

		StateStack* Next()
		{
			return fNext;
		}

		void MakeEmpty();

	private:
		int32 fState;
		StateStack* fNext;
	};

			void				_PushState(int32 state);
			void				_ShowPage(WizardView* wizard);

private:
			StateStack*			fStack;
};


#endif	// WIZARD_CONTROLLER_H
