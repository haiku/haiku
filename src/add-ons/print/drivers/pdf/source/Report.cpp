/*

Report.

Copyright (c) 2002 OpenBeOS. 

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

#include "Report.h"
#include <string.h>
#include <Autolock.h>

#define LOGGING 0
#define LOG_TO_STDERR 0

Report* Report::fInstance = NULL;


Report::Report() {
	for (int i = 0; i < kNumKinds; i++) {
		fNum[i] = 0;
	}
}


Report::~Report() {
}


Report* Report::Instance() {
	if (fInstance == NULL) {
		fInstance = new Report();
	}
	return fInstance;
}


void Report::Free() {
	delete fInstance; fInstance = NULL;
}

void Report::Add(kind kind, int32 page, const char* fmt, ...) {
	#if !LOGGING
	if (kind == kDebug) return;
	#endif
	
	if (kind >= 0 && kind < kNumKinds) {
		BAutolock lock(fLock);
		fNum[kind] ++;
		
		va_list list;
		char *b = new char[1 << 16];
		va_start(list, fmt);
#if LOG_TO_STDERR
		vfprintf(stderr, fmt, list);
		fprintf(stderr, "\n");
#endif
		vsprintf(b, fmt, list);
		AddItem(new ReportRecord(kind, page, "", b));
		delete b;
		va_end(list);
	}
}

int Report::Count(kind kind) {
	if (kind >= 0 && kind < kNumKinds) {
		BAutolock lock(fLock);
		return (int)fNum[kind];
	}
	return 0;
}

ReportRecord* Report::ItemAt(int32 i) {
	BAutolock lock(fLock);
	return inherited::ItemAt(i);
}

int32 Report::CountItems() {
	BAutolock lock(fLock);
	return inherited::CountItems();
}
