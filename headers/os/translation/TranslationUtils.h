/*
 * Copyright 2002-2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TRANSLATION_UTILS_H
#define _TRANSLATION_UTILS_H


#include <GraphicsDefs.h>
#include <SupportDefs.h>
#include <TranslationDefs.h>


class BBitmap;
class BFile;
class BMenu;
class BMessage;
class BPositionIO;
class BTextView;
class BTranslatorRoster;
struct entry_ref;


class BTranslationUtils {
								BTranslationUtils();
								BTranslationUtils(
									const BTranslationUtils& other);
								~BTranslationUtils();

			BTranslationUtils&	operator=(const BTranslationUtils& other);

public:
	enum {
		B_TRANSLATION_MENU = 'BTMN'
	};

	static	BBitmap*			GetBitmap(const char* name,
									BTranslatorRoster* roster = NULL);
	static	BBitmap*			GetBitmap(uint32 type, int32 id,
									BTranslatorRoster* roster = NULL);
	static	BBitmap*			GetBitmap(uint32 type, const char* name,
									BTranslatorRoster* roster = NULL);
	static	BBitmap*			GetBitmapFile(const char* name,
									BTranslatorRoster* roster = NULL);
	static	BBitmap*			GetBitmap(const entry_ref* ref,
									BTranslatorRoster* roster = NULL);
	static	BBitmap*			GetBitmap(BPositionIO* stream,
									BTranslatorRoster* roster = NULL);

	static	void				SetBitmapColorSpace(color_space space);
	static	color_space			BitmapColorSpace();

	static	status_t			GetStyledText(BPositionIO* fromStream,
									BTextView* intoView,
									BTranslatorRoster* roster = NULL);
	static	status_t			GetStyledText(BPositionIO* fromStream,
									BTextView* intoView, const char* encoding,
									BTranslatorRoster* roster = NULL);
	static	status_t			PutStyledText(BTextView* fromView,
									BPositionIO* intoStream,
									BTranslatorRoster* roster = NULL);
	static	status_t			WriteStyledEditFile(BTextView* fromView,
									BFile* intoFile);
	static	status_t			WriteStyledEditFile(BTextView* fromView,
									BFile* intoFile, const char* encoding);

	static	BMessage*			GetDefaultSettings(translator_id translator,
									BTranslatorRoster* roster = NULL);
	static	BMessage*			GetDefaultSettings(const char* name,
									int32 version);

	static	status_t			AddTranslationItems(BMenu* intoMenu,
									uint32 fromType,
									const BMessage* model = NULL,
									const char* idName = NULL,
									const char* typeName = NULL,
									BTranslatorRoster* roster = NULL);

private:
	static	translator_info*	_BuildTranslatorInfo(const translator_id id,
									const translation_format* format);
	static	int					_CompareTranslatorInfoByName(const translator_info* info1,
									const translator_info* info2);

	static	color_space			sBitmapSpace;
};


#endif	// _TRANSLATION_UTILS_H
