/*

	PDFWriter.cpp

Copyright (c) 2001, 2002 OpenBeOS. 

Authors: 
	Philippe Houdoin
	Simon Gauvin	
	Michael Pfeiffer
	
Based on:
 - gdevbjc.c from Aladdin GhostScript
 - LMDriver.cpp from Ryan Lockhart Lexmark 5/7xxxx BeOS driver 

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

#include <stdio.h>
#include <string.h>			// for memset()
#include <math.h>
#include <ctype.h>

#include <Debug.h>
#include <StorageKit.h>
#include <TranslationKit.h>
#include <support/UTF8.h>

#include "PDFWriter.h"
#include "Bezier.h"
#include "LinePathBuilder.h"
#include "DrawShape.h"
#include "Log.h"
#include "pdflib.h"
#include "Bookmark.h"
#include "XReferences.h"
#include "Report.h"

// Constructor & destructor
// ------------------------

PDFWriter::PDFWriter()
	:	PrinterDriver()
	,   fTextLine(this)
{
	fFontSearchOrder[0] = japanese_encoding;
	fFontSearchOrder[1] = chinese_cns1_encoding;
	fFontSearchOrder[2]	= chinese_gb1_encoding;
	fFontSearchOrder[3]	= korean_encoding;
	
	fPage             = 0;
	fEmbedMaxFontSize = 250 * 1024;
	fScreen           = new BScreen();
	fFonts            = NULL;
	fBookmark         = new Bookmark(this);
	fXRefs            = new XRefDefs();
	fXRefDests        = NULL;
}


// --------------------------------------------------
PDFWriter::~PDFWriter()
{	
	if (Transport())
		CloseTransport();

	delete fScreen;
	delete fFonts;
	delete fBookmark;
	delete fXRefs;
	delete fXRefDests;
}


#ifdef CODEWARRIOR
	#pragma mark [Public methods]
#endif

// Public methods
// --------------

// --------------------------------------------------
status_t 
PDFWriter::PrintPage(int32	pageNumber, int32 pageCount)
{
	status_t	status;
	BRect		r;
	uint32 		pictureCount;
	BRect		paperRect;
	BRect 		printRect;
	BRect		*picRects;
	BPoint		*picPoints;
	BRegion		*picRegion;
	BPicture	**pictures;
	uint32		i;
	int32       orientation;
	
	status = B_OK;
	
	fPage = pageNumber;

	if (pageNumber == 1) {
		if (MakesPattern()) 
			REPORT(kDebug, fPage, ">>>>> Collecting patterns...");
		else if (MakesPDF())
			REPORT(kDebug, fPage, ">>>>> Generating PDF...");
	}
	
	paperRect = JobMsg()->FindRect("paper_rect");
	printRect = JobMsg()->FindRect("printable_rect");
	if (B_OK != JobMsg()->FindInt32("orientation", &orientation)) orientation = 0;
	if (orientation == 1) 
		printRect.Set(printRect.top, printRect.left,
			printRect.bottom, printRect.right);

	JobFile()->Read(&pictureCount, sizeof(uint32));

	pictures = (BPicture **) 	malloc(pictureCount * sizeof(BPicture *));
	picRects = (BRect *)		malloc(pictureCount * sizeof(BRect));
	picPoints = (BPoint *)		malloc(pictureCount * sizeof(BPoint));
	picRegion = new BRegion();
	
	for (i = 0; i < pictureCount; i++) {
		JobFile()->Seek(40 + sizeof(off_t), SEEK_CUR);
		JobFile()->Read(&picPoints[i], sizeof(BPoint));
		JobFile()->Read(&picRects[i], sizeof(BRect));
		pictures[i] = new BPicture();
		pictures[i]->Unflatten(JobFile());
		picRegion->Include(picRects[i]);
	}
	
	r  = picRegion->Frame();
	delete picRegion;

	BeginPage(paperRect, printRect);
	for (i = 0; i < pictureCount; i++) {
		Iterate(pictures[i]);
		delete pictures[i];
	}
	EndPage();
	
	free(pictures);
	free(picRects);
	free(picPoints);
	
	return status;
}


// --------------------------------------------------
status_t 
PDFWriter::BeginJob() 
{
	fLog = fopen("/tmp/pdf_writer.log", "w");
	
	PDF_boot();

	fPdf = PDF_new2(_ErrorHandler, NULL, NULL, NULL, this);	// set *this* as pdf cookie
	if ( fPdf == NULL )
		return B_ERROR;

	// load font embedding settings
	fFonts = new Fonts();
	fFonts->CollectFonts();
	BMessage fonts;
	if (B_OK == JobMsg()->FindMessage("fonts", &fonts)) {
		fFonts->SetTo(&fonts);
	}
	
	// set font search order
	int           j = 0; 
	font_encoding enc; 
	bool          active;
	
	for (int i = 0; j < no_of_cjk_encodings && fFonts->GetCJKOrder(i, enc, active); i ++) {
		if (active) fFontSearchOrder[j++] = enc;
	}
	for (; j < no_of_cjk_encodings; j ++) {
		fFontSearchOrder[j] = invalid_encoding;
	}
		
	return InitWriter();
}


// --------------------------------------------------
status_t 
PDFWriter::EndJob() 
{
#ifdef DEBUG
	Report* r = Report::Instance();
	int32 n = r->CountItems();
	fprintf(fLog, "Report:\n");
	fprintf(fLog, "=======\n");
	if (n == 0) {
		fprintf(fLog, "<empty>\n");
	}
	for (int32 i = 0; i < n; i++) {
		ReportRecord* rr = r->ItemAt(i);
		switch (rr->Kind()) {
			case kInfo:    fprintf(fLog, "Info"); break;
			case kWarning: fprintf(fLog, "Warning"); break;
			case kError:   fprintf(fLog, "Error"); break;
			case kDebug:   fprintf(fLog, "Debug"); break;
		}
		fprintf(fLog, " %s", rr->Label());
		if (rr->Page() > 0) fprintf(fLog, " (Page %d)", (int)rr->Page());
		fprintf(fLog, ": %s\n", rr->Desc()); 
	}
#endif

	PDF_close(fPdf);
	REPORT(kDebug, 0, ">>>> PDF_close");

   	PDF_delete(fPdf);
    PDF_shutdown();

	fclose(fLog);
	return B_OK;
}


// --------------------------------------------------
void 
PDFWriter::SetAttribute(const char* name, const char* value) {
	BFile* file = dynamic_cast<BFile*>(Transport());
	if (file) {
		
	}
}


// --------------------------------------------------
status_t 
PDFWriter::InitWriter()
{
	char	buffer[512];
	BString s;

	const char * compatibility;
	if (JobMsg()->FindString("pdf_compatibility", &compatibility) == B_OK) {
		PDF_set_parameter(fPdf, "compatibility", compatibility);
	}

	REPORT(kDebug, 0, ">>>> PDF_open_mem");	
	PDF_open_mem(fPdf, _WriteData);	// use callback to stream PDF document data to printer transport

	PDF_set_parameter(fPdf, "flush", "heavy");

	// set document info
	BMessage doc;
	bool setTitle = true;
	bool setCreator = true;
	if (JobMsg()->FindMessage("doc_info", &doc) == B_OK) {
#ifndef B_BEOS_VERSION_DANO	
		char *name;
#else
		const char *name;
#endif		
		uint32 type;
		int32 count;
		for (int32 i = 0; doc.GetInfo(B_STRING_TYPE, i, &name, &type, &count) != B_BAD_INDEX; i++) {
			if (type == B_STRING_TYPE) {
				BString value;
				if (doc.FindString(name, &value) == B_OK && value != "") {
					SetAttribute(name, value.String());
					BString s;
					ToPDFUnicode(value.String(), s);
					PDF_set_info(fPdf, name, s.String());
				}
			}
		}
		BString s;
		if (doc.FindString("Title", &s) == B_OK && s != "") setTitle = false;
		if (doc.FindString("Creator", &s) == B_OK && s != "") setCreator = false;
	}
		
	// find job title 
	if (setTitle && JobFile()->ReadAttr("_spool/Description", B_STRING_TYPE, 0, buffer, sizeof(buffer))) {
		SetAttribute("Title", buffer);
	    ToPDFUnicode(buffer, s); PDF_set_info(fPdf, "Title", s.String());
	}
				
	// find job creator
	if (setCreator && JobFile()->ReadAttr("_spool/MimeType", B_STRING_TYPE, 0, buffer, sizeof(buffer))) {
		SetAttribute("Creator", buffer);
	    ToPDFUnicode(buffer, s); PDF_set_info(fPdf, "Creator", s.String());
	}
	
	int32 compression;
	if (JobMsg()->FindInt32("pdf_compression", &compression) == B_OK) {
	    PDF_set_value(fPdf, "compress", compression);
	}

    // PDF_set_parameter(fPdf, "warning", "false");

	PDF_set_parameter(fPdf, "fontwarning", "false");
	// PDF_set_parameter(fPdf, "native-unicode", "true");

	REPORT(kDebug, 0, "Start of fonts declaration:");	

	PDF_set_parameter(fPdf, "Encoding", "t1enc0==/boot/home/config/settings/PDF Writer/t1enc0.enc");
	PDF_set_parameter(fPdf, "Encoding", "t1enc1==/boot/home/config/settings/PDF Writer/t1enc1.enc");
	PDF_set_parameter(fPdf, "Encoding", "t1enc2==/boot/home/config/settings/PDF Writer/t1enc2.enc");
	PDF_set_parameter(fPdf, "Encoding", "t1enc3==/boot/home/config/settings/PDF Writer/t1enc3.enc");
	PDF_set_parameter(fPdf, "Encoding", "t1enc4==/boot/home/config/settings/PDF Writer/t1enc4.enc");

	PDF_set_parameter(fPdf, "Encoding", "ttenc0==/boot/home/config/settings/PDF Writer/ttenc0.cpg");
	PDF_set_parameter(fPdf, "Encoding", "ttenc1==/boot/home/config/settings/PDF Writer/ttenc1.cpg");
	PDF_set_parameter(fPdf, "Encoding", "ttenc2==/boot/home/config/settings/PDF Writer/ttenc2.cpg");
	PDF_set_parameter(fPdf, "Encoding", "ttenc3==/boot/home/config/settings/PDF Writer/ttenc3.cpg");
	PDF_set_parameter(fPdf, "Encoding", "ttenc4==/boot/home/config/settings/PDF Writer/ttenc4.cpg");

	DeclareFonts();

	REPORT(kDebug, fPage, "End of fonts declaration.");	

	// Links
	float width;
	if (JobMsg()->FindFloat("link_border_width", &width) != B_OK) {
		width = 1;
	}
	PDF_set_border_style(fPdf, "solid", width);
	
	if (JobMsg()->FindBool("create_web_links", &fCreateWebLinks) != B_OK) {
		fCreateWebLinks = false;
	}

	// Bookmarks	
	if (JobMsg()->FindBool("create_bookmarks", &fCreateBookmarks) != B_OK) {
		fCreateBookmarks = false;
	}

	if (fCreateBookmarks) {
		BString name;
		if (JobMsg()->FindString("bookmark_definition_file", &name) == B_OK && LoadBookmarkDefinitions(name.String())) {
		} else {
			fCreateBookmarks = false;
		}
	}	

	// Cross References
	if (JobMsg()->FindBool("create_xrefs", &fCreateXRefs) != B_OK) {
		fCreateXRefs = false;
	}

	if (fCreateXRefs) {
		BString name;
		if (JobMsg()->FindString("xrefs_file", &name) == B_OK && LoadXRefsDefinitions(name.String())) {
		} else {
			REPORT(kError, 0, "Could not read xrefs file!");
			fCreateXRefs = false;
		}
	}	

	fState = NULL;
	fStateDepth = 0;

	return B_OK;
}


// --------------------------------------------------
status_t
PDFWriter::DeclareFonts()
{
	char buffer[1024];
	char *parameter_name;

	for (int i = 0; i < fFonts->Length(); i++) {
		FontFile* f = fFonts->At(i);
//		LOG((fLog, "path= %s\n", f->Path()));		
		if (f->Type() == true_type_type) {
			parameter_name = "FontOutline";
		} else { // f->Type() == type1_type
			if (strstr(f->Path(), ".afm"))
				parameter_name = "FontAFM";
			else if (strstr(f->Path(), ".pfm"))
				parameter_name = "FontPFM";
			else // should not reach here! 
				continue;		
		}
#ifndef __POWERPC__						
		snprintf(buffer, sizeof(buffer), "%s==%s", f->Name(), f->Path());
#else
		sprintf(buffer, "%s==%s", f->Name(), f->Path());
#endif
//		LOG((fLog, "%s: %s\n", parameter_name, buffer));
	
		PDF_set_parameter(fPdf, parameter_name, buffer);
	}
	return B_OK;
}

bool
PDFWriter::LoadBookmarkDefinitions(const char* name)
{
	// TODO: use B_USER_SETTINGS_DIRECTORY instead of hard coded constant
	BString path("/boot/home/config/settings/PDF Writer/bookmarks/");
	path.Append(name);

	return fBookmark->Read(path.String());
}


bool
PDFWriter::LoadXRefsDefinitions(const char* name)
{
	// TODO: use B_USER_SETTINGS_DIRECTORY instead of hard coded constant
	BString path("/boot/home/config/settings/PDF Writer/xrefs/");
	path.Append(name);

	if (fXRefs->Read(path.String())) {
		fXRefDests = new XRefDests(fXRefs->Count());
		return true;
	}
	return false;
}


// --------------------------------------------------
status_t 
PDFWriter::BeginPage(BRect paperRect, BRect printRect)
{
	float width = paperRect.Width() < 10 ? a4_width : paperRect.Width();
	float height = paperRect.Height() < 10 ? a4_height : paperRect.Height();
	
	fMode = kDrawingMode;
	
	ASSERT(fState == NULL);
	fState = new State(height, printRect.left, printRect.top);

    if (MakesPDF())
    	PDF_begin_page(fPdf, width, height);

	REPORT(kDebug, fPage, ">>>> PDF_begin_page [%f, %f]", width, height);
	
	if (MakesPDF())
		PDF_initgraphics(fPdf);
	
	fState->penX = 0;
	fState->penY = 0;

	// XXX should we clip to the printRect here?

	PushState(); // so that fState->prev != NULL

	return B_OK;
}


// --------------------------------------------------
status_t 
PDFWriter::EndPage()
{	
	fTextLine.Flush();
	if (fCreateBookmarks) fBookmark->CreateBookmarks();

	while (fState->prev != NULL) PopState();

    if (MakesPDF())
    	PDF_end_page(fPdf);
	REPORT(kDebug, fPage, ">>>> PDF_end_page");

	delete fState; fState = NULL;
	
	return B_OK;
}


#ifdef CODEWARRIOR
	#pragma mark [PDFlib callbacks]
#endif

// --------------------------------------------------
size_t 
PDFWriter::WriteData(void *data, size_t	size)
{
	REPORT(kDebug, fPage, ">>>> WriteData %p, %ld", data, size);

	return Transport()->Write(data, size);
}


// --------------------------------------------------
void 
PDFWriter::ErrorHandler(int	type, const char *msg)
{
	REPORT(kError, fPage, "PDFlib %d: %s", type, msg);
}

#ifdef CODEWARRIOR
	#pragma mark [Generic drawing support routines]
#endif


// --------------------------------------------------
void
PDFWriter::PushInternalState() 
{
	REPORT(kDebug, fPage, "PushInternalState");
	fState = new State(fState); fStateDepth ++;
}


// --------------------------------------------------
bool
PDFWriter::PopInternalState() 
{
	REPORT(kDebug, fPage, "PopInternalState");
	if (fStateDepth != 0) {
		State* s = fState; fStateDepth --;
		fState = fState->prev;
		delete s;
//		LOG((fLog, "height = %f x0 = %f y0 = %f", pdfSystem->Height(), pdfSystem->Origin().x, pdfSystem.Origin().y));
		return true;
	} else {
		REPORT(kDebug, fPage, "State stack underflow!");
		return false;
	}
}


// --------------------------------------------------
void 
PDFWriter::SetColor(rgb_color color) 
{
	if (!MakesPDF()) return;
	if (fState->currentColor.red != color.red || 
		fState->currentColor.blue != color.blue || 
		fState->currentColor.green != color.green || 
		fState->currentColor.alpha != color.alpha) {
		fState->currentColor = color;
		float red   = color.red / 255.0;
		float green = color.green / 255.0;
		float blue  = color.blue / 255.0;
		PDF_setcolor(fPdf, "both", "rgb", red, green, blue, 0.0);	
	}
}


// --------------------------------------------------
int
PDFWriter::FindPattern() 
{
	// TODO: use hashtable instead of BList for fPatterns
	const int n = fPatterns.CountItems();
	for (int i = 0; i < n; i ++) {
		Pattern* p = fPatterns.ItemAt(i);
		if (p->Matches(fState->pattern0, fState->backgroundColor, fState->foregroundColor)) return p->patternId;
	}
	return -1;
}


// --------------------------------------------------
void
PDFWriter::CreatePattern() 
{
	REPORT(kDebug, fPage, "CreatePattern");
#if 1
	// use filled pathes for pattern
	// pro: see con of bitmap pattern
	// con: not rendered (correctly) in BePDF with small zoom factor
	int pattern = PDF_begin_pattern(fPdf, 8, 8, 8, 8, 1);
	
	if (pattern == -1) {
		REPORT(kError, fPage, "CreatePattern could not create pattern");
		return;
	}
	
	for (int pass = 0; pass < 2; pass ++) {
		float r, g, b;
		bool is_transparent;
	
		if (pass == 0) {
			r =  fState->foregroundColor.red / 255.0;
			g =  fState->foregroundColor.green / 255.0;
			b =  fState->foregroundColor.blue / 255.0;
			is_transparent = fState->foregroundColor.alpha < 128;
		} else {
			r =  fState->backgroundColor.red / 255.0;
			g =  fState->backgroundColor.green / 255.0;
			b =  fState->backgroundColor.blue / 255.0;
			is_transparent = fState->backgroundColor.alpha < 128;
		}
	
		PDF_setcolor(fPdf, "fill", "rgb", r, g, b, 0);
	
		if (is_transparent) continue;
		
		uint8* data = (uint8*)fState->pattern0.data;
		for (int8 y = 0; y <= 7; y ++, data ++) {
			uint8  d = *data;
			for (int8 x = 0; x <= 7; x ++, d >>= 1) {
				if (d & 1 == 1) { // foreground
					if (pass != 0) continue;
				} else { // background
					if (pass != 1) continue;
				}
				
				// create rectangle for pixel
				float x1 = x + 1, y1 = y + 1;
				PDF_moveto(fPdf, x, y);
				PDF_lineto(fPdf, x, y1);
				PDF_lineto(fPdf, x1, y1);
				PDF_lineto(fPdf, x1, y);
				PDF_closepath(fPdf);
				PDF_fill(fPdf);
			} 
		}
	}
	
	PDF_end_pattern(fPdf);

#else
	// use bitmap for pattern
	// pro: BePDF renders pattern also with small zoom factor
	// con: BePDF omits mask during rendering
	BBitmap bm(BRect(0, 0, 7, 7), B_RGBA32);
	
	uint8* data = (uint8*)fState->pattern0.data;
	rgb_color h = fState->foregroundColor;
	rgb_color l = fState->backgroundColor;
	if (h.alpha < 128) {
		h.red = h.green = h.blue = 255;
	}
	if (l.alpha < 128) {
		l.red = l.green = l.blue = 255;
	}

	uint8* row = (uint8*)bm.Bits();
	for (int8 y = 0; y <= 7; y ++, data ++) {
		uint8  d = *data;
		uint8* b = row; row += bm.BytesPerRow();
		for (int8 x = 0; x <= 7; x ++, d >>= 1, b += 4) {
			if (d & 1 == 1) {
				b[2] = h.red;   //fState->foregroundColor.red;
				b[1] = h.green; //fState->foregroundColor.green;
				b[0] = h.blue;  //fState->foregroundColor.blue;
				b[3] = h.alpha; //fState->foregroundColor.alpha;
			} else {
				b[2] = l.red;   //fState->backgroundColor.red;
				b[1] = l.green; //fState->backgroundColor.green;
				b[0] = l.blue;  //fState->backgroundColor.blue;
				b[3] = l.alpha; //fState->backgroundColor.alpha;
			}
		} 
	}

	int mask, image;
		
	if (!GetImages(bm.Bounds(), 8, 8, bm.BytesPerRow(), bm.ColorSpace(), 0, bm.Bits(), &mask, &image)) {
		return;
	}

	int pattern = PDF_begin_pattern(fPdf, 8, 8, 8, 8, 1);
	if (pattern == -1) {
		REPORT(kError, fPage, "CreatePattern could not create pattern");
		PDF_close_image(fPdf, image);
		if (mask != -1) PDF_close_image(fPdf, mask);
		return;
	}
	PDF_setcolor(fPdf, "both", "rgb", 0, 0, 1, 0);
	PDF_place_image(fPdf, image, 0, 0, 1);
	PDF_end_pattern(fPdf);
	PDF_close_image(fPdf, image);
	if (mask != -1) PDF_close_image(fPdf, mask);
#endif
	
	Pattern* p = new Pattern(fState->pattern0, fState->backgroundColor, fState->foregroundColor, pattern);
	fPatterns.AddItem(p);
}


// --------------------------------------------------
void
PDFWriter::SetPattern() 
{
	REPORT(kDebug, fPage, "SetPattern (bitmap version)");
	if (MakesPattern()) {
		if (FindPattern() == -1) CreatePattern();
	} else {
		int pattern = FindPattern();
		if (pattern != -1) {
			PDF_setcolor(fPdf, "both", "pattern", pattern, 0, 0, 0);
		} else {
			// TODO: fall back to another method
			REPORT(kError, fPage, "pattern missing!");
		}
	}
}


// --------------------------------------------------
void 
PDFWriter::StrokeOrClip() 
{
	if (IsDrawing()) {
		PDF_stroke(fPdf);
	} else {
		REPORT(kError, fPage, "Clipping not implemented for this primitive!!!");
		PDF_closepath(fPdf);
	}
}


// --------------------------------------------------
void 
PDFWriter::FillOrClip() 
{
	if (IsDrawing()) {
		PDF_fill(fPdf);
	} else {
		PDF_closepath(fPdf);
	}
}


// --------------------------------------------------
void
PDFWriter::Paint(bool stroke) 
{
	if (stroke) {
		StrokeOrClip();
	} else {
		FillOrClip();
	}
}


// --------------------------------------------------
bool 
PDFWriter::IsSame(const pattern &p1, const pattern &p2)
{
	char *a = (char*)p1.data;
	char *b = (char*)p2.data;
	return strncmp(a, b, 8) == 0;
}


// --------------------------------------------------
bool 
PDFWriter::IsSame(const rgb_color &c1, const rgb_color &c2)
{
	char *a = (char*)&c1;
	char *b = (char*)&c1;
	return strncmp(a, b, sizeof(rgb_color)) == 0;
}


// --------------------------------------------------
void 
PDFWriter::SetColor() 
{
	if (IsSame(fState->pattern0, B_SOLID_HIGH)) {
		SetColor(fState->foregroundColor);
	} else if (IsSame(fState->pattern0, B_SOLID_LOW)) {
		SetColor(fState->backgroundColor);
	} else {
		SetPattern();
	} 
}


#ifdef CODEWARRIOR
	#pragma mark [Image drawing support routines]
#endif


// --------------------------------------------------
int32 
PDFWriter::BytesPerPixel(int32 pixelFormat) 
{
	switch (pixelFormat) {
		case B_RGB32:      // fall through
		case B_RGB32_BIG:  // fall through
		case B_RGBA32:     // fall through
		case B_RGBA32_BIG: return 4;

		case B_RGB24_BIG:  // fall through
		case B_RGB24:      return 3;

		case B_RGB16:      // fall through
		case B_RGB16_BIG:  // fall through
		case B_RGB15:      // fall through
		case B_RGB15_BIG:  // fall through
		case B_RGBA15:     // fall through
		case B_RGBA15_BIG: return 2;

		case B_GRAY8:      // fall through
		case B_CMAP8:      return 1;
		case B_GRAY1:      return 0;
		default: return -1;
	}
}


// --------------------------------------------------
bool 
PDFWriter::NeedsAlphaCheck(int32 pixelFormat) 
{
	switch (pixelFormat) {
		case B_RGB32:      // fall through
		case B_RGB32_BIG:  // fall through
		case B_RGBA32:     // fall through
		case B_RGBA32_BIG: // fall through
//		case B_RGB24:      // fall through
//		case B_RGB24_BIG:  // fall through
//		case B_RGB16:      // fall through
//		case B_RGB16_BIG:  // fall through
		case B_RGB15:      // fall through
		case B_RGB15_BIG:  // fall through
		case B_RGBA15:     // fall through
		case B_RGBA15_BIG: // fall through
		case B_CMAP8:      return true;
		default: return false;
	}
}


// --------------------------------------------------
bool 
PDFWriter::IsTransparentRGB32(uint8* in) 
{
	return *((uint32*)in) == B_TRANSPARENT_MAGIC_RGBA32;
}


// --------------------------------------------------
bool 
PDFWriter::IsTransparentRGB32_BIG(uint8* in) 
{
	return *(uint32*)in == B_TRANSPARENT_MAGIC_RGBA32_BIG;
}


// --------------------------------------------------
bool 
PDFWriter::IsTransparentRGBA32(uint8* in) 
{
	return in[3] < 128 || IsTransparentRGB32(in);
}


// --------------------------------------------------
bool 
PDFWriter::IsTransparentRGBA32_BIG(uint8* in) 
{
	return in[0] < 127 || IsTransparentRGB32_BIG(in);
}

/*
// --------------------------------------------------
bool 
PDFWriter::IsTransparentRGB24(uint8* in) 
{
	return false;
}


// --------------------------------------------------
bool 
PDFWriter::IsTransparentRGB24_BIG(uint8* in) 
{
}


// --------------------------------------------------
bool 
PDFWriter::IsTransparentRGB16(uint8* in) 
{
	return false;
}


// --------------------------------------------------
bool 
PDFWriter::IsTransparentRGB16_BIG(uint8* in) 
{
}
*/


