#include <Debug.h>
#include "PDFWriter.h"
#include "Bookmark.h"
#include "Scanner.h"
#include "Report.h"


Bookmark::Definition::Definition(int level, BFont* font, bool expanded)
	: fLevel(level)
	, fFont(*font)
	, fExpanded(expanded)
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


Bookmark::Definition* Bookmark::Find(BFont* font) const
{
	font_family family;
	font_style  style;
	float       size;

	font->GetFamilyAndStyle(&family, &style);
	size = font->Size();
	
	for (int i = 0; i < fDefinitions.CountItems(); i++) {
		Definition* definition = fDefinitions.ItemAt(i);
		if (definition->Matches(&family, &style, size)) {
			return definition;
		}
	}
	return NULL;
}


void Bookmark::AddDefinition(int level, BFont* font, bool expanded)
{
	ASSERT(1 <= level && level <= kMaxBookmarkLevels);
	if (Find(font) == NULL) {
		fDefinitions.AddItem(new Definition(level, font, expanded));
	}
}


void Bookmark::AddBookmark(BPoint start, float height, const char* text, BFont* font)
{
	Definition* definition = Find(font);
	if (definition != NULL) {
		fOutlines.AddItem(new Outline(start, height, text, definition));
	}
}


int Bookmark::AscendingByStart(const Outline** a, const Outline** b) {
	return (int)((*b)->Start().y - (*a)->Start().y);
}


void Bookmark::CreateBookmarks() {
	char optList[256];
	fOutlines.SortItems(AscendingByStart);

	for (int i = 0; i < fOutlines.CountItems(); i++) {
		BString ucs2;

		Outline* o = fOutlines.ItemAt(i);		
		REPORT(kInfo, fWriter->fPage, "Bookmark '%s' at level %d", o->Text(), o->Level());

		sprintf(optList, "type=fixed left=%f top=%f", o->Start().x, o->Start().y + o->Height());
		PDF_set_parameter(fWriter->fPdf, "bookmarkdest", optList);
		
		fWriter->ToPDFUnicode(o->Text(), ucs2);

		int open = o->Expanded() ? 1 : 0;
		int bookmark = PDF_add_bookmark(fWriter->fPdf, ucs2.String(), fLevels[o->Level()-1], open);
		
		if (bookmark < 0) bookmark = 0;
		
		for (int i = o->Level(); i < kMaxBookmarkLevels; i ++) {
			fLevels[i] = bookmark;
		} 
	}
	// reset to default
	PDF_set_parameter(fWriter->fPdf, "bookmarkdest", "type=fitwindow");

	fOutlines.MakeEmpty();
}

// Reads bookmark definitions from file

/*
File Format: Definition.
Line comment starts with '#'.

Definition = Version { Font }.
Version    = "Bookmarks" "2.0".
Font       = Level Family Style Size Expanded.
Level      = int.
Family     = String.
Style      = String.
Size       = float.
Expanded   = "expanded" | "collapsed". // new in version 2.0, version 1.0 defaults to collapsed
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
		BString s; float version; bool ok;
		ok = scnr.ReadName(&s) && scnr.ReadFloat(&version);
		if (!ok || strcmp(s.String(), "Bookmarks") != 0 || (version != 1.0 && version != 2.0) ) {
			REPORT(kError, 0, "Bookmarks (line %d, column %d): '%s' not a bookmarks file or wrong version!", scnr.Line(), scnr.Column(), name);
			return false;
		}
		
		while (!scnr.IsEOF()) {
			float   level, size;
			bool    expanded = false;
			BString family, style, expand;
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
			if (version == 2.0) {
				if (!scnr.ReadName(&expand) || (strcmp(expand.String(), "expanded") != 0 && strcmp(expand.String(), "collapsed") != 0)) {
					REPORT(kError, 0, "Bookmarks (line %d, column %d): Invalid expanded value", scnr.Line(), scnr.Column());
					return false;
				}
				expanded = strcmp(expand.String(), "expanded") == 0;
			}
			
			if (Exists(family.String(), style.String())) {
				BFont font;
				font.SetFamilyAndStyle(family.String(), style.String());
				font.SetSize(size);
			
				AddDefinition((int)level, &font, expanded);
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
