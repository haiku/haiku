/*

Cross References.

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

#include <Debug.h>
#include <UTF8.h>
#include "Scanner.h"
#include "PDFWriter.h"
#include "XReferences.h"
#include "Report.h"


// Pattern

Pattern::Pattern(const char* pattern) {
	fPattern.SetTo(pattern);
}


Pattern::~Pattern() {
}


status_t Pattern::InitCheck() {
	return fPattern.InitCheck();
}


bool Pattern::Matches(const char* string, MatchResult* result) const {
	regexp* re = fPattern.Expression();
	result->SetRegExp(re);
	result->SetString(string);
	if (fPattern.RunMatcher(re, string)) {
		int32 i;
		for (i = 0; i < kSubExpressionMax && re->startp[i]; i++);
		result->SetResults(i);
		return true;
	} else {
		result->SetResults(0);
		return false;
	}
}


// MatchResult

MatchResult::MatchResult() {
	fRegExp  = NULL;
	fString  = NULL;
	fResults = 0;
}


bool MatchResult::HasResult(int32 index) const {
	return fRegExp && fString && index >= 0 && index < fResults;
}


const char* MatchResult::Start(int index) const {
	if (HasResult(index)) {
		return fRegExp->startp[index];
	}
	return NULL;
}

int32 MatchResult::StartPos(int index) const {
	if (HasResult(index)) {
		return fRegExp->startp[index] - fString;
	}
	return -1;
}


int32 MatchResult::EndPos(int index) const {
	if (HasResult(index)) {
		return fRegExp->endp[index] - fString;
	}
	return -1;
}


void  MatchResult::GetString(int index, BString *result) {
	*result = "";
	if (HasResult(index)) {
		const char* s = fRegExp->startp[index];
		const char* e = fRegExp->endp[index];
		while (s != e) { *result << *s; s++; }
	}
}


// XRefDef

Pattern* XRefDef::Matches(PatternList* list, const char* s, const char** start, int32* len) {
	MatchResult m;
	const int32 n = list->CountItems();
	Pattern* pat = NULL;
	for (int32 i = 0; i < n; i++) {
		Pattern* p = list->ItemAt(i);

		if (p->Matches(s, &m) && m.CountResults() >= 1) {
			if (m.Start(0) <= s) {
				if (m.Start(0) == s && m.Length(0) <= *len) continue;
				pat    = p;
				*start = m.Start(0);
				*len   = m.Length(0);
			}
		}
	}
	return pat;
}


Pattern* XRefDef::Matches(const char* s, const char** start, int32* len) {
	*start = s + strlen(s); *len = 0;		
	Pattern* pat1 = Matches(&fLinks, s, start, len);
	Pattern* pat2 = Matches(&fDests, s, start, len);
	return pat2 ? pat2 : pat1;
}


// XRefDefs

void XRefDefs::Add(XRefDef* def) {
	def->SetId(fDefs.CountItems());
	fDefs.AddItem(def);
}


void XRefDefs::Matches(const char* s, XMatchResult* r, bool all) {
	const int32 n              = fDefs.CountItems(); 
	const char* end            = s + strlen(s);
	MatchResult match;
	
	do {
		// match that starts at lowest position
		const char* start1 = end;
		XRefDef*    def1   = NULL;
		Pattern*    pat1   = NULL;
		int32       len1   = 0;
		
		for (int32 i = 0; i < n; i ++) {
			const char* start; int32 len;
			XRefDef* def = fDefs.ItemAt(i);
			Pattern* pat = def->Matches(s, &start, &len);
			if (pat && start <= start1) {
				if (start == start1 && len <= len1) continue;
				start1 = start; len1 = len; pat1 = pat; def1 = def;
			}
		}
		
		if (def1) {
			bool found = pat1->Matches(start1, &match);
			bool cont;
			ASSERT(found); (void)found;
			if (pat1->IsLink()) {
				cont = r->Link(def1, &match);
			} else {
				cont = r->Dest(def1, &match);
			}
			if (!cont) break;
			
			// update for next iteration
			s = start1 + len1;
		} else {
			if (all) {
				do { 
					s ++;
				} while (!BEGINS_CHAR(*s));
			} else {
				break;
			}
		}
	} while (*s);
}


class AutoDelete {
	XRefDef* fDefs;
public:
	AutoDelete(XRefDef* defs) : fDefs(defs) { }
	~AutoDelete()                           { delete fDefs; }
	void Release()                          { fDefs = NULL; }
};

/*
FileFormat: Definition.
Line comment starts with: '#'

Definition = Version {XRefDef}.
Version    = "CrossReferences" "1.0".
XRefDef    = Pattern {"," Pattern} "-" ">" Pattern {"," Pattern} ".". 
Pattern    = '"' string '"'.

Example:
CrossReferences 1.0
"see [tT]able ([0-9]+)", "see [tT]bl. ([0-9]+)"  -> "[tT]able ([0-9]+)".
"see [fF]igure ([0-9]+)" -> "[fF]igure ([0-9]+)".
"page ([0-9]+)"          -> "- ([0-9]+) -"
*/