// --------------------------------------------------
bool 
PDFWriter::IsTransparentRGB15(uint8* in) 
{
	return *((uint16*)in) == B_TRANSPARENT_MAGIC_RGBA15;
}


// --------------------------------------------------
bool 
PDFWriter::IsTransparentRGB15_BIG(uint8* in) 
{
	// 01234567 01234567
	// 00123434 01201234
	// -RRRRRGG GGGBBBBB
	return *(uint16*)in == B_TRANSPARENT_MAGIC_RGBA15_BIG;
}


// --------------------------------------------------
bool 
PDFWriter::IsTransparentRGBA15(uint8* in) 
{
	// 01234567 01234567
	// 01201234 00123434
	// GGGBBBBB ARRRRRGG
	return in[1] & 1 == 0 || IsTransparentRGB15(in);
}


// --------------------------------------------------
bool 
PDFWriter::IsTransparentRGBA15_BIG(uint8* in) 
{
	// 01234567 01234567
	// 00123434 01201234
	// ARRRRRGG GGGBBBBB
	return in[0] & 1 == 0 || IsTransparentRGB15_BIG(in);
}


// --------------------------------------------------
bool 
PDFWriter::IsTransparentCMAP8(uint8* in) 
{
	return *in == B_TRANSPARENT_MAGIC_CMAP8;
}


