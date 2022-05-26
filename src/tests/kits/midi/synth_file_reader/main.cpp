/*

main.cpp

Copyright (c) 2002 Haiku.

Author:
	Michael Pfeiffer

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

#include "SynthFileReader.h"
#include "SynthFile.h"
#include <stdlib.h>
#include <Application.h>


void start_be_app() {
	new BApplication("application/x.vnd-synth-file-reader");
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("%s <synth file> [play [instr] | dump]\n", argv[0]);
		return -1;
	}
	const char* fileName = argv[1];
	bool        play     = argc >= 3 ? strcmp(argv[2], "play") == 0 : false;
	bool        dump     = argc >= 3 ? strcmp(argv[2], "dump") == 0 : false;
	uint32      instr    = argc >= 4 ? atol(argv[3]) : 0xffff;

	if (dump) {
		SSynthFile synth(fileName);
		if (synth.InitCheck() == B_OK) {
			synth.Dump();
		}
	} else {
		SSynthFileReader reader(fileName);
		if (reader.InitCheck() == B_OK) {
			start_be_app();
			reader.Dump(play, instr);
		} else {
			printf("could not open '%s' or not a valid synth file!\n", fileName);
		}	
	}
}
