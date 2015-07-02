/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef SIGNAL_DISPOSITION_EDIT_WINDOW_H
#define SIGNAL_DISPOSITION_EDIT_WINDOW_H


#include <Window.h>

#include "Team.h"

#include "types/Types.h"


class BButton;
class BMenu;
class BMenuField;
class Team;
class UserInterfaceListener;


class SignalDispositionEditWindow : public BWindow {
public:
								SignalDispositionEditWindow(
									::Team* team,
									int32 signal,
									UserInterfaceListener* listener,
									BHandler* target);

								~SignalDispositionEditWindow();

	static	SignalDispositionEditWindow* Create(::Team* team,
									int32 signal,
									UserInterfaceListener* listener,
									BHandler* target);
									// throws

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				Show();

private:
			void	 			_Init();
			BMenu*				_BuildSignalSelectionMenu();
			void				_UpdateState();

private:
			::Team*				fTeam;
			UserInterfaceListener* fListener;
			bool				fEditMode;
			int32				fCurrentSignal;
			int32				fCurrentDisposition;
			BButton*			fSaveButton;
			BButton*			fCancelButton;
			BMenuField*			fSignalSelectionField;
			BMenuField*			fDispositionSelectionField;
			BHandler*			fTarget;
};


#endif // SIGNAL_DISPOSITION_EDIT_WINDOW
