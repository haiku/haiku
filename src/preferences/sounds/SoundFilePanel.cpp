// SndFilePanel.cpp
// ------------
// A BFilePanel derivative that lets you select and preview
// audio files.
//
// Copyright 2000, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.


#include "SoundFilePanel.h"

#include <Catalog.h>
#include <ControlLook.h>
#include <File.h>
#include <Messenger.h>
#include <NodeInfo.h>
#include <String.h>
#include <Window.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SoundFilePanel"


const uint32 kPlayStop = 'plst';
const uint32 kUpdateButton = 'btup';
const char* kPlayLabel = "\xE2\x96\xB6";
const char* kStopLabel = "\xE2\x96\xA0";


SoundFilePanel::SoundFilePanel(BHandler* handler)
	:
	BFilePanel(B_OPEN_PANEL, new BMessenger(handler), NULL, B_FILE_NODE, false, NULL,
		new SoundFileFilter(), false, true),
	fSoundFile(NULL),
	fPlayButton(NULL),
	fButtonUpdater(NULL)
{
	BView* view;

	if (Window()->Lock()) {
		Window()->AddHandler(this);

		BView* cancel = Window()->FindView("cancel button");
		if (cancel != NULL) {
			view = Window()->ChildAt(0);
			if (view != NULL) {
				static const float spacing = be_control_look->DefaultItemSpacing();
				BRect cancelRect(cancel->Frame());
				BRect playRect(spacing, cancelRect.top, cancelRect.Height() + spacing,
					cancelRect.bottom);
				fPlayButton = new BButton(playRect, "PlayStop", kPlayLabel, new BMessage(kPlayStop),
					B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
				fPlayButton->SetTarget(this);
				fPlayButton->SetEnabled(false);
				view->AddChild(fPlayButton);
			}
		}
		Window()->Unlock();
	}

	SetButtonLabel(B_DEFAULT_BUTTON, B_TRANSLATE("Select"));
	Window()->SetTitle(B_TRANSLATE("Select a sound file"));
}


SoundFilePanel::~SoundFilePanel()
{
	delete RefFilter();
	delete fSoundFile;
	delete fButtonUpdater;
}


void
SoundFilePanel::SelectionChanged()
{
	status_t err;
	entry_ref ref;

	if (fSoundFile) {
		delete fSoundFile;
		fSoundFile = NULL;
		fPlayButton->SetEnabled(false);
	}

	Rewind();
	err = GetNextSelectedRef(&ref);
	if (err == B_OK) {
		BNode node(&ref);
		if (!node.IsDirectory()) {
			delete fSoundFile;
			fSoundFile = new BFileGameSound(&ref, false);
			if (fSoundFile->InitCheck() == B_OK)
				fPlayButton->SetEnabled(true);
		}
	}
}


void
SoundFilePanel::WasHidden()
{
	delete fButtonUpdater;
	fButtonUpdater = NULL;
}


void
SoundFilePanel::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kPlayStop:
		{
			if (fSoundFile != NULL) {
				if (fSoundFile->IsPlaying()) {
					fSoundFile->StopPlaying();
					fPlayButton->SetLabel(kPlayLabel);
				} else {
					fSoundFile->StartPlaying();
					fPlayButton->SetLabel(kStopLabel);
					if (fButtonUpdater == NULL) {
						fButtonUpdater = new BMessageRunner(BMessenger(this),
							new BMessage(kUpdateButton), 500000); // every .5 sec
					}
				}
			}
			break;
		}
		case kUpdateButton:
		{
			if (fSoundFile != NULL) {
				if (!fSoundFile->IsPlaying())
					fPlayButton->SetLabel(kPlayLabel);
			}
			break;
		}
	}
}


bool
SoundFileFilter::Filter(const entry_ref* entryRef, BNode* node, struct stat_beos* stat,
	const char* fileType)
{
	bool admitIt = false;
	char type[256];
	const BString mask("audio");
	BNodeInfo nodeInfo(node);

	if (node->IsDirectory()) {
		admitIt = true;
	} else {
		nodeInfo.GetType(type);
		admitIt = (mask.Compare(type, mask.CountChars()) == 0);
	}

	return admitIt;
}
