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

#define DEBUG 1
#include <Debug.h>
#include <ctype.h>
#include "Link.h"
#include "XReferences.h"
#include "Bookmark.h"
#include "PDFWriter.h"
#include "Log.h"
#include "Report.h"

Link::Link(PDFWriter* writer, BString* utf8, BFont* font)
	: fWriter(writer)
	, fUtf8(utf8)
	, fFont(font)
	, fPos(0)
	, fContainsLink(false)
{
}


// create link from fStartPos to fEndPos and fStart to end
void 
Link::CreateLink(BPoint end)
{
	if (fFont->Rotation() != 0.0) {
		REPORT(kWarning, fWriter->fPage, "Warning: Can not create link for rotated font!");
		return;
	}
	
	// calculate rectangle for url
	font_height height;
	fFont->GetHeight(&height);

	float llx, lly, urx, ury;	

	llx = fWriter->tx(fStart.x);
	urx = fWriter->tx(end.x);
	lly = fWriter->ty(fStart.y + height.descent);
	ury = fWriter->ty(end.y - height.ascent);

	CreateLink(llx, lly, urx, ury);
}


void
Link::NextChar(int cps, float x, float y, float w)
{
	if (fContainsLink) {
		if (fPos == fStartPos) {
			fStart.Set(x, y);
		}
		if (fPos == fEndPos) {
			CreateLink(BPoint(x + w, y));
			DetectLink(fPos + cps);
		}
		fPos += cps;
	}
}

// TODO: check this list and add more prefixes
char* WebLink::fURLPrefix[] = {
	"http://",
	"https://",
	"ftp://",
	"telnet://",
	"mailto:",
	"news://",
	"nntp://",
	"gopher://",
	"wais://",
	"prospero://",
	"mid:",
	"cid:",
	"afs://",
	NULL
};


// Superset of xpalphas rule in BNF for specific URL schemes 
// See: http://www.w3.org/Addressing/URL/5_BNF.html
bool
WebLink::IsValidCodePoint(const char* cp, int cps) {
	if (cps == 1) {
		char c = *cp;
		switch (c) {
			// safe
			case '$': case '-': case '_': case '@': 
			case '.': case '&': case '+': 
			// extra
			case '!': case '*': case '"': case '\'':
			case '(': case ')': case ',': 
			// hex (prefix only)
			case '%':
			// reserved (subset)
			case '/': case '?': case '=':
			// national (subset)
			case '~':
				return true;
			default:
				return isalpha(c) || isdigit(c);		
		}
	}
	return false;
}


bool 
WebLink::DetectUrlWithPrefix(int start)
{
	int pos = INT_MAX;
	char* prefix = NULL;
	const char* utf8 = fUtf8->String();

	// search prefix with smallest position
	for (int i = 0; fURLPrefix[i]; i ++) {
		int p = fUtf8->FindFirst(fURLPrefix[i], start);
		if (p >= start && p < pos) {
			pos = p;
			prefix = fURLPrefix[i];
		}
	}
	
	if (pos != INT_MAX) {
		fStartPos = pos;
		fEndPos   = pos + strlen(prefix);
		
		// search end of url
		int         prev;		
		int         cps;
		const char* cp;

		prev = fEndPos;
		pos  = fEndPos;
		cp   = &utf8[pos];
		cps  = fWriter->CodePointSize(cp);
		while (IsValidCodePoint(cp, cps)) {
			prev = pos;
			pos += cps;
			cp  = &utf8[pos];
			cps  = fWriter->CodePointSize(cp);
		}
		
		// skip from end over '.' and '@'
		while (fStartPos < prev && (utf8[prev] == '.' || utf8[prev] == '@')) {
			// search to begin of code point
			do {
				prev --;
			} while (!fWriter->BeginsChar(utf8[prev]));
		}
		
		if (prev != fEndPos) { // url not empty
			fEndPos = prev; 
			return true;
		}
	}

	return false;
}


