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

#ifndef _REPORT_H
#define _REPORT_H

#include <stdio.h>
#include <String.h>
#include <Locker.h>
#include "PrintUtils.h"


enum kind {
	kInfo,
	kWarning,
	kError,
	kDebug,
};

const int32 kNumKinds = kDebug + 1;


class ReportRecord {
public:
private:
	kind    fKind;
	int32   fPage;		
	BString fLabel;
	BString fDesc;

public:	
	ReportRecord(kind kind, int32 page, const char* label, const char* desc)
		: fKind(kind), fPage(page), fLabel(label), fDesc(desc) 
	{ }
	
	kind Kind() const         { return fKind; }
	int32 Page() const        { return fPage; }
	const char* Label() const { return fLabel.String(); }
	const char* Desc() const  { return fDesc.String(); }
};


class Report : public TList<ReportRecord>{
	static  Report* fInstance;
	int32   fNum[kNumKinds];
	BLocker fLock;
	
	Report();
	~Report();
	
	typedef TList<ReportRecord> inherited;
	
public:
	static Report* Instance();
	void           Free();

	// Only these methods are "synchronized":
	// Note that they are *NOT* virtual in the base class!
	void          Add(kind kind, int32 page, const char* fmt, ...);
	int           Count(kind kind);
	ReportRecord* ItemAt(int32 i);
	int32         CountItems();
};


#define REPORT  Report::Instance()->Add

#endif
