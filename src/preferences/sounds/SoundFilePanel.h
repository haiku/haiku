// SndFilePanel.h
// ------------
// A BFilePanel derivative that lets you select and preview
// audio files.
//
// Copyright 2000, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#ifndef _SOUNDFILEPANEL_H_
#define _SOUNDFILEPANEL_H_

#include <Button.h>
#include <FileGameSound.h>
#include <FilePanel.h>
#include <Handler.h>
#include <MessageRunner.h>


class SoundFilePanel : public BFilePanel, public BHandler {
public:
	SoundFilePanel(BHandler* handler);
	virtual ~SoundFilePanel();

	virtual void SelectionChanged();
	virtual void WasHidden();

	virtual void MessageReceived(BMessage* msg);

private:
	BFileGameSound* fSoundFile;
	BButton* fPlayButton;
	BMessageRunner* fButtonUpdater;
};


class SoundFileFilter : public BRefFilter {
public:
	bool Filter(const entry_ref* entryRef, BNode* node, struct stat_beos* stat,
			const char* fileType);
};

#endif // _SOUNDFILEPANEL_H_
