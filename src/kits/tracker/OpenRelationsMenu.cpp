#include "Attributes.h"
#include "AutoLock.h"
#include "Commands.h"
#include "FSUtils.h"
#include "IconMenuItem.h"
#include "OpenRelationsMenu.h"
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

OpenRelationsMenu::OpenRelationsMenu(const char* label, const BMessage* entriesToOpen,
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


OpenRelationsMenu::OpenRelationsMenu(const char* label, const BMessage* entriesToOpen,
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
OpenRelationsMenu::StartBuildingItemList()
{
    fSenMessenger = BMessenger(SEN_SERVER_SIGNATURE);
    
    if (fSenMessenger.IsValid()) {
        BMessage* message = new BMessage(fEntriesToOpen);
        message->what = SEN_RELATIONS_GET;     // get all relations for refs received
        
        //TODO: support multi-selection - when SEN is adapted
        entry_ref ref;
        fEntriesToOpen.FindRef("refs", &ref);
        BEntry entry(&ref);
        BPath path;
        entry.GetPath(&path);
        
        PRINT(("Tracker->SEN: getting relations for path %s", path.Path()));
        message->AddString(SEN_RELATION_SOURCE, path.Path());
        
        fSenMessenger.SendMessage(message, &fRelationsReply);
        
        PRINT(("SEN->Tracker: received reply: %c", fRelationsReply.what));
        
        // also add source to reply for later use in building relation target menu
        fRelationsReply.AddString(SEN_RELATION_SOURCE, path.Path());
        
        return true;
    } else {
        PRINT(("failed to reach SEN server, is it running?"));
        return false;
    }
}

bool OpenRelationsMenu::AddNextItem()
{
    // nothing to do here
    return false;
}

void OpenRelationsMenu::ClearMenuBuildingState() 
{
    //  empty;
    return;
}

void
OpenRelationsMenu::DoneBuildingItemList()
{
	// target the menu
	if (target != NULL)
		SetTargetForItems(target);
	else
		SetTargetForItems(fMessenger);

    BString relation, source;
    int index = 0;
    fRelationsReply.FindString(SEN_RELATION_SOURCE, &source);
    
    while (fRelationsReply.FindString("relations", index, &relation) == B_OK) {
        BMessage* message = new BMessage(SEN_RELATIONS_GET_TARGETS);
        message->AddString(SEN_RELATION_SOURCE, source.String());
        message->AddString(SEN_RELATION_NAME, relation.String());
        
        BMenuItem* item = new BMenuItem(
            new OpenRelationTargetsMenu(relation.String(), message, fParentWindow, be_app_messenger),
            0
        );
		AddItem(item);
        
        index++;
    }
    
	if (index == 0) {
		BMenuItem* item = new BMenuItem("no relations found.", 0);
		item->SetEnabled(false);
		AddItem(item);
	}
}