/*
// --------------------------------------------------
bool 
PDFWriter::IsTransparentGRAY8(uint8* in)
{
}


// --------------------------------------------------
bool 
PDFWriter::IsTransparentGRAY1(uint8* in, int8 bit)
{
}
*/


// --------------------------------------------------
void *
PDFWriter::CreateMask(BRect src, int32 bytesPerRow, int32 pixelFormat, int32 flags, void *data)
{
	uint8	*in;
	uint8   *inRow;
	int32	x, y;
	
	uint8	*mask;
	uint8	*maskRow;
	uint8	*out;
	int32	maskWidth;
	uint8	shift;
	bool	alpha;
	int32   bpp = 4; 

	bpp = BytesPerPixel(pixelFormat);	
	if (bpp < 0) 
		return NULL;
	
	int32	width = src.IntegerWidth() + 1;
	int32	height = src.IntegerHeight() + 1;
		
	if (!NeedsAlphaCheck(pixelFormat))
		return NULL;

	// Image Mask
	inRow = (uint8 *) data;
	inRow += bytesPerRow * (int) src.top + bpp * (int) src.left;

	maskWidth = (width+7)/8;
	maskRow = mask = new uint8[maskWidth * height];
	memset(mask, 0, maskWidth*height);
	alpha = false;
	
	for (y = height; y > 0; y--) {
		in = inRow;
		out = maskRow;
		shift = 7;
		
		bool a = false;			

		for (x = width; x > 0; x-- ) {
			// For each pixel
			switch (pixelFormat) {
				case B_RGB32:      a = IsTransparentRGB32(in); break;
				case B_RGB32_BIG:  a = IsTransparentRGB32_BIG(in); break;
				case B_RGBA32:     a = IsTransparentRGBA32(in); break;
				case B_RGBA32_BIG: a = IsTransparentRGBA32_BIG(in); break;
				//case B_RGB24:      a = IsTransparentRGB24(in); break;
				//case B_RGB24_BIG:  a = IsTransparentRGB24_BIG(in); break;
				//case B_RGB16:      a = IsTransparentRGB16(in); break;
				//case B_RGB16_BIG:  a = IsTransparentRGB16_BIG(in); break;
				case B_RGB15:      a = IsTransparentRGB15(in); break;
				case B_RGB15_BIG:  a = IsTransparentRGB15_BIG(in); break;
				case B_RGBA15:     a = IsTransparentRGBA15(in); break;
				case B_RGBA15_BIG: a = IsTransparentRGBA15_BIG(in); break;
				case B_CMAP8:      a = IsTransparentCMAP8(in); break;
				//case B_GRAY8:      a = IsTransparentGRAY8(in); break;
				//case B_GRAY1:      a = false; break;
				default: a = false; // should not reach here
					REPORT(kDebug, fPage, "CreateMask: non transparentable pixelFormat");
			}

			if (a) {
				out[0] |= (1 << shift);
				alpha = true;
			}
			// next pixel			
			if (shift == 0) out ++;
			shift = (shift + 7) & 0x07;		
			in += bpp;
		}

		// next row
		inRow += bytesPerRow;
		maskRow += maskWidth;
	}
	
	if (!alpha) {
		delete []mask;
		mask = NULL;
	}
	return mask;
}


