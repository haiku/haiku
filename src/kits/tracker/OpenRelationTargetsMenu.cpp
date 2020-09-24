#include "Attributes.h"
#include "AutoLock.h"
#include "Commands.h"
#include "FSUtils.h"
#include "IconMenuItem.h"
#include "OpenRelationTargetsMenu.h"
#include "MimeTypes.h"
#include "StopWatch.h"
#include "Tracker.h"

#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <Debug.h>
#include <GroupView.h>
#include <GridView.h>
#include <Locale.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>
#include <SpaceLayoutItem.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

OpenRelationTargetsMenu::OpenRelationTargetsMenu(const char* label, const BMessage* entriesToOpen,
	BWindow* parentWindow, BHandler* target)
	:
	BSlowMenu(label),
	fEntriesToOpen(*entriesToOpen),
	target(target),
	fParentWindow(parentWindow)
{
	InitIconPreloader();

	SetFont(be_plain_font);

	// too long to have triggers
	SetTriggersEnabled(false);
}


OpenRelationTargetsMenu::OpenRelationTargetsMenu(const char* label, const BMessage* entriesToOpen,
	BWindow* parentWindow, const BMessenger &messenger)
	:
	BSlowMenu(label),
	fEntriesToOpen(*entriesToOpen),
	target(NULL),
	fMessenger(messenger),
	fParentWindow(parentWindow)
{
	InitIconPreloader();

	SetFont(be_plain_font);

	// too long to have triggers - TODO; what does it mean?
	SetTriggersEnabled(false);
}

bool
OpenRelationTargetsMenu::StartBuildingItemList()
{
    fSenMessenger = BMessenger(SEN_SERVER_SIGNATURE);
    
    if (fSenMessenger.IsValid()) {
        
        PRINT(("Tracker->SEN: getting relation targets" UTF8_ELLIPSIS, path.Path()));
        
        fSenMessenger.SendMessage(&fEntriesToOpen, &fRelationTargetsReply);
        
        PRINT(("SEN->Tracker: received reply."));
        
        return true;
    } else {
        PRINT(("failed to reach SEN server, is it running?"));
        return false;
    }
}

bool OpenRelationTargetsMenu::AddNextItem()
{
    // nothing to do here
    return false;
}

void OpenRelationTargetsMenu::ClearMenuBuildingState() 
{
    //  empty;
    return;
}

void
OpenRelationTargetsMenu::DoneBuildingItemList()
{
	// target the menu
	if (target != NULL)
		SetTargetForItems(target);
	else
		SetTargetForItems(fMessenger);

    entry_ref ref;
    BEntry entry;
    BPath path;
    int index = 0;
    
    while (fRelationTargetsReply.FindRef("refs", index, &ref) == B_OK) {
        entry.SetTo(&ref);
        entry.GetPath(&path);
        BMenuItem* item = new BMenuItem(path.Path(), NULL);
        
		AddItem(item);
        index++;
    }
    
	if (index == 0) {
		BMenuItem* item = new BMenuItem("no targets found.", 0);
		item->SetEnabled(false);
		AddItem(item);
	}
}
