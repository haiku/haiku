/*
 * Copyright 2014, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "MidiWindow.h"

#include <LayoutBuilder.h>

#include "MidiView.h"


MidiWindow::MidiWindow()
	:
	BWindow(BRect(40, 40, 260, 260), "Midi Preferences",
			B_TITLED_WINDOW, B_SUPPORTS_LAYOUT)
{
	fMidiView = new MidiView();
	BLayoutBuilder::Group<>(this, B_HORIZONTAL)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(fMidiView, 0.0f);
}