// --------------------------------------------------
void 
PDFWriter::ConvertFromRGB32(uint8* in, uint8 *out)
{
	*((rgb_color*)out) = *((rgb_color*)in);
}


// --------------------------------------------------
void 
PDFWriter::ConvertFromRGBA32(uint8* in, uint8 *out)
{
	*((rgb_color*)out) = *((rgb_color*)in);
}


// --------------------------------------------------
void 
PDFWriter::ConvertFromRGB24(uint8* in, uint8 *out)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
	out[3] = 255;
}


// --------------------------------------------------
void 
PDFWriter::ConvertFromRGB16(uint8* in, uint8 *out)
{
	// 01234567 01234567
	// 01201234 01234345
	// GGGBBBBB RRRRRGGG
	out[0] = in[0] & 0xf8; // blue
	out[1] = ((in[0] & 7) << 2) | (in[1] & 0xe0); // green
	out[2] = in[1] << 3; // red
	out[3] = 255;
}


// --------------------------------------------------
void 
PDFWriter::ConvertFromRGB15(uint8* in, uint8 *out)
{
	// 01234567 01234567
	// 01201234 00123434
	// GGGBBBBB -RRRRRGG
	out[0] = in[0] & 0xf8; // blue
	out[1] = ((in[0] & 7) << 3) | (in[1] & 0xc0); // green
	out[2] = (in[1] & ~1) << 2; // red
	out[3] = 255;
}


// --------------------------------------------------
void 
PDFWriter::ConvertFromRGBA15(uint8* in, uint8 *out)
{
	// 01234567 01234567
	// 01201234 00123434
	// GGGBBBBB ARRRRRGG
	out[0] = in[0] & 0xf8; // blue
	out[1] = ((in[0] & 7) << 3) | (in[1] & 0xc0); // green
	out[2] = (in[1] & ~1) << 2; // red
	out[3] = in[1] << 7;
}


// --------------------------------------------------
void 
PDFWriter::ConvertFromCMAP8(uint8* in, uint8 *out)
{
	rgb_color c = fScreen->ColorForIndex(in[0]);
	out[0] = c.blue;
	out[1] = c.green;
	out[2] = c.red;
	out[3] = c.alpha;
}


// --------------------------------------------------
void 
PDFWriter::ConvertFromGRAY8(uint8* in, uint8 *out)
{
	out[0] = in[0];
	out[1] = in[0];
	out[2] = in[0];
	out[3] = 255;
}


