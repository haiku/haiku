/*
 * Copyright (c) 2004 Matthijs Hollemans
 * Copyright (c) 2008-2014 Haiku, Inc. All rights reserved.
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


#include "MidiPlayerApp.h"

#include <AboutWindow.h>
#include <Alert.h>
#include <Catalog.h>
#include <Locale.h>
#include <StorageKit.h>

#include "MidiPlayerWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Main Application"


//	#pragma mark - MidiPlayerApp


MidiPlayerApp::MidiPlayerApp()
	:
	BApplication(MIDI_PLAYER_SIGNATURE)
{
	window = new MidiPlayerWindow();
}


void
MidiPlayerApp::ReadyToRun()
{
	window->Show();
}


void
MidiPlayerApp::AboutRequested()
{
	BAboutWindow* window = new BAboutWindow("MidiPlayer",
		MIDI_PLAYER_SIGNATURE);

	const char* extraCopyrights[] = {
		"2008-2014 Haiku, Inc.",
		NULL
	};
	window->AddCopyright(2004, "Matthijs Hollemans", extraCopyrights);

	const char* authors[] = {
		"Adrien Destugues",
		"Axel Dörfler",
		"Matthijs Hollemans",
		"Humdinger",
		"Ryan Leavengood",
		"Matt Madia",
		"John Scipione",
		"Jonas Sundström",
		"Oliver Tappe",
		NULL
	};
	window->AddAuthors(authors);

	window->AddDescription(B_TRANSLATE_COMMENT(
		"This tiny program\n"
		"Knows how to play thousands of\n"
		"Cheesy sounding songs",
		"This is a haiku. First line has five syllables, "
		"second has seven and last has five again. "
		"Create your own."));

	window->Show();
}


void
MidiPlayerApp::RefsReceived(BMessage* message)
{
	message->what = B_SIMPLE_DATA;
	window->PostMessage(message);
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


//	#pragma mark - main method


int
main()
{
	MidiPlayerApp app;
	app.Run();
	return 0;
}
