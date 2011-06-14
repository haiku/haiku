/*
 * Copyright 2011, Rene Gollent, rene@gollent.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef INSPECTOR_WINDOW_H
#define INSPECTOR_WINDOW_H


#include <Window.h>

#include "TeamMemoryBlock.h"
#include "Types.h"


class BButton;
class BTextControl;
class MemoryView;
class UserInterfaceListener;


class InspectorWindow : public BWindow,
	public TeamMemoryBlock::Listener {
public:
								InspectorWindow(
									UserInterfaceListener* listener);
	virtual						~InspectorWindow();

	static	InspectorWindow*	Create(UserInterfaceListener* listener);
										// throws

	virtual void				MessageReceived(BMessage* message);
	virtual bool				QuitRequested();

	virtual void				MemoryBlockRetrieved(TeamMemoryBlock* block);

private:
	void						_Init();

private:
	UserInterfaceListener*		fListener;
	BTextControl*				fAddressInput;
	MemoryView*					fMemoryView;
	TeamMemoryBlock*			fCurrentBlock;
	target_addr_t				fCurrentAddress;
};

#endif // INSPECTOR_WINDOW_H
