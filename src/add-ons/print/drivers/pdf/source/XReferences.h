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

#ifndef _X_REF_H
#define _X_REF_H

#include <List.h>
#include "Link.h"
#include "RegExp.h" 
#include "Utils.h"

class MatchResult;
class Pattern;


// PatternList

typedef TList<Pattern> PatternList;


// Pattern

class Pattern {
protected:
	RegExp fPattern;
	
	Pattern() { };

public:
	Pattern(const char* pattern);
	virtual ~Pattern();
	status_t InitCheck();
	bool Matches(const char* string, MatchResult* result) const; 

	virtual bool IsLink() const { return false; }
	virtual bool IsDest() const { return false; }
	void Print() const;
};


// two subclasses for destinction between link and anchor

class LinkPattern : public Pattern {
public:
	LinkPattern(const char* pattern) : Pattern(pattern) { }
	
	bool IsLink() const { return true; }
};


class DestPattern : public Pattern {
public:
	DestPattern(const char* pattern) : Pattern(pattern) { }
	
	bool IsDest() const { return true; }
};


// MatchResult; wrapper for struct regexp 

// NB: Pattern object must not die while MatchResult is alive
//     Pattern::Matches() invalidates a previous MatchResult!

class MatchResult {
private:
	regexp*     fRegExp;
	const char* fString;
	int32       fResults;

	bool        HasResult(int32 index) const;
public:
	MatchResult();
	void SetRegExp(regexp* regexp)     { fRegExp  = regexp; }
	void SetString(const char* string) { fString  = string; }
	void SetResults(int32 results)     { fResults = results; }	
	
	int32       CountResults() const         { return fResults; }
	const char* Start(int index) const;
	int32       StartPos(int index) const;   // inclusive
	int32       EndPos(int index) const;     // exclusive
	int32       Length(int index) const      { return EndPos(index) - StartPos(index); }
	void        GetString(int index, BString *result);
};


// XRefDef

class XRefDef {
private:
	PatternList fLinks;
	PatternList fDests;
	int         fId;

	Pattern* Matches(PatternList* list, const char* s, const char** start, int32 *len);
	
public:
	XRefDef() { fId = -1; }

	void     SetId(int32 id)           { fId = id; }
	int32    Id() const                { return fId; }

	void     AddLink(LinkPattern* p)   { fLinks.AddItem(p); }
	void     AddDest(DestPattern* p)   { fDests.AddItem(p); }
	int32    CountLinks() const        { return fLinks.CountItems(); }
	int32    CountDests() const        { return fDests.CountItems(); }
	Pattern* LinkAt(int32 index) const { return fLinks.ItemAt(index); }
	Pattern* DestAt(int32 index) const { return fDests.ItemAt(index); }

	Pattern* Matches(const char* s, const char** start, int32 *len);
};


// XMatchResult; called from XRefDefs::Matches with matches

class XMatchResult {
public:
	virtual ~XMatchResult() { };
	virtual bool Link(XRefDef* def, MatchResult* result) = 0;
	virtual bool Dest(XRefDef* def, MatchResult* result) = 0;
};


// XRefDefs

class XRefDefs {
private:
	TList<XRefDef> fDefs;
public:

	void Add(XRefDef* def);
	int32 Count() { return fDefs.CountItems(); }
	// if "all" is false, only start of string is matched
	void Matches(const char* s, XMatchResult* result, bool all);

	bool Read(const char* name);
};


// Destination

class Destination {
private:
	BString fLabel;
	int32   fPage;
	
public:
	Destination(const char* label, int32 page);
	const char* Label() const { return fLabel.String(); }
	int32       Page() const  { return fPage; }
};


// DestList

class DestList : public TList<Destination> {
public:
	void Sort();
	Destination* Find(const char* label);
};


// XRefDests; container for the destinations 

class XRefDests {
	TList<DestList> fDests;
	
	DestList* GetList(XRefDef* def) const { return fDests.ItemAt(def->Id()); }
	
public:
	XRefDests(int32 n);
	bool Add(XRefDef* def, const char* label, int32 page);
	bool Find(XRefDef* def, const char* label, int32* page) const;
};


// RecordDests; detect destinations and store them in XRefDests object

class RecordDests : public XMatchResult {
	XRefDests* fDests;
	int32      fPage;
	
public:
	RecordDests(XRefDests* dests, int32 page);
	bool Link(XRefDef* def, MatchResult* result);
	bool Dest(XRefDef* def, MatchResult* result);	
};


// LocalLink; detect links and create local links in PDF to destination

class LocalLink : public Link, XMatchResult {
protected:
	XRefDefs*  fDefs;
	XRefDests* fDests;
	
	int32      fLinkPage, fDestPage;

	void DetectLink(int start);
	void CreateLink(float llx, float lly, float urx, float ury);

public:
	LocalLink(XRefDefs* defs, XRefDests* dests, PDFWriter* writer, BString* utf8, BFont* font, int32 page);
	bool Link(XRefDef* def, MatchResult* result);
	bool Dest(XRefDef* def, MatchResult* result);
};


#endif