bool
WebLink::IsValidStart(const char* cp)
{
	return *cp != '.' && *cp != '@' && IsValidCodePoint(cp, 1);
}

bool
WebLink::IsValidChar(const char* cp)
{
	return IsValidCodePoint(cp, 1); 
}

// TODO: add better url detection (ie. "e.g." is detected as "http://e.g")
// A complete implementation should identify these kind of urls correctly:
//   domain/~user
//   domain/page.html?value=key
bool
WebLink::DetectUrlWithoutPrefix(int start)
{
	const char* utf8 = fUtf8->String();	

	while (utf8[start]) {
		// search to start
		while (utf8[start] && !IsValidStart(&utf8[start])) {
			start += fWriter->CodePointSize(&utf8[start]);
		}
		
		// search end and count dots and @s
		int numDots = 0, numAts = 0;
		int end = start;
		int prev = end;
		while (utf8[end] && IsValidChar(&utf8[end])) {
			if (utf8[end] == '.') numDots ++;
			else if (utf8[end] == '@') numAts ++;
			prev = end; end += fWriter->CodePointSize(&utf8[end]);
		}
		
		// skip from end over '.' and '@'
		while (start < prev && (utf8[prev] == '.' || utf8[prev] == '@')) {
			if (utf8[prev] == '.') numDots --;
			else if (utf8[prev] == '@') numAts --;
			// search to begin of code point
			do {
				prev --;
			} while (!fWriter->BeginsChar(utf8[prev]));
		}
		
		fStartPos = start; fEndPos = prev;
		int32 length = fEndPos - fStartPos;

		if (numAts == 1) {
			fKind = kMailtoKind;
			return true;
		} else if (numDots >= 2 && length > 2*numDots+1 && numAts == 0) {
			if (strncmp(&utf8[start], "ftp", 3) == 0) fKind = kFtpKind;
			else fKind = kHttpKind;
			return true;
		}
		
		// next iteration
		start = end;
	}	
	return false;
}

void 
WebLink::DetectLink(int start)
{
	fContainsLink = true;
	fKind         = kUnknownKind;
	if (DetectUrlWithPrefix(start)) return;
	if (DetectUrlWithoutPrefix(start)) return;
	fContainsLink = false;
}


// create link from fStartPos to fEndPos and fStart to end
void 
WebLink::CreateLink(float llx, float lly, float urx, float ury)
{
	BString url;

	// prepend protocol if required
	switch (fKind) {
		case kMailtoKind:  url = "mailto:"; break;
		case kHttpKind:    url = "http://"; break;
		case kFtpKind:     url = "ftp://"; break;
		case kUnknownKind: break;
		default: 
			LOG((fWriter->fLog, "Error: Link kind missing!!!"));
	}

	// append url	
	int endPos = fEndPos + fWriter->CodePointSize(&fUtf8->String()[fEndPos]);
	
	for (int i = fStartPos; i < endPos; i ++) {
		url.Append(fUtf8->ByteAt(i), 1);
	}		

	REPORT(kInfo, fWriter->fPage, "Web link '%s'", url.String());
	// XXX: url should be in 7-bit ascii encoded!
	PDF_add_weblink(fWriter->fPdf, llx, lly, urx, ury, url.String());
}


WebLink::WebLink(PDFWriter* writer, BString* utf8, BFont* font)
	: Link(writer, utf8, font)
{
}


// TextSegment

TextSegment::TextSegment(const char* text, BPoint start, float escpSpace, float escpNoSpace, BRect* bounds, BFont* font, PDFSystem* system)
	: fText(text)
	, fStart(start)
	, fEscpSpace(escpSpace)
	, fEscpNoSpace(escpNoSpace)
	, fBounds(*bounds)
	, fFont(*font)
	, fSystem(*system)
	, fSpaces(0)
{
}