// --------------------------------------------------
void 
PDFWriter::ConvertFromGRAY1(uint8* in, uint8 *out, int8 bit)
{
	uint8 gray = (in[0] & (1 << bit)) ? 255 : 0;
	out[0] = gray;
	out[1] = gray;
	out[2] = gray;
	out[3] = 255;
}


// --------------------------------------------------
void 
PDFWriter::ConvertFromRGB32_BIG(uint8* in, uint8 *out)
{
	out[0] = in[3];
	out[1] = in[2];
	out[2] = in[1];
	out[3] = 255;
}


// --------------------------------------------------
void 
PDFWriter::ConvertFromRGBA32_BIG(uint8* in, uint8 *out)
{
	out[0] = in[3];
	out[1] = in[2];
	out[2] = in[1];
	out[3] = in[0];
}


// --------------------------------------------------
void 
PDFWriter::ConvertFromRGB24_BIG(uint8* in, uint8 *out)
{
	out[0] = in[2];
	out[1] = in[1];
	out[2] = in[0];
	out[3] = 255;
}


// --------------------------------------------------
void 
PDFWriter::ConvertFromRGB16_BIG(uint8* in, uint8 *out)
{
	// 01234567 01234567
	// 01234345 01201234
	// RRRRRGGG GGGBBBBB
	out[0] = in[2] & 0xf8; // blue
	out[1] = ((in[1] & 7) << 2) | (in[0] & 0xe0); // green
	out[2] = in[0] << 3; // red
	out[3] = 255;
}


// --------------------------------------------------
void 
PDFWriter::ConvertFromRGB15_BIG(uint8* in, uint8 *out)
{
	// 01234567 01234567
	// 00123434 01201234
	// -RRRRRGG GGGBBBBB
	out[0] = in[1] & 0xf8; // blue
	out[1] = ((in[1] & 7) << 3) | (in[0] & 0xc0); // green
	out[2] = (in[0] & ~1) << 2; // red
	out[3] = 255;
}


// --------------------------------------------------
void 
PDFWriter::ConvertFromRGBA15_BIG(uint8* in, uint8 *out)
{
	// 01234567 01234567
	// 00123434 01201234
	// ARRRRRGG GGGBBBBB
	out[0] = in[1] & 0xf8; // blue
	out[1] = ((in[1] & 7) << 3) | (in[0] & 0xc0); // green
	out[2] = (in[0] & ~1) << 2; // red
	out[3] = in[0] << 7;
}


// --------------------------------------------------
// Convert and clip bits to colorspace B_RGBA32
BBitmap *
PDFWriter::ConvertBitmap(BRect src, int32 bytesPerRow, int32 pixelFormat, int32 flags, void *data)
{
	uint8	*in;
	uint8   *inLeft;
	uint8	*out;
	uint8   *outLeft;
	int32	x, y;
	int8    bit;
	int32   bpp = 4; 

	bpp = BytesPerPixel(pixelFormat);	
	if (bpp < 0) 
		return NULL;

	int32 width  = src.IntegerWidth();
	int32 height = src.IntegerHeight();
	BBitmap *	bm = new BBitmap(BRect(0, 0, width, height), B_RGB32);
	if (!bm->IsValid()) {
		delete bm;
		REPORT(kError, fPage, "BBitmap constructor failed");
		return NULL;
	}

	inLeft  = (uint8 *)data;
	inLeft += bytesPerRow * (int)src.top + bpp * (int)src.left; 
	outLeft	= (uint8*)bm->Bits();
		
	for (y = height; y >= 0; y--) {
		in = inLeft;
		out = outLeft;

		for (x = 0; x <= width; x++) {
			// For each pixel
			switch (pixelFormat) {
				case B_RGB32:      ConvertFromRGB32(in, out); break;
				case B_RGBA32:     ConvertFromRGBA32(in, out); break;
				case B_RGB24:      ConvertFromRGB24(in, out); break;
				case B_RGB16:      ConvertFromRGB16(in, out); break;
				case B_RGB15:      ConvertFromRGB15(in, out); break;
				case B_RGBA15:     ConvertFromRGBA15(in, out); break;
				case B_CMAP8:      ConvertFromCMAP8(in, out); break;
				case B_GRAY8:      ConvertFromGRAY8(in, out); break;
				case B_GRAY1:      
					bit = x & 7;
					ConvertFromGRAY1(in, out, bit);
					if (bit == 7) in ++; 
					break;
				case B_RGB32_BIG:  ConvertFromRGB32_BIG(in, out); break;
				case B_RGBA32_BIG: ConvertFromRGBA32_BIG(in, out); break;
				case B_RGB24_BIG:  ConvertFromRGB24_BIG(in, out); break;
				case B_RGB16_BIG:  ConvertFromRGB16_BIG(in, out); break;
				case B_RGB15_BIG:  ConvertFromRGB15_BIG(in, out); break;
				case B_RGBA15_BIG: ConvertFromRGBA15_BIG(in, out); break;
				default:; // should not reach here
			}
			in += bpp;	// next pixel
			out += 4;
		}

		// next row
		inLeft += bytesPerRow;
		outLeft += bm->BytesPerRow();
	}
	
	return bm;
}


// --------------------------------------------------
bool 
PDFWriter::StoreTranslatorBitmap(BBitmap *bitmap, const char *filename, uint32 type)
{
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	if (!roster) {
		REPORT(kDebug, fPage, "TranslatorRoster is NULL!");
		return false;
	}
	BBitmapStream stream(bitmap); // init with contents of bitmap
/*
	translator_info info, *i = NULL;
	if (quality != -1.0 && roster->Identify(&stream, NULL, &info) == B_OK) {
		#ifdef DEBUG
		LOG((fLog, ">>> translator_info:\n"));
		LOG((fLog, "   type = %4.4s\n", (char*)&info.type));
		LOG((fLog, "   id = %d\n", info.translator));
		LOG((fLog, "   group = %4.4s\n", (char*)&info.group));
		LOG((fLog, "   quality = %f\n", info.quality));
		LOG((fLog, "   capability = %f\n", info.type));
		LOG((fLog, "   name = %s\n", info.name));
		LOG((fLog, "   MIME = %s\n", info.MIME));
		#endif
		
		int32 numInfo;
		translator_info *tInfo = NULL;
		if (roster->GetTranslators(&stream, NULL, &tInfo, &numInfo, 
			info.type, NULL, type) == B_OK) {

//			#ifdef DEBUG
			for (int j = 0; j < numInfo; j++) {
				LOG((fLog, ">>> translator_info [%d]:\n", j));
				LOG((fLog, "   type = %4.4s\n", (char*)&tInfo[j].type));
				LOG((fLog, "   id = %d\n", tInfo[j].translator));
				LOG((fLog, "   group = %4.4s\n", (char*)&tInfo[j].group));
				LOG((fLog, "   quality = %f\n", tInfo[j].quality));
				LOG((fLog, "   capability = %f\n", tInfo[j].type));
				LOG((fLog, "   name = %s\n", tInfo[j].name));
				LOG((fLog, "   MIME = %s\n", tInfo[j].MIME));
			}
//			#endif

			BMessage m;
			status_t s = roster->GetConfigurationMessage(tInfo[0].translator, &m);
			if (s == B_OK) {
				fMessagePrinter.Print(&m);
			} else {
				LOG((fLog, "ERROR: could not get configuration message %d\n", s));
			}
			
			info = tInfo[0];
			info.quality = quality;
			delete []tInfo;

			i = &info;
		}
	}
*/
	BFile file(filename, B_CREATE_FILE | B_WRITE_ONLY | B_ERASE_FILE);
	bool res = roster->Translate(&stream, NULL /*i*/, NULL, &file, type) == B_OK;
	BBitmap *bm = NULL; stream.DetachBitmap(&bm); // otherwise bitmap destructor crashes here!
	ASSERT(bm == bitmap);
	return res;
}

