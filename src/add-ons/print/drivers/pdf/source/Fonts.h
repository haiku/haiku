/*

PDF Writer printer driver.

Copyright (c) 2001 OpenBeOS. 

Authors: 
	Philippe Houdoin
	Simon Gauvin	
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

#ifndef FONTS_H
#define FONTS_H

#include <Archivable.h>
#include <String.h>
#include <List.h>
#include "Utils.h"

enum font_encoding 
{
	macroman_encoding,
	// TrueType
	tt_encoding0,
	tt_encoding1,
	tt_encoding2,
	tt_encoding3,
	tt_encoding4,
	// Type 1
	t1_encoding0,
	t1_encoding1,
	t1_encoding2,
	t1_encoding3,
	t1_encoding4,
	// CJK
	japanese_encoding,
	chinese_cns1_encoding,
	chinese_gb1_encoding,
	korean_encoding,
	first_cjk_encoding  = japanese_encoding,
	no_of_cjk_encodings = korean_encoding - first_cjk_encoding + 1,
	invalid_encoding = korean_encoding + 1,
	user_defined_encoding_start
};


enum font_type 
{
	true_type_type,
	type1_type,
	unknown_type
};


class FontFile : public BArchivable
{
private:
	BString   fName;
	BString   fPath;
	int64     fSize;
	font_type fType;
	bool      fEmbed;
	BString   fSubst;  

public:
	FontFile() { }
	FontFile(const char *n, const char *p, int64 s, font_type t, bool embed) : fName(n), fPath(p), fSize(s), fType(t), fEmbed(embed) { }
	FontFile(BMessage *archive);
	~FontFile() {};

	static BArchivable* Instantiate(BMessage *archive);
	status_t Archive(BMessage *archive, bool deep = true) const;

	// accessors
	const char*		Name() const  { return fName.String(); }
	const char*		Path() const  { return fPath.String(); }
	int64			Size() const  { return fSize; }
	font_type		Type() const  { return fType; }
	bool			Embed() const { return fEmbed; }
	const char*     Subst() const { return fSubst.String(); }

	// setters
	void SetEmbed(bool embed)        { fEmbed = embed; }
	void SetSubst(const char* subst) { fSubst = subst; }
};


class Fonts : public BArchivable {
private:
	TList<FontFile>  fFontFiles;

	struct {
		font_encoding encoding;
		bool          active;
	} fCJKOrder[no_of_cjk_encodings];

	status_t	LookupFontFiles(BPath path);	

public:
	Fonts();
	Fonts(BMessage *archive);
	
	static BArchivable* Instantiate(BMessage *archive);
	status_t Archive(BMessage *archive, bool deep = true) const;

	status_t	CollectFonts();
	void        SetTo(BMessage *archive);

	FontFile*	At(int i) const { return fFontFiles.ItemAt(i); }
	int32		Length() const  { return fFontFiles.CountItems(); }

	void        SetDefaultCJKOrder();
	bool        SetCJKOrder(int i, font_encoding  enc, bool  active);
	bool        GetCJKOrder(int i, font_encoding& enc, bool& active) const;
};

class UsedFont {
private:
	BString fFamily;
	BString fStyle;
	float   fSize;

public:
	UsedFont(const char* family, const char* style, float size) : fFamily(family), fStyle(style), fSize(size) {}
	
	const char* GetFamily() const { return fFamily.String(); }
	const char* GetStyle() const { return fStyle.String(); }
	float GetSize() const { return fSize; }

	bool Equals(const char* family, const char* style, float size) const { return strcmp(GetFamily(), family) == 0 && strcmp(GetStyle(), style) == 0 && fSize == size; }
};


class UserDefinedEncodings {
public:
	UserDefinedEncodings();
	// returns true if new encoding and index pair
	bool Get(uint16 unicode, uint8 &encoding, uint8 &index);

private:
	bool IsUsed(uint16 unicode) const { return (fUsedMask[unicode / 8] & (1 << (unicode % 8))) != 0; }
	void SetUsed(uint16 unicode) { fUsedMask[unicode / 8] |= 1 << (unicode % 8); }
	
	uint8 fUsedMask[65536/8]; // exists an encoding and index for this code point?
	uint8 fEncoding[65536];   // the encoding for each code point
	uint8 fIndex[65536];      // the index for each code point
	
	uint8 fCurrentEncoding;
	uint8 fCurrentIndex;
};

#endif // FONTS_H