bool XRefDefs::Read(const char* name) {
	Scanner scnr(name);

	if (scnr.InitCheck() == B_OK) {

		BString s; float f;
		bool ok = scnr.ReadName(&s) && scnr.ReadFloat(&f);
		if (!ok || strcmp(s.String(), "CrossReferences") != 0 || f != 1.0) {
			REPORT(kError, 0, "XRefs (line %d, column %d): '%s' not a cross references file or wrong version!", scnr.Line(), scnr.Column(), name);
			return false;
		}
		
		while (!scnr.IsEOF()) {
			XRefDef* def = new XRefDef();
			AutoDelete autoDelete(def);
			BString pattern;

			// Links: Pattern {"," Pattern}.
			do {
				if (!scnr.ReadString(&pattern)) {
					REPORT(kError, 0, "XRefs (line %d, column %d): Could not parse 'link' pattern", scnr.Line(), scnr.Column());
					return false;
				}
				LinkPattern* link = new LinkPattern(pattern.String());
				if (link->InitCheck() == B_OK) {
					def->AddLink(link);
				} else {
					delete link;
					REPORT(kError, 0, "XRefs (line %d, column %d): Invalid RegExp '%s'", pattern.String(), scnr.Line(), scnr.Column());
					return false;
				}
			} while (scnr.NextChar(','));

			// "->"
			if (scnr.NextChar('-') && scnr.NextChar('>')) {

				// Dests: Pattern {"," Pattern}.
				do {
					if (!scnr.ReadString(&pattern)) {
						REPORT(kError, 0, "XRefs (line %d, column %d): Could not parse 'destination' pattern", scnr.Line(), scnr.Column());
						return false;
					}
					DestPattern* dest = new DestPattern(pattern.String());
					if (dest->InitCheck() == B_OK) {
						def->AddDest(dest);
					} else {
						delete dest;
						REPORT(kError, 0, "XRefs (line %d, column %d): Invalid RegExp '%s'", scnr.Line(), scnr.Column(), pattern.String());
						return false;
					}
				} while (scnr.NextChar(','));

				// "."
				if (!scnr.NextChar('.')) {
					REPORT(kError, 0, "XRefs (line %d, column %d): '.' expected at end of definition", scnr.Line(), scnr.Column());
					return false;
				}
			} else {
				REPORT(kError, 0, "XRefs (line %d, column %d): '->' expected", scnr.Line(), scnr.Column());
				return false;
			}

			this->Add(def); 
			autoDelete.Release();
			
			scnr.SkipSpaces();
		}
		return true;
	} else {
		REPORT(kError, 0, "XRefs: Could not open cross references file '%d'", name);
	}	
	return false;	
}


// Destination

Destination::Destination(const char* label, int32 page, BRect bounds)
	: fLabel(label)
	, fPage(page)
	, fBoundingBox(bounds)
{
}


// DestList

void DestList::Sort() {
	// TODO: sort by label
}


Destination* DestList::Find(const char* label) {
	const int n = CountItems();
	// TODO: binary sort
	for (int32 i = 0; i < n; i++) {
		Destination* d = ItemAt(i);
		if (strcmp(d->Label(), label) == 0) return d;
	}
	return NULL;
}


// XRefDefs

XRefDests::XRefDests(int32 n) {
	fDests.MakeEmpty();
	for (int32 i = 0; i < n; i ++) {
		fDests.AddItem(new DestList());
	}
}


bool XRefDests::Add(XRefDef* def, const char* label, int32 page, BRect boundingBox) {
	DestList* list = GetList(def);
	ASSERT(list != NULL);
	if (!list->Find(label)) {
		list->AddItem(new Destination(label, page, boundingBox));
		return true;
	}
	return false;
}


bool XRefDests::Find(XRefDef* def, const char* label, int32* page, BRect* boundingBox) const {
	DestList* list = GetList(def);
	ASSERT(list != NULL);
	Destination* dest = list->Find(label);
	if (dest) {
		*page = dest->Page();
		*boundingBox = dest->BoundingBox();
		return true;
	}
	return false;
}


// RecordDests

RecordDests::RecordDests(XRefDests* dests, TextLine* line, int32 page)
	: fDests(dests)
	, fLine(line)
	, fPage(page)
{
}

bool RecordDests::Link(XRefDef* def, MatchResult* result) {
	return true;
}


bool RecordDests::Dest(XRefDef* def, MatchResult* result) {
	if (result->CountResults() >= 2) {
		BString s;
		BRect b;
		result->GetString(1, &s);
		if (fLine->BoundingBox(result->StartPos(1), result->EndPos(1), &b)) {
			fDests->Add(def, s.String(), fPage, b);
		}
	}
	return true;
}


// LocalLink

LocalLink::LocalLink(XRefDefs* defs, XRefDests* dests, PDFWriter* writer, BString* utf8, int32 page)
	: ::Link(writer, utf8)
	, fDefs(defs)
	, fDests(dests)
	, fLinkPage(page)
{
}


bool LocalLink::Link(XRefDef* def, MatchResult* result) {
	if (result->CountResults() >= 2) {
		BString label;
		result->GetString(1, &label);
		if (fDests->Find(def, label.String(), &fDestPage, &fDestBounds)) {
			result->Start(1);
			fContainsLink = true;
			fStartPos     = result->Start(1) - fUtf8->String();
			const char* p = result->Start(1) + result->Length(1);
			do {
				p --;
			} while (!BEGINS_CHAR(*p));
			fEndPos       = p - fUtf8->String();
			return false;
		} else {
			REPORT(kWarning, fLinkPage, "Destination for link '%s' not found!", label.String());
		}
	}
	return true;
}


bool LocalLink::Dest(XRefDef* def, MatchResult* result) {
	return true;
}


void LocalLink::DetectLink(int start) {
	fContainsLink = false;
	fDefs->Matches(fUtf8->String() + start, this, true);	
}


void LocalLink::CreateLink(float llx, float lly, float urx, float ury) {
	char optList[256];
	if (fDestPage != fLinkPage) {
		BString s(&fUtf8->String()[fStartPos], fEndPos-fStartPos+1);
		REPORT(kInfo, fLinkPage, "Link '%s' to page %d", s.String(), fDestPage);
		sprintf(optList, "type=fixed left=%f top=%f", fDestBounds.left, fDestBounds.top);
		PDF_add_locallink(fWriter->fPdf, llx, lly, urx, ury, fDestPage, optList);
	}
}