// --------------------------------------------------
bool	
PDFWriter::GetImages(BRect src, int32 width, int32 height, int32 bytesPerRow, int32 pixelFormat, 
	int32 flags, void *data, int* maskId, int* image)
{
	void 	*mask = NULL;
	*maskId = -1;

	mask = CreateMask(src, bytesPerRow, pixelFormat, flags, data);

	if (mask) {
		int32 width = src.IntegerWidth() + 1;
		int32 height = src.IntegerHeight() + 1;
		int32 w = (width+7)/8;
		int32 h = height;
		*maskId = PDF_open_image(fPdf, "raw", "memory", (const char *) mask, w*h, width, height, 1, 1, "mask");
		delete []mask;
	}

	BBitmap * bm = ConvertBitmap(src, bytesPerRow, pixelFormat, flags, data);
	if (!bm) {
		REPORT(kError, fPage, "ConvertBitmap failed!");
		if (*maskId != -1) PDF_close_image(fPdf, *maskId);
		return false;
	}

	char *pdfLibFormat   = "png";
	char *bitmapFileName = "/tmp/pdfwriter.png";	
	const uint32 beosFormat    = B_PNG_FORMAT;

	if (!StoreTranslatorBitmap(bm, bitmapFileName, beosFormat)) {
		delete bm;
		REPORT(kError, fPage, "StoreTranslatorBitmap failed");
		if (*maskId != -1) PDF_close_image(fPdf, *maskId);
		return false;
	}
	delete bm;

	*image = PDF_open_image_file(fPdf, pdfLibFormat, bitmapFileName, 
		*maskId == -1 ? "" : "masked", *maskId == -1 ? 0 : *maskId);

	return *image >= 0;
}


#ifdef CODEWARRIOR
	#pragma mark -- BPicture playback handlers
#endif


// --------------------------------------------------
void PDFWriter::Op(int number)
{
	REPORT(kError, fPage, "Unhandled operand %d", number);
}


// --------------------------------------------------
void	
PDFWriter::MovePenBy(BPoint delta)
{
	REPORT(kDebug, fPage, "MovePenBy delta=[%f, %f]", \
			delta.x, delta.y);
	fState->penX += delta.x;
	fState->penY += delta.y;
}


// --------------------------------------------------
void
PDFWriter::StrokeLine(BPoint start,	BPoint end)
{
	REPORT(kDebug, fPage, "StrokeLine start=[%f, %f], end=[%f, %f]", \
			start.x, start.y, end.x, end.y);

	SetColor();			
	if (!MakesPDF()) return;
	if (IsClipping()) {
		BShape shape;
		shape.MoveTo(start);
		shape.LineTo(end);
		StrokeShape(&shape);
	} else {
		PDF_moveto(fPdf, tx(start.x), ty(start.y));
		PDF_lineto(fPdf, tx(end.x),   ty(end.y));
		StrokeOrClip();
	}
}


// --------------------------------------------------
void 
PDFWriter::StrokeRect(BRect rect)
{
	REPORT(kDebug, fPage, "StrokeRect rect=[%f, %f, %f, %f]", \
			rect.left, rect.top, rect.right, rect.bottom);

	SetColor();			
	if (!MakesPDF()) return;
	if (IsClipping()) {
		BShape shape;
		shape.MoveTo(BPoint(rect.left, rect.top));
		shape.LineTo(BPoint(rect.right, rect.top));
		shape.LineTo(BPoint(rect.right, rect.bottom));
		shape.LineTo(BPoint(rect.left, rect.bottom));
		shape.Close();
		StrokeShape(&shape);
	} else {
		PDF_rect(fPdf, tx(rect.left), ty(rect.bottom), scale(rect.Width()), scale(rect.Height()));
		StrokeOrClip();
	}
}


// --------------------------------------------------
void	
PDFWriter::FillRect(BRect rect)
{
	REPORT(kDebug, fPage, "FillRect rect=[%f, %f, %f, %f]", \
			rect.left, rect.top, rect.right, rect.bottom);

	SetColor();			
	if (!MakesPDF()) return;
	PDF_rect(fPdf, tx(rect.left), ty(rect.bottom), scale(rect.Width()), scale(rect.Height()));
	FillOrClip();
}


// --------------------------------------------------
// The quarter ellipses in the corners of the rectangle are
// approximated with bezier curves.
// The constant 0.555... is taken from gobeProductive.
void	
PDFWriter::PaintRoundRect(BRect rect, BPoint radii, bool stroke) {
	SetColor();
	if (!MakesPDF()) return;

	BPoint center;
	
	float sx = radii.x;
	float sy = radii.y;
	
	float ax = sx;
	float bx = 0.5555555555555 * sx;
	float ay = sy;
	float by = 0.5555555555555 * sy;

	center.x = rect.left + sx;
	center.y = rect.top - sy;

	BShape shape;
	shape.MoveTo(BPoint(center.x - ax, center.y));
	BPoint a[3] = {
		BPoint(center.x - ax, center.y + by),
		BPoint(center.x - bx, center.y + ay),
		BPoint(center.x     , center.y + ay)};
	shape.BezierTo(a);
	
	center.x = rect.right - sx;
	shape.LineTo(BPoint(center.x, center.y + ay));

	BPoint b[3] = {
		BPoint(center.x + bx, center.y + ay),
		BPoint(center.x + ax, center.y + by),
		BPoint(center.x + ax, center.y)};
	shape.BezierTo(b);
		
	center.y = rect.bottom + sy;
	shape.LineTo(BPoint(center.x + sx, center.y)); 

	BPoint c[3] = {
		BPoint(center.x + ax, center.y - by),
		BPoint(center.x + bx, center.y - ay),
		BPoint(center.x     , center.y - ay)};
	shape.BezierTo(c);
	
	center.x = rect.left + sx;	
	shape.LineTo(BPoint(center.x, center.y - ay));

	BPoint d[3] = {
		BPoint(center.x - bx, center.y - ay),
		BPoint(center.x - ax, center.y - by),
		BPoint(center.x - ax, center.y)};
	shape.BezierTo(d); 

	shape.Close();
	
	PaintShape(&shape, stroke);
}


// --------------------------------------------------
void	
PDFWriter::StrokeRoundRect(BRect rect, BPoint radii)
{
	REPORT(kDebug, fPage, "StrokeRoundRect center=[%f, %f, %f, %f], radii=[%f, %f]", \
			rect.left, rect.top, rect.right, rect.bottom, radii.x, radii.y);
	PaintRoundRect(rect, radii, kStroke);
}


// --------------------------------------------------
void	
PDFWriter::FillRoundRect(BRect rect, BPoint	radii)
{
	REPORT(kDebug, fPage, "FillRoundRect rect=[%f, %f, %f, %f], radii=[%f, %f]", \
			rect.left, rect.top, rect.right, rect.bottom, radii.x, radii.y);
	PaintRoundRect(rect, radii, kFill);
}


// --------------------------------------------------
void	
PDFWriter::StrokeBezier(BPoint	*control)
{
	REPORT(kDebug, fPage, "StrokeBezier");
	SetColor();
	if (!MakesPDF()) return;

	BShape shape;
	shape.MoveTo(control[0]);
	shape.BezierTo(&control[1]);
	StrokeShape(&shape);
}


// --------------------------------------------------
void	
PDFWriter::FillBezier(BPoint *control)
{
	REPORT(kDebug, fPage, "FillBezier");
	SetColor();
	if (!MakesPDF()) return;
	PDF_moveto(fPdf, tx(control[0].x), ty(control[0].y));
	PDF_curveto(fPdf, tx(control[1].x), ty(control[1].y),
	            tx(control[2].x), ty(control[2].y),
	            tx(control[3].x), ty(control[3].y));
	PDF_closepath(fPdf);
	FillOrClip();
}


// --------------------------------------------------
// Note the pen size is also scaled!
// We should approximate it with bezier curves too!
void
PDFWriter::PaintArc(BPoint center, BPoint radii, float startTheta, float arcTheta, int stroke) {
	float sx = scale(radii.x);
	float sy = scale(radii.y);
	float smax = sx > sy ? sx : sy;

	SetColor();
	if (!MakesPDF()) return;
	if (IsClipping()); // TODO clip to line path
	
	PDF_save(fPdf);
	PDF_scale(fPdf, sx, sy);
	PDF_setlinewidth(fPdf, fState->penSize / smax);
	PDF_arc(fPdf, tx(center.x) / sx, ty(center.y) / sy, 1, startTheta, startTheta + arcTheta);	
	Paint(stroke);
	PDF_restore(fPdf);
}


// --------------------------------------------------
void
PDFWriter::StrokeArc(BPoint center, BPoint radii, float startTheta, float arcTheta)
{
	REPORT(kDebug, fPage, "StrokeArc center=[%f, %f], radii=[%f, %f], startTheta=%f, arcTheta=%f", \
			center.x, center.y, radii.x, radii.y, startTheta, arcTheta);
	PaintArc(center, radii, startTheta, arcTheta, true);
}


