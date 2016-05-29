/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SOURCE_LANGUAGE_INFO_H
#define SOURCE_LANGUAGE_INFO_H

#include <SupportDefs.h>


struct SourceLanguageInfo {
	const char*	name;
	uint8		arrayOrdering;
	uint64		subrangeLowerBound;
};


struct UnknownSourceLanguageInfo : SourceLanguageInfo {
								UnknownSourceLanguageInfo();
};


struct CFamilySourceLanguageInfo : SourceLanguageInfo {
								CFamilySourceLanguageInfo();
};


struct CSourceLanguageInfo : CFamilySourceLanguageInfo {
								CSourceLanguageInfo();
};


struct C89SourceLanguageInfo : CFamilySourceLanguageInfo {
								C89SourceLanguageInfo();
};


struct C99SourceLanguageInfo : CFamilySourceLanguageInfo {
								C99SourceLanguageInfo();
};


struct CPlusPlusSourceLanguageInfo : CFamilySourceLanguageInfo {
								CPlusPlusSourceLanguageInfo();
};


extern const UnknownSourceLanguageInfo		kUnknownLanguageInfo;
extern const UnknownSourceLanguageInfo		kUnsupportedLanguageInfo;
extern const CSourceLanguageInfo			kCLanguageInfo;
extern const C89SourceLanguageInfo			kC89LanguageInfo;
extern const C99SourceLanguageInfo			kC99LanguageInfo;
extern const CPlusPlusSourceLanguageInfo	kCPlusPlusLanguageInfo;


#endif	// SOURCE_LANGUAGE_INFO_H
