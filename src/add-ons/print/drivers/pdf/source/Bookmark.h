#ifndef _BOOKMARK_H
#define _BOOKMARK_H

#include <String.h>
#include <Font.h>
#include <List.h>

class PDFWriter;

const int kMaxBookmarkLevels = 10;

class Bookmark {
	class Definition {
	public:
		int   fLevel;
		BFont fFont;
		bool  fExpanded;
		
		Definition(int level, BFont* font, bool expanded);
		bool  Matches(font_family* family, font_style* style, float size) const;
	};
	
	class Outline {
	public:
		BPoint     fStart;
		float      fHeight;
		BString    fText;
		Definition *fDefinition;
		
		Outline(BPoint start, float height, const char* text, Definition* definition) : fStart(start), fHeight(height), fText(text), fDefinition(definition) { }

		int         Level() const    { return fDefinition->fLevel; }
		bool        Expanded() const { return fDefinition->fExpanded; }
		const char* Text() const     { return fText.String(); }
		BPoint      Start() const    { return fStart; }
		float       Height() const   { return fHeight; }
	};
	
	PDFWriter*         fWriter;
	TList<Definition>  fDefinitions;	
	TList<Outline>     fOutlines; // of the current page

	int                fLevels[kMaxBookmarkLevels+1];

	bool Exists(const char* family, const char* style) const;
	Definition* Find(BFont* font) const;

	static int AscendingByStart(const Outline** a, const Outline** b);

public:
	
	Bookmark(PDFWriter* writer);

	// level starts with 1	
	void AddDefinition(int level, BFont* font, bool expanded);
	void AddBookmark(BPoint start, float height, const char* text, BFont* font);
	bool Read(const char* name); // adds definitions from file
	void CreateBookmarks();
};

#endif