// --------------------------------------------------
void 
PDFWriter::FillArc(BPoint center, BPoint radii, float startTheta, float arcTheta)
{
	REPORT(kDebug, fPage, "FillArc center=[%f, %f], radii=[%f, %f], startTheta=%f, arcTheta=%f", \
			center.x, center.y, radii.x, radii.y, startTheta, arcTheta);
	PaintArc(center, radii, startTheta, arcTheta, false);
}


// --------------------------------------------------
void
PDFWriter::PaintEllipse(BPoint center, BPoint radii, bool stroke)
{
	float sx = radii.x;
	float sy = radii.y;
	
	float ax = sx;
	float bx = 0.5555555555555 * sx;
	float ay = sy;
	float by = 0.5555555555555 * sy;

	SetColor();
	if (!MakesPDF()) return;

	BShape shape;

	shape.MoveTo(BPoint(center.x - ax, center.y));

	BPoint a[3] = { 
		BPoint(center.x - ax, center.y - by),
		BPoint(center.x - bx, center.y - ay),
		BPoint(center.x     , center.y - ay) };
	shape.BezierTo(a);

	BPoint b[3] = {
		BPoint(center.x + bx, center.y - ay),
		BPoint(center.x + ax, center.y - by),
		BPoint(center.x + ax, center.y)};
	shape.BezierTo(b); 

	BPoint c[3] = {
		BPoint(center.x + ax, center.y + by),
		BPoint(center.x + bx, center.y + ay),
		BPoint(center.x     , center.y + ay)};
	shape.BezierTo(c);

	BPoint d[3] = {
		BPoint(center.x - bx, center.y + ay),
		BPoint(center.x - ax, center.y + by),
		BPoint(center.x - ax, center.y)};
	shape.BezierTo(d);
	
	shape.Close();

	PaintShape(&shape, stroke);	
}

// --------------------------------------------------
void
PDFWriter::StrokeEllipse(BPoint center, BPoint radii)
{
	REPORT(kDebug, fPage, "StrokeEllipse center=[%f, %f], radii=[%f, %f]", \
			center.x, center.y, radii.x, radii.y);
	PaintEllipse(center, radii, true);
}


// --------------------------------------------------
void
PDFWriter::FillEllipse(BPoint center, BPoint radii)
{
	REPORT(kDebug, fPage, "FillEllipse center=[%f, %f], radii=[%f, %f]", \
			center.x, center.y, radii.x, radii.y);
	PaintEllipse(center, radii, false);
}


// --------------------------------------------------
void 
PDFWriter::StrokePolygon(int32 numPoints, BPoint *points, bool isClosed)
{
	int32	i;
	float   x0, y0;
	REPORT(kDebug, fPage, "StrokePolygon numPoints=%ld, isClosed=%d\npoints=", \
			numPoints, isClosed);

	if (numPoints <= 1) return;
	
	x0 = y0 = 0.0;
		
	SetColor();		
	if (!MakesPDF()) return;

	if (IsClipping()) {
		BShape shape;
		shape.MoveTo(*points);
		for (i = 1, points ++; i < numPoints; i++, points++) {
			shape.LineTo(*points);
		}
		if (isClosed) 
			shape.Close();
		StrokeShape(&shape);
	} else {
		for ( i = 0; i < numPoints; i++, points++ ) {
			REPORT(kDebug, fPage, " [%f, %f]", points->x, points->y);
			if (i != 0) {
				PDF_lineto(fPdf, tx(points->x), ty(points->y));
			} else {
				x0 = tx(points->x);
				y0 = ty(points->y);
				PDF_moveto(fPdf, x0, y0);
			}
		}
		if (isClosed) 
			PDF_lineto(fPdf, x0, y0);
		StrokeOrClip();
	}
}


// --------------------------------------------------
void 
PDFWriter::FillPolygon(int32 numPoints, BPoint *points, bool isClosed)
{
	int32	i;
	
	REPORT(kDebug, fPage, "FillPolygon numPoints=%ld, isClosed=%dpoints=", \
			numPoints, isClosed);
			
	SetColor();		
	if (!MakesPDF()) return;

	for ( i = 0; i < numPoints; i++, points++ ) {
		REPORT(kDebug, fPage, " [%f, %f]", points->x, points->y);
		if (i != 0) {
			PDF_lineto(fPdf, tx(points->x), ty(points->y));
		} else {
			PDF_moveto(fPdf, tx(points->x), ty(points->y));
		}
	}
	PDF_closepath(fPdf);
	FillOrClip();
}


// --------------------------------------------------
void PDFWriter::PaintShape(BShape *shape, bool stroke)
{
	if (stroke) StrokeShape(shape); else FillShape(shape);
}


// --------------------------------------------------
void PDFWriter::StrokeShape(BShape *shape)
{
	REPORT(kDebug, fPage, "StrokeShape");
	SetColor();			
	if (!MakesPDF()) return;
	DrawShape iterator(this, true);
	iterator.Iterate(shape);
}


// --------------------------------------------------
void PDFWriter::FillShape(BShape *shape)
{
	REPORT(kDebug, fPage, "FillShape");
	SetColor();			
	if (!MakesPDF()) return;
	DrawShape iterator(this, false);
	iterator.Iterate(shape);
}


// --------------------------------------------------
void
PDFWriter::ClipToPicture(BPicture *picture, BPoint point, bool clip_to_inverse_picture)
{
	REPORT(kDebug, fPage, "ClipToPicture at (%f, %f) clip_to_inverse_picture = %s", point.x, point.y, clip_to_inverse_picture ? "true" : "false");
	if (!MakesPDF()) return;
	if (clip_to_inverse_picture) {
		REPORT(kError, fPage, "Clipping to inverse picture not implemented!");
		return;
	}
	if (fMode == kDrawingMode) {
		const bool set_origin = point.x != 0 || point.y != 0;
		if (set_origin) { 
			PushInternalState(); SetOrigin(point); PushInternalState();
		}

		fMode = kClippingMode;
		// create subpath(s) for clipping
		Iterate(picture);
		fMode = kDrawingMode;
		// and clip to it/them
		PDF_clip(fPdf);
								
		if (set_origin) {
			PopInternalState(); PopInternalState();
		}

		REPORT(kDebug, fPage, "Returning from ClipToPicture");
	} else {
		REPORT(kError, fPage, "Nested call of ClipToPicture not implemented yet!");
	}
}

// --------------------------------------------------
void	
PDFWriter::DrawPixels(BRect src, BRect dest, int32 width, int32 height, int32 bytesPerRow, int32 pixelFormat, 
	int32 flags, void *data)
{
	REPORT(kDebug, fPage, "DrawPixels src=[%f, %f, %f, %f], dest=[%f, %f, %f, %f], " \
					"width=%ld, height=%ld, bytesPerRow=%ld, pixelFormat=%ld, " \
					"flags=%ld, data=%p", \
					src.left, src.top, src.right, src.bottom, \
					dest.left, dest.top, dest.right, dest.bottom, \
					width, height, bytesPerRow, pixelFormat, flags, data);

	if (!MakesPDF()) return;
	
	if (IsClipping()) {
		REPORT(kError, fPage, "DrawPixels for clipping not implemented yet!");
		return;
	}

	int maskId, image;
	
	if (!GetImages(src, width, height, bytesPerRow, pixelFormat, flags, data, &maskId, &image)) {
		return;
	}

	const float scaleX = (dest.Width()+1) / (src.Width()+1);
	const float scaleY = (dest.Height()+1) / (src.Height()+1);

	const bool needs_scaling = scaleX != 1.0 || scaleY != 1.0;
	
	if (needs_scaling) {	
		PDF_save(fPdf);
		PDF_scale(fPdf, scaleX, scaleY);
	}

	float x = tx(dest.left)   / scaleX;
	float y = ty(dest.bottom) / scaleY;

	if ( image >= 0 ) {
		PDF_place_image(fPdf, image, x, y, scale(1.0));
		PDF_close_image(fPdf, image);
	} else 
		REPORT(kError, fPage, "PDF_open_image_file failed!");

	if (maskId != -1) PDF_close_image(fPdf, maskId);
	if (needs_scaling) PDF_restore(fPdf);
}



