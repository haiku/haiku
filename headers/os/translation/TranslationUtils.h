/*****************************************************************************/
//               File: TranslationUtils.h
//              Class: BTranslationUtils
//   Reimplemented by: Michael Wilber, Translation Kit Team
//   Reimplementation: 2002-04
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
		// bitmap functions
		static BBitmap *GetBitmap(const char *name, BTranslatorRoster *roster = NULL);
		static BBitmap *GetBitmap(uint32 type, int32 id,
			BTranslatorRoster *roster = NULL);
		static BBitmap *GetBitmap(uint32 type, const char *name,
			BTranslatorRoster *roster = NULL);
		static BBitmap *GetBitmapFile(const char *name,
			BTranslatorRoster *roster = NULL);
		static BBitmap *GetBitmap(const entry_ref *ref,
			BTranslatorRoster *roster = NULL);	
		static BBitmap *GetBitmap(BPositionIO *stream,
			BTranslatorRoster *roster = NULL);

		// text functions
		static status_t GetStyledText(BPositionIO *fromStream, BTextView *intoView,
			BTranslatorRoster *roster = NULL);
		static status_t PutStyledText(BTextView *fromView, BPositionIO *intoStream,
			BTranslatorRoster *roster = NULL);
		static status_t WriteStyledEditFile(BTextView *fromView, BFile *intoFile);
		static status_t WriteStyledEditFile(BTextView *fromView, BFile *intoFile,
			const char* encoding);

		// misc. functions
		static BMessage *GetDefaultSettings(translator_id forTranslator,
			BTranslatorRoster *roster = NULL);
		static BMessage *GetDefaultSettings(const char *translatorName,
			int32 translatorVersion);

		enum {
			B_TRANSLATION_MENU = 'BTMN'
		};
		static status_t AddTranslationItems(BMenu *intoMenu, uint32 fromType,
			const BMessage *model = NULL,	// default B_TRANSLATION_MENU
			const char *translatorIdName = NULL, // default "be:translator"
			const char *translatorTypeName = NULL, // default "be:type"
			BTranslatorRoster *roster = NULL);
};

#endif /* _TRANSLATION_UTILS_H */

