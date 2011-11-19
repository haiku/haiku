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
class BMessenger;
class BTextControl;
class GUITeamUISettings;
class MemoryView;
class Team;
class UserInterfaceListener;


class InspectorWindow : public BWindow,
	public TeamMemoryBlock::Listener {
public:
								InspectorWindow(::Team* team,
									UserInterfaceListener* listener,
									BHandler* target);
	virtual						~InspectorWindow();

	static	InspectorWindow*	Create(::Team* team,
									UserInterfaceListener* listener,
									BHandler* target);
										// throws

	virtual void				MessageReceived(BMessage* message);
	virtual bool				QuitRequested();

	virtual void				MemoryBlockRetrieved(TeamMemoryBlock* block);

			status_t			LoadSettings(
									const GUITeamUISettings& settings);
			status_t			SaveSettings(
									BMessage& settings);
private:
	void						_Init();

	void						_LoadMenuFieldMode(BMenuField* field,
									const char* name,
									const BMessage& settings);
	status_t					_SaveMenuFieldMode(BMenuField* field,
									const char* name,
									BMessage& settings);

private:
	UserInterfaceListener*		fListener;
	BTextControl*				fAddressInput;
	BMenuField*					fHexMode;
	BMenuField*					fEndianMode;
	BMenuField*					fTextMode;
	MemoryView*					fMemoryView;
	BButton*					fPreviousBlockButton;
	BButton*					fNextBlockButton;
	TeamMemoryBlock*			fCurrentBlock;
	target_addr_t				fCurrentAddress;
	::Team*						fTeam;
	BHandler*					fTarget;
};

#endif // INSPECTOR_WINDOW_H
