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
		Definition(int level, BFont* font);
		bool  Matches(font_family* family, font_style* style, float size) const;
	};
	
	class Outline {
	public:
		BPoint  fStart;
		BString fText;
		int     fLevel;
		
		Outline(BPoint start, const char* text, int level) : fStart(start), fText(text), fLevel(level) { }
		int         Level() const { return fLevel; }
		const char* Text() const  { return fText.String(); }
		BPoint      Start() const { return fStart; }
	};
	
	PDFWriter*         fWriter;
	TList<Definition>  fDefinitions;	
	TList<Outline>     fOutlines; // of the current page

	int                fLevels[kMaxBookmarkLevels+1];

	bool Exists(const char* family, const char* style) const;
	bool Find(BFont* font, int &level) const;

	static int AscendingByStart(const Outline** a, const Outline** b);

public:
	
	Bookmark(PDFWriter* writer);

	// level starts with 1	
	void AddDefinition(int level, BFont* font);
	void AddBookmark(BPoint start, const char* text, BFont* font);
	bool Read(const char* name); // adds definitions from file
	void CreateBookmarks();
};

#endif
