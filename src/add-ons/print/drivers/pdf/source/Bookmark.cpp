#include <Debug.h>
#include "PDFWriter.h"
#include "Bookmark.h"
#include "Scanner.h"
#include "Report.h"


Bookmark::Definition::Definition(int level, BFont* font)
	: fLevel(level)
	, fFont(*font)
{	
}


bool Bookmark::Definition::Matches(font_family* family, font_style* style, float size) const
{
	font_family family0;
	font_style  style0;
	
	if (fFont.Size() != size) return false;
	
	fFont.GetFamilyAndStyle(&family0, &style0);

	return strcmp(family0, *family) == 0 &&
		strcmp(style0, *style) == 0;
}


Bookmark::Bookmark(PDFWriter* writer)
	: fWriter(writer)
{
	for (int i = 0; i < kMaxBookmarkLevels; i ++) {
		fLevels[i] = 0;
	}
}


bool Bookmark::Find(BFont* font, int& level) const
{
	font_family family;
	font_style  style;
	float       size;

	font->GetFamilyAndStyle(&family, &style);
	size = font->Size();
	
	for (int i = 0; i < fDefinitions.CountItems(); i++) {
		Definition* d = fDefinitions.ItemAt(i);
		if (d->Matches(&family, &style, size)) {
			level = d->fLevel;
			return true;
		}
	}
	return false;
}


void Bookmark::AddDefinition(int level, BFont* font)
{
	ASSERT(1 <= level && level <= kMaxBookmarkLevels);
	if (!Find(font, level)) {
		fDefinitions.AddItem(new Definition(level, font));
	}
}


void Bookmark::AddBookmark(BPoint start, const char* text, BFont* font)
{
	int level;
	if (Find(font, level)) {
		fOutlines.AddItem(new Outline(start, text, level));
	}
}


int Bookmark::AscendingByStart(const Outline** a, const Outline** b) {
	return (int)((*b)->Start().y - (*a)->Start().y);
}


void Bookmark::CreateBookmarks() {
	fOutlines.SortItems(AscendingByStart);

	for (int i = 0; i < fOutlines.CountItems(); i++) {
		BString ucs2;

		Outline* o = fOutlines.ItemAt(i);		
		REPORT(kInfo, fWriter->fPage, "Bookmark '%s' at level %d", o->Text(), o->Level());
		
		fWriter->ToPDFUnicode(o->Text(), ucs2);

		int bookmark = PDF_add_bookmark(fWriter->fPdf, ucs2.String(), fLevels[o->Level()-1], 1);
		
		if (bookmark < 0) bookmark = 0;
		
		for (int i = o->Level(); i < kMaxBookmarkLevels; i ++) {
			fLevels[i] = bookmark;
		} 
	}

	fOutlines.MakeEmpty();
}

// Reads bookmark definitions from file

/*
File Format: Definition.
Line comment starts with '#'.

Definition = Version { Font }.
Version    = "Bookmarks" "1.0".
Font       = Level Family Style Size.
Level      = int.
Family     = String.
Style      = String.
Size       = float.
String     = '"' string '"'.
*/

bool Bookmark::Exists(const char* f, const char* s) const {
	font_family family;
	font_style  style;
	uint32      flags;
	int32       nFamilies;
	int32       nStyles;
	
	nFamilies = count_font_families();
	
	for (int32 i = 0; i < nFamilies; i ++) {
		if (get_font_family(i, &family, &flags) == B_OK && strcmp(f, family) == 0) {
			nStyles = count_font_styles(family);
			for (int32 j = 0; j < nStyles; j++) {
				if (get_font_style(family, j, &style, &flags) == B_OK && strcmp(s, style) == 0) {
					return true;
				}
			}
		}
	}
	return false;
}


bool Bookmark::Read(const char* name) {
	Scanner scnr(name);
	if (scnr.InitCheck() == B_OK) {
		BString s; float f; bool ok;
		ok = scnr.ReadName(&s) && scnr.ReadFloat(&f);
		if (!ok || strcmp(s.String(), "Bookmarks") != 0 || f != 1.0) {
			REPORT(kError, 0, "Bookmarks (line %d, column %d): '%s' not a bookmarks file or wrong version!", scnr.Line(), scnr.Column(), name);
			return false;
		}
		
		while (!scnr.IsEOF()) {
			float   level, size;
			BString family, style;
			if (!(scnr.ReadFloat(&level) && level >= 1.0 && level <= 10.0)) {
				REPORT(kError, 0, "Bookmarks (line %d, column %d): Invalid level", scnr.Line(), scnr.Column());
				return false;
			}
			if (!scnr.ReadString(&family)) {
				REPORT(kError, 0, "Bookmarks (line %d, column %d): Invalid font family", scnr.Line(), scnr.Column());
				return false;
			}
			if (!scnr.ReadString(&style)) {
				REPORT(kError, 0, "Bookmarks (line %d, column %d): Invalid font style", scnr.Line(), scnr.Column());
				return false;
			}
			if (!scnr.ReadFloat(&size)) {
				REPORT(kError, 0, "Bookmarks (line %d, column %d): Invalid font size", scnr.Line(), scnr.Column());
				return false;
			}
			
			if (Exists(family.String(), style.String())) {
				BFont font;
				font.SetFamilyAndStyle(family.String(), style.String());
				font.SetSize(size);
			
				AddDefinition((int)level, &font);
			} else {
				REPORT(kWarning, 0, "Bookmarks (line %d, column %d): Font %s-%s not available!", scnr.Line(), scnr.Column(), family.String(), style.String());
			}
			
			scnr.SkipSpaces();
		}
		return true;
	} else {
		REPORT(kError, 0, "Bookmarks: Could not open bookmarks file '%d'", name);
	}
	return false;
}