void TextSegment::PrependSpaces(int32 n) {
	ASSERT(fSpaces == 0);
	fSpaces = n;
	BString spaces;
	spaces.Append(' ', n);
	fText.Prepend(spaces.String());
}


// TextLine

TextLine::TextLine(PDFWriter* writer)
	: fWriter(writer)
{
}


void TextLine::Add(TextSegment* segment) {
	// simply skip rotated text
	if (segment->Font()->Rotation() != 0.0) {
		delete segment; return;
	}

	if (!Follows(segment)) {
		Flush();
	}

	fSegments.AddItem(segment);
}


bool TextLine::Follows(TextSegment* segment) const {
	const int32 n = fSegments.CountItems();
	if (n == 0) return true;
	TextSegment* s = fSegments.ItemAt(n-1);
	// ATM segments must have same PDFSystem!
	PDFSystem* os = s->System();
	PDFSystem* ns = segment->System();
	if (os->Origin() != ns->Origin() || os->Scale() != ns->Scale() || os->Height() != ns->Height()) return false;

	// follows the new segment the latest one?	
	BPoint start = segment->Start();
	BRect  b = s->Bounds();
	float w = segment->Font()->StringWidth(" ", 1);
	if (b.top <= start.y && start.y <= b.bottom && (b.right-w/2.0) <= start.x) {
		// insert spaces
		if (w > 0.0) {
			int n = (int)((start.x - b.right) / w);
			if (n > 200) n = 200; // santiy check
			if (n > 0) {
				segment->PrependSpaces(n);
			}
		}
		return true;
	}
	return false;
}


void TextLine::Flush() {
	const int32 n = fSegments.CountItems();
	if (n == 0) return;

	// build the text in the line	
	BString line;
	for (int32 i = 0; i < n; i ++) {
		line << fSegments.ItemAt(i)->Text();
	}	 

	const char* c = line.String();

	if (!fWriter->MakesPDF()) {
		if (fWriter->fCreateXRefs) fWriter->RecordDests(c);
	} else {
		// XXX
		TextSegment* s = fSegments.ItemAt(0);
		BFont* f = s->Font();

		// simple link handling for now
		WebLink webLink(fWriter, &line, f);
		if (fWriter->fCreateWebLinks) webLink.Init();
	
		// local links
		LocalLink localLink(fWriter->fXRefs, fWriter->fXRefDests, fWriter, &line, f, fWriter->fPage);
		if (fWriter->fCreateXRefs) localLink.Init();
	
		// simple bookmark adding (XXX: s->Start()
		if (fWriter->fCreateBookmarks) {
			BPoint start(s->System()->tx(s->Start().x), s->System()->ty(s->Start().y));
			fWriter->fBookmark->AddBookmark(start, c, f);
		}
	

		for (int32 i = 0; i < n; i ++) {
		
			TextSegment* seg         = fSegments.ItemAt(i);
			BPoint       pos         = seg->Start();
			BFont*       font        = seg->Font();
			const char*  sc          = seg->Text();
			float        escpSpace   = seg->EscpSpace();
			float        escpNoSpace = seg->EscpNoSpace();
			int32        spaces      = seg->Spaces();
			
			while (*sc != 0) {
				ASSERT(*sc == *c);
				
				int s = fWriter->CodePointSize((char*)c);
		
				float w = font->StringWidth(c, s);
		
				if (fWriter->fCreateWebLinks) webLink.NextChar(s,   pos.x, pos.y, w);
				if (fWriter->fCreateXRefs)    localLink.NextChar(s, pos.x, pos.y, w);
				
				if (spaces == 0) {
					// position of next character
					if (*(unsigned char*)c <= 0x20) { // should test if c is a white-space!
						w += escpSpace;
					} else {
						w += escpNoSpace;
					}
		
					pos.x += w;
				} else {
					// skip over the prepended spaces
					spaces --;
				}
					
				// next character
				c += s; sc += s;
			}
		}
	}
	
	fSegments.MakeEmpty();
}