// --------------------------------------------------
void	
PDFWriter::SetClippingRects(BRect *rects, uint32 numRects)
{
	uint32	i;
	
	REPORT(kDebug, fPage, "SetClippingRects numRects=%ld\nrects=", \
			numRects);

	if (!MakesPDF()) return;
	
	for ( i = 0; i < numRects; i++, rects++ ) {
		REPORT(kDebug, fPage, " [%f, %f, %f, %f]", \
				rects->left, rects->top, rects->right, rects->bottom);
		PDF_moveto(fPdf, tx(rects->left),  ty(rects->top)); 
		PDF_lineto(fPdf, tx(rects->right), ty(rects->top));
		PDF_lineto(fPdf, tx(rects->right), ty(rects->bottom));
		PDF_lineto(fPdf, tx(rects->left),  ty(rects->bottom));
		PDF_closepath(fPdf);
	}
	if (numRects > 0) PDF_clip(fPdf);
}


// --------------------------------------------------
void
PDFWriter::PushState()
{
	REPORT(kDebug, fPage, "PushState");
	PushInternalState();
//	LOG((fLog, "height = %f x0 = %f y0 = %f", fState->height, fState->x0, fState->y0));
	if (!MakesPDF()) return;
	PDF_save(fPdf);
}


// --------------------------------------------------
void
PDFWriter::PopState()
{
	REPORT(kDebug, fPage, "PopState");
	if (PopInternalState()) {
		if (!MakesPDF()) return;
		PDF_restore(fPdf);
	}
}


// --------------------------------------------------
void
PDFWriter::EnterStateChange()
{
	REPORT(kDebug, fPage, "EnterStateChange");
	// nothing to do
}


// --------------------------------------------------
void
PDFWriter::ExitStateChange()
{
	REPORT(kDebug, fPage, "ExitStateChange");
	// nothing to do
}


// --------------------------------------------------
void
PDFWriter::EnterFontState()
{
	REPORT(kDebug, fPage, "EnterFontState");
	// nothing to do
}


// --------------------------------------------------
void
PDFWriter::ExitFontState()
{
	REPORT(kDebug, fPage, "ExitFontState");
	// nothing to do
}


// --------------------------------------------------
void
PDFWriter::SetOrigin(BPoint pt)
{
	REPORT(kDebug, fPage, "SetOrigin pt=[%f, %f]", \
			pt.x, pt.y);

	// XXX scale pt with current scaling factor or with
	//     scaling factor of previous state? (fState->prev->scale)			
	BPoint o = fState->prev->pdfSystem.Origin();
	pdfSystem()->SetOrigin(
		o.x + pdfSystem()->scale(pt.x),
		o.y + pdfSystem()->scale(pt.y)
	);
}


// --------------------------------------------------
void	
PDFWriter::SetPenLocation(BPoint pt)
{
	REPORT(kDebug, fPage, "SetPenLocation pt=[%f, %f]", \
			pt.x, pt.y);
		
	fState->penX = pt.x;
	fState->penY = pt.y;
}



// --------------------------------------------------
void
PDFWriter::SetDrawingMode(drawing_mode mode)
{
	REPORT(kDebug, fPage, "SetDrawingMode mode=%d", mode);
	fState->drawingMode = mode;
}



// --------------------------------------------------
void
PDFWriter::SetLineMode(cap_mode capMode, join_mode joinMode, float miterLimit)
{
	REPORT(kDebug, fPage, "SetLineMode");
	fState->capMode    = capMode;
	fState->joinMode   = joinMode;
	fState->miterLimit = miterLimit;
	if (!MakesPDF()) return;
	int m = 0;
	switch (capMode) {
		case B_BUTT_CAP:   m = 0; break;
		case B_ROUND_CAP:  m = 1; break;
		case B_SQUARE_CAP: m = 2; break;
	}
	PDF_setlinecap(fPdf, m);
	
	m = 0;
	switch (joinMode) {
		case B_MITER_JOIN: m = 0; break;
		case B_ROUND_JOIN: m = 1; break;
		case B_BUTT_JOIN: // fall through XXX: check this; no equivalent in PDF?
		case B_SQUARE_JOIN: // fall through XXX: check this too
		case B_BEVEL_JOIN: m = 2; break;
	}
	PDF_setlinejoin(fPdf, m);

	PDF_setmiterlimit(fPdf, miterLimit);
	
}


// --------------------------------------------------
void
PDFWriter::SetPenSize(float size)
{
	REPORT(kDebug, fPage, "SetPenSize size=%f", size);
	if (size <= 0.00001) size = 1;
	// XXX scaling required?
	fState->penSize = scale(size);
	if (!MakesPDF()) return;
	PDF_setlinewidth(fPdf, size);
}


// --------------------------------------------------
void
PDFWriter::SetForeColor(rgb_color color)
{
	float red, green, blue;
	
	red 	= color.red / 255.0;
	green 	= color.green / 255.0;
	blue 	= color.blue / 255.0;

	REPORT(kDebug, fPage, "SetForColor color=[%d, %d, %d, %d] -> [%f, %f, %f]", \
			color.red, color.green, color.blue, color.alpha, \
			red, green, blue);
			
	fState->foregroundColor = color;
}


// --------------------------------------------------
void
PDFWriter::SetBackColor(rgb_color color)
{
	float red, green, blue;
	
	red 	= color.red / 255.0;
	green 	= color.green / 255.0;
	blue 	= color.blue / 255.0;

	REPORT(kDebug, fPage, "SetBackColor color=[%d, %d, %d, %d] -> [%f, %f, %f]", \
			color.red, color.green, color.blue, color.alpha, \
			red, green, blue);
			
	fState->backgroundColor = color;
}


// --------------------------------------------------
void
PDFWriter::SetStipplePattern(pattern pat)
{
	REPORT(kDebug, fPage, "SetStipplePattern");
	fState->pattern0 = pat;
}


// --------------------------------------------------
void
PDFWriter::SetScale(float scale)
{
	REPORT(kDebug, fPage, "SetScale scale=%f", scale);
	pdfSystem()->SetScale(scale * fState->prev->pdfSystem.Scale());
}


// --------------------------------------------------
void
PDFWriter::SetFontFamily(char *family)
{
	REPORT(kDebug, fPage, "SetFontFamily family=\"%s\"", family);
	
	fState->beFont.SetFamilyAndStyle(family, NULL);
}


// --------------------------------------------------
void
PDFWriter::SetFontStyle(char *style)
{
	REPORT(kDebug, fPage, "SetFontStyle style=\"%s\"", style);

	fState->beFont.SetFamilyAndStyle(NULL, style);
}


// --------------------------------------------------
void
PDFWriter::SetFontSpacing(int32 spacing)
{
	REPORT(kDebug, fPage, "SetFontSpacing spacing=%ld", spacing);
	// XXX scaling required?
	// if it is, do it when the font is used...
	fState->beFont.SetSpacing(spacing);
}


// --------------------------------------------------
void
PDFWriter::SetFontSize(float size)
{
	REPORT(kDebug, fPage, "SetFontSize size=%f", size);
	
	fState->beFont.SetSize(size);
}


// --------------------------------------------------
void
PDFWriter::SetFontRotate(float rotation)
{
	REPORT(kDebug, fPage, "SetFontRotate rotation=%f", rotation);
	fState->beFont.SetRotation(RAD2DEGREE(rotation));
}


// --------------------------------------------------
void
PDFWriter::SetFontEncoding(int32 encoding)
{
	REPORT(kDebug, fPage, "SetFontEncoding encoding=%ld", encoding);
	fState->beFont.SetEncoding(encoding);
}


// --------------------------------------------------
void
PDFWriter::SetFontFlags(int32 flags)
{
	REPORT(kDebug, fPage, "SetFontFlags flags=%ld (0x%lx)", flags, flags);
	fState->beFont.SetFlags(flags);
}


// --------------------------------------------------
void
PDFWriter::SetFontShear(float shear)
{
	REPORT(kDebug, fPage, "SetFontShear shear=%f", shear);
	fState->beFont.SetShear(shear);
}


// --------------------------------------------------
void
PDFWriter::SetFontFace(int32 flags)
{
	REPORT(kDebug, fPage, "SetFontFace flags=%ld (0x%lx)", flags, flags);
//	fState->beFont.SetFace(flags);
}

#ifdef CODEWARRIOR
	#pragma mark [Redirectors to instance callbacks/handlers]
#endif


// --------------------------------------------------
size_t	
_WriteData(PDF *pdf, void *data, size_t size)
{ 
	return ((PDFWriter *) PDF_get_opaque(pdf))->WriteData(data, size); 
}


// --------------------------------------------------
void
_ErrorHandler(PDF *pdf, int type, const char *msg)
{
	return ((PDFWriter *) PDF_get_opaque(pdf))->ErrorHandler(type, msg);
}

