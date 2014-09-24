/*
 * Copyright 2014, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef MIDIWINDOW_H_
#define MIDIWINDOW_H_

#include <Window.h>

class MidiView;
class MidiWindow : public BWindow {
public:
	MidiWindow();
private:
	MidiView* fMidiView;
};


#endif /* MIDIWINDOW_H_ */
