/**
 * @author Gregor Rosenauer <gregor.rosenauer@gmail.com>
 * All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _OPEN_RELATION_TARGETS_WINDOW_H
#define _OPEN_RELATION_TARGETS_WINDOW_H

#include <String.h>

#include "Sen.h"
#include "SlowMenu.h"

class OpenRelationTargetsMenu : public BSlowMenu {
public:
	OpenRelationTargetsMenu(const char* label, const BMessage* entriesToOpen,
		BWindow* parentWindow, BHandler* target);
	OpenRelationTargetsMenu(const char* label, const BMessage* entriesToOpen,
		BWindow* parentWindow, const BMessenger &target);

private:
	virtual bool StartBuildingItemList();
	virtual bool AddNextItem();
	virtual void DoneBuildingItemList();
	virtual void ClearMenuBuildingState();

	BMessage fEntriesToOpen;
	BHandler* target;
	BMessenger fMessenger;
	BWindow* fParentWindow;
    
    BMessenger  fSenMessenger;
    BMessage    fRelationTargetsReply;

	typedef BSlowMenu _inherited;
};

#endif	// _OPEN_RELATION_TARGETS_WINDOW_H
