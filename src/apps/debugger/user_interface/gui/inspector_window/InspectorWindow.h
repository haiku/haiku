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
class BMenuField;
class BTextControl;
class MemoryView;
class Team;
class UserInterfaceListener;


class InspectorWindow : public BWindow,
	public TeamMemoryBlock::Listener {
public:
								InspectorWindow(::Team* team,
									UserInterfaceListener* listener);
	virtual						~InspectorWindow();

	static	InspectorWindow*	Create(::Team* team,
									UserInterfaceListener* listener);
										// throws

	virtual void				MessageReceived(BMessage* message);
	virtual bool				QuitRequested();

	virtual void				MemoryBlockRetrieved(TeamMemoryBlock* block);

private:
	void						_Init();

private:
	UserInterfaceListener*		fListener;
	BTextControl*				fAddressInput;
	BMenuField*					fHexMode;
	BMenuField*					fTextMode;
	MemoryView*					fMemoryView;
	TeamMemoryBlock*			fCurrentBlock;
	target_addr_t				fCurrentAddress;
	::Team*						fTeam;
};

#endif // INSPECTOR_WINDOW_H
