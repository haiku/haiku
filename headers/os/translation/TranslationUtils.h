/*****************************************************************************/
//               File: TranslationUtils.h
//              Class: BTranslationUtils
//   Reimplimented by: Michael Wilber, Translation Kit Team
//   Reimplimentation: 2002-04
//
// Description: Utility functions for the Translation Kit
//
//
// Copyright (c) 2002 OpenBeOS Project
//
// Original Version: Copyright 1998, Be Incorporated, All Rights Reserved.
//                   Copyright 1995-1997, Jon Watte
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#if !defined(_TRANSLATION_UTILS_H)
#define _TRANSLATION_UTILS_H

#include <TranslationDefs.h>
#include <SupportDefs.h>

class BBitmap;
class BTranslatorRoster;
class BPositionIO;
class BMenu;

// This class is a little different from many other classes.
// You don't create an instance of it; you just call its various
// static member functions for utility-like operations.
class BTranslationUtils {
	BTranslationUtils();
	~BTranslationUtils();
	BTranslationUtils(const BTranslationUtils &);
	BTranslationUtils & operator=(const BTranslationUtils &);

public:

// BITMAP getters - allocate the BBitmap; you call delete on it!

static BBitmap *GetBitmap(const char *kName, BTranslatorRoster *roster = NULL);
	// Get bitmap - first try as file name, then as B_TRANSLATOR_BITMAP
	// resource type from app file -- can be of any kind for which a
	// translator is installed (TGA, etc)
	
static BBitmap *GetBitmap(uint32 type, int32 id,
	BTranslatorRoster *roster = NULL);
	// Get bitmap - from app resource by id
	
static BBitmap *GetBitmap(uint32 type, const char *kName,
	BTranslatorRoster *roster = NULL);
	// Get Bitmap - from app resource by name
	
static BBitmap *GetBitmapFile(const char *kName,
	BTranslatorRoster *roster = NULL);
	// Get bitmap - from file only

static BBitmap *GetBitmap(const entry_ref *kRef,
	BTranslatorRoster *roster = NULL);
	// Get bitmap - from open file (or BMemoryIO)
		
static BBitmap *GetBitmap(BPositionIO *stream,
	BTranslatorRoster *roster = NULL);
	// Get bitmap - from open file or IO type object

static status_t GetStyledText(BPositionIO *fromStream, BTextView *intoView,
	BTranslatorRoster *roster = NULL);
	// For styled text, we can translate from a stream
	// directly into a BTextView

static status_t PutStyledText(BTextView *fromView, BPositionIO *intoStream,
	BTranslatorRoster *roster = NULL);
	// Saving is slightly different -- given the BTextView, a 
	// B_STYLED_TEXT_FORMAT stream is written to intoStream. You can then call
	// Translate() yourself to translate that stream into something else if
	// you want, if it is a temp file or a BMallocIO. It's also OK to write
	// the B_STYLED_TEXT_FORMAT to a file, although the StyledEdit format 
	// (TEXT file + style attributes) is preferred. This convenience function
	// is only marginally part of the Translation Kit, but what the hey :-)
	
static status_t WriteStyledEditFile(BTextView *fromView, BFile *intoFile);
	// Writes the styled text data from fromView to intoFile

static BMessage *GetDefaultSettings(translator_id forTranslator,
	BTranslatorRoster *roster = NULL);
	// Get default settings for the translator with the id forTranslator
	
static BMessage *GetDefaultSettings(const char *kTranslatorName,
	int32 translatorVersion);
	// Get default settings for the translator with the name kTranslatorName

// Envious of that "Save As" menu in ShowImage? Well, you can have your own!
// AddTranslationItems will add menu items for all translations from the
// basic format you specify (B_TRANSLATOR_BITMAP, B_TRANSLATOR_TEXT etc).
// The translator ID and format constant chosen will be added to the message
// that is sent to you when the menu item is selected.
	enum {
		B_TRANSLATION_MENU = 'BTMN'
	};
static status_t AddTranslationItems(BMenu *intoMenu, uint32 fromType,
	const BMessage *kModel = NULL,	// default B_TRANSLATION_MENU
	const char *kTranslatorIdName = NULL, // default "be:translator"
	const char *kTranslatorTypeName = NULL, // default "be:type"
	BTranslatorRoster *roster = NULL);
				
// BEGIN: Added by Michael Wilber
private:

static BBitmap * TranslateToBitmap(BPositionIO *pio,
	BTranslatorRoster *roster = NULL);
	// Translates the image data from pio to the type type using the
	// supplied BTranslatorRoster. If BTranslatorRoster is not supplied
	// the default BTranslatorRoster is used.
	
// END: Added by Michael Wilber
				
};

#endif /* _TRANSLATION_UTILS_H */

