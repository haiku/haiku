/*

main.cpp

Copyright (c) 2002 OpenBeOS. 

Test application plays a midi file via a BMidiLocalProducer.

Authors: 
	Michael Pfeiffer
	Paul Stadler

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include <Application.h>
#include <MidiStore.h>
#include <Entry.h>
#include <stdio.h>

#include "MidiDelay.h"
#include "Midi1To2Bridge.h"

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Error missing parameter!\n%s midi_file\n", argv[0]);
		return -1;
	}

	BEntry entry(argv[1],true);
	if(!entry.Exists()) {
		fprintf(stderr, "File does not exist!\n");
		return -1;
	}

	new BApplication("application/vnd.obos-midiplayer");
	BMidiStore     store;
	MidiDelay      delay;
	Midi1To2Bridge bridge("MidiPlayer output");	
	
	entry_ref e_ref;
	entry.GetRef(&e_ref);
	store.Import(&e_ref);
	store.Connect(&delay);
	delay.Connect(&bridge);
	// Use PatchBay to connect the MidiProducer with a MidiConsumer
	// (or just MidiSynth 1.6)
	printf("Connect MidiPlayer output to a MidiConsumer.\n""Hit return to start! "); getchar();
	store.Start();
	while(store.IsRunning()) {
		snooze(100000);
	}
	store.Stop();	
	store.Disconnect(&delay);
	delay.Disconnect(&bridge);
}
