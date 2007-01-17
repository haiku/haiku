/*
 * Copyright 2002-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TRANSLATION_UTILS_H
#define _TRANSLATION_UTILS_H


#include <TranslationDefs.h>
#include <SupportDefs.h>

class BBitmap;
class BTranslatorRoster;
class BPositionIO;
class BMenu;


class BTranslationUtils {
	private:
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
		static status_t GetStyledText(BPositionIO* fromStream, BTextView* intoView,
			BTranslatorRoster *roster = NULL);
		static status_t GetStyledText(BPositionIO* fromStream, BTextView* intoView,
			const char* encoding, BTranslatorRoster* roster = NULL);
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
			const BMessage *model = NULL,			// default B_TRANSLATION_MENU
			const char *translatorIdName = NULL,	// default "be:translator"
			const char *translatorTypeName = NULL,	// default "be:type"
			BTranslatorRoster *roster = NULL);
};

#endif /* _TRANSLATION_UTILS_H */

