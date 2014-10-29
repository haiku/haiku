/*
 * Copyright 2011-2014, Rene Gollent, rene@gollent.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef INSPECTOR_WINDOW_H
#define INSPECTOR_WINDOW_H


#include <Window.h>

#include "MemoryView.h"
#include "Team.h"
#include "TeamMemoryBlock.h"
#include "Types.h"


class BButton;
class BMenuField;
class BMessenger;
class BTextControl;
class GuiTeamUiSettings;
class SourceLanguage;
class UserInterfaceListener;


class InspectorWindow : public BWindow,
	public TeamMemoryBlock::Listener,
	public MemoryView::Listener,
	private Team::Listener {
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

	// TeamMemoryBlock::Listener
	virtual void				MemoryBlockRetrieved(TeamMemoryBlock* block);
	virtual void				MemoryBlockRetrievalFailed(
									TeamMemoryBlock* block, status_t result);

	// MemoryView::Listener
	virtual	void				TargetAddressChanged(target_addr_t address);

	// Team::Listener
	virtual	void				ExpressionEvaluated(
									const Team::ExpressionEvaluationEvent&
										event);

			status_t			LoadSettings(
									const GuiTeamUiSettings& settings);
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

	void						_SetToAddress(target_addr_t address);

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
	SourceLanguage*				fLanguage;
	BHandler*					fTarget;
};

#endif // INSPECTOR_WINDOW_H
