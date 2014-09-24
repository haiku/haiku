/*
 * Copyright 2014, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef MIDIVIEW_H_
#define MIDIVIEW_H_


#include <GroupView.h>

class BButton;
class BListView;
class MidiView : public BGroupView {
public:
	MidiView();
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage* message);

private:
	void _RetrieveSoftSynthList();

	BButton* fImportButton;
	BListView* fListView;
};

#endif /* MIDIVIEW_H_ */
