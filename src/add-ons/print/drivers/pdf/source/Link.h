/*

Detection and adding of web links.

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

#ifndef _LINK_H
#define _LINK_H

#include <String.h>
#include <Font.h>
#include <Point.h>
#include "Utils.h"

class PDFWriter;

class Link {
protected:
	PDFWriter* fWriter;
	BString*   fUtf8;
	BFont*     fFont;
	int        fPos; // into fUtf8
		
	bool       fContainsLink;
	int        fStartPos, fEndPos;
	BPoint     fStart;

	virtual void DetectLink(int start) = 0;
	virtual void CreateLink(float llx, float lly, float urx, float ury) = 0;
	
	void CreateLink(BPoint end);
	
public:
	Link(PDFWriter* writer, BString* utf8, BFont* font);
	void Init() { DetectLink(0); }
	void NextChar(int cps, float x, float y, float w);
};

class WebLink : public Link {
	enum kind {
		kMailtoKind,
		kHttpKind,
		kFtpKind,
		kUnknownKind
	};

	enum kind  fKind;

	static char* fURLPrefix[];

	bool IsValidStart(const char* cp);
	bool IsValidChar(const char* cp);
	bool DetectUrlWithoutPrefix(int start);

	bool IsValidCodePoint(const char* cp, int cps);
	bool DetectUrlWithPrefix(int start);	

	void CreateLink(float llx, float lly, float urx, float ury);
	void DetectLink(int start);
	
public:
	WebLink(PDFWriter* writer, BString* utf8, BFont* font);
};

class TextSegment {
	BString      fText;
	BPoint       fStart;
	float        fEscpSpace;   // escapement space
	float        fEscpNoSpace; // escapement no space
	BRect        fBounds;
	BFont        fFont;
	PDFSystem    fSystem;
	int32        fSpaces;
		
public:
	TextSegment(const char* text, BPoint start, float escpSpace, float escpNoSpace, BRect* bounds, BFont* font, PDFSystem* system);

	void        PrependSpaces(int32 n);

	const char* Text() const        { return fText.String(); }
	BPoint      Start() const       { return fStart; }
	float       EscpSpace() const   { return fEscpSpace; }
	float       EscpNoSpace() const { return fEscpNoSpace; }
	BRect       Bounds() const      { return fBounds; }
	BFont*      Font()              { return &fFont; }
	PDFSystem*  System()            { return &fSystem; }
	int32       Spaces() const      { return fSpaces; }
};


class TextLine {
	TList<TextSegment> fSegments;
	PDFWriter*         fWriter;
	
	bool Follows(TextSegment* segment) const;
	
public:
	TextLine(PDFWriter* writer);
	
	void Add(TextSegment* segment);
	void Flush();
};


#endif
