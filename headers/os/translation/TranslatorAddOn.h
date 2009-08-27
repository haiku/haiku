/*
 * Copyright 2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TRANSLATOR_ADD_ON_H
#define _TRANSLATOR_ADD_ON_H


#include <TranslationDefs.h>


class BMessage;
class BView;
class BRect;
class BPositionIO;


// Deprecated, use BTranslator API instead

extern char translatorName[];
extern char translatorInfo[];
extern int32 translatorVersion;

extern translation_format inputFormats[];	// optional
extern	translation_format outputFormats[];	// optional


extern "C" {

extern status_t	Identify(BPositionIO* source, const translation_format* format,
					BMessage* extension, translator_info* info, uint32 outType);
extern status_t	Translate(BPositionIO* source, const translator_info* info,
					BMessage* extension, uint32 outType,
					BPositionIO* destination);
extern status_t	MakeConfig(BMessage* extension, BView** _view, BRect* _frame);
extern status_t	GetConfigMessage(BMessage* extension);

}


#endif	//_TRANSLATOR_ADD_ON_H
