/*
 * Copyright 2011-2015, Rene Gollent, rene@gollent.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef INSPECTOR_WINDOW_H
#define INSPECTOR_WINDOW_H


#include <Window.h>

#include "ExpressionInfo.h"
#include "MemoryView.h"
#include "Team.h"
#include "TeamMemoryBlock.h"
#include "Types.h"


class BButton;
class BMenuField;
class BMessenger;
class BStringView;
class BTextControl;
class GuiTeamUiSettings;
class SourceLanguage;
class UserInterfaceListener;


class InspectorWindow : public BWindow, private Team::Listener,
	private TeamMemoryBlock::Listener,	private MemoryView::Listener,
	private ExpressionInfo::Listener {
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

	// Team::Listener
	virtual	void				ThreadStateChanged(
									const Team::ThreadEvent& event);

	virtual	void				MemoryChanged(
									const Team::MemoryChangedEvent& event);

	// TeamMemoryBlock::Listener
	virtual void				MemoryBlockRetrieved(TeamMemoryBlock* block);
	virtual void				MemoryBlockRetrievalFailed(
									TeamMemoryBlock* block, status_t result);

	// MemoryView::Listener
	virtual	void				TargetAddressChanged(target_addr_t address);
	virtual	void				HexModeChanged(int32 newMode);
	virtual	void				EndianModeChanged(int32 newMode);
	virtual	void				TextModeChanged(int32 newMode);

	// ExpressionInfo::Listener
	virtual	void				ExpressionEvaluated(ExpressionInfo* info,
									status_t result, ExpressionResult* value);

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
	void						_SetCurrentBlock(TeamMemoryBlock* block);

	void						_SetEditMode(bool enabled);

	bool						_GetWritableState() const;

	void						_UpdateWritableOptions();

	void						_UpdateWritableIndicator();
	const char*					_GetCurrentWritableIndicator() const;

private:
	UserInterfaceListener*		fListener;
	BTextControl*				fAddressInput;
	BMenuField*					fHexMode;
	BMenuField*					fEndianMode;
	BMenuField*					fTextMode;
	BStringView*				fWritableBlockIndicator;
	MemoryView*					fMemoryView;
	BButton*					fPreviousBlockButton;
	BButton*					fNextBlockButton;
	BButton*					fEditBlockButton;
	BButton*					fCommitBlockButton;
	BButton*					fRevertBlockButton;
	TeamMemoryBlock*			fCurrentBlock;
	target_addr_t				fCurrentAddress;
	::Team*						fTeam;
	SourceLanguage*				fLanguage;
	ExpressionInfo*				fExpressionInfo;
	BHandler*					fTarget;
};

#endif // INSPECTOR_WINDOW_H
