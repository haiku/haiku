/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef WATCH_PROMPT_WINDOW_H
#define WATCH_PROMPT_WINDOW_H


#include <Window.h>

#include "types/Types.h"


class BTextControl;
class Watchpoint;
class BMenuField;
class UserInterfaceListener;


class WatchPromptWindow : public BWindow
{
public:
			// edit existing watchpoint
								WatchPromptWindow(target_addr_t address,
									uint32 type, int32 length,
									UserInterfaceListener* listener);

								~WatchPromptWindow();

	static	WatchPromptWindow*	Create(target_addr_t address, uint32 type,
									int32 length,
									UserInterfaceListener* listener);
									// throws


	virtual	void				MessageReceived(BMessage* message);

	virtual	void				Show();

private:
			void	 			_Init();


private:
			target_addr_t		fInitialAddress;
			uint32				fInitialType;
			int32				fInitialLength;
			BTextControl*		fAddressInput;
			BTextControl*		fLengthInput;
			BMenuField*			fTypeField;
			UserInterfaceListener* fListener;
			BButton*			fWatchButton;
			BButton*			fCancelButton;
};

#endif // WATCH_PROMPT_WINDOW_H
