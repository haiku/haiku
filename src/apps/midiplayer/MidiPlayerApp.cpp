/*
 * Copyright (c) 2004 Matthijs Hollemans
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <Alert.h>
#include <Catalog.h>
#include <Locale.h>
#include <StorageKit.h>

#include "MidiPlayerApp.h"
#include "MidiPlayerWindow.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Main Application"

MidiPlayerApp::MidiPlayerApp()
	: BApplication(MIDI_PLAYER_SIGNATURE)
{
	window = new MidiPlayerWindow;
}


void
MidiPlayerApp::ReadyToRun()
{
	window->Show();
}


void
MidiPlayerApp::AboutRequested()
{
	(new BAlert(
		NULL,
		B_TRANSLATE_COMMENT("Haiku MIDI Player 1.0.0 beta\n\n"
		"This tiny program\n"
		"Knows how to play thousands of\n"
		"Cheesy sounding songs", "This is a haiku. First line has five syllables, second has seven and last has five again. Create your own."),
		"Okay", NULL, NULL,
		B_WIDTH_AS_USUAL, B_INFO_ALERT))->Go();
}


void
MidiPlayerApp::RefsReceived(BMessage* msg)
{
	msg->what = B_SIMPLE_DATA;
	window->PostMessage(msg);
}


void
MidiPlayerApp::ArgvReceived(int32 argc, char** argv)
{
	// Note: we only load the first file, even if more than one is specified.
	// For some reason, BeOS R5 MidiPlayer loads them all but will only play
	// the last one. That's not very useful.

	if (argc > 1) {
		BMessage msg;
		msg.what = B_SIMPLE_DATA;

		BEntry entry(argv[1]);
		entry_ref ref;
		entry.GetRef(&ref);
		msg.AddRef("refs", &ref);

		window->PostMessage(&msg);
	}
}


//	#pragma mark -


int
main()
{
	MidiPlayerApp app;
	app.Run();
	return 0;
}

