/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "SourceLanguageInfo.h"

#include "Dwarf.h"


UnknownSourceLanguageInfo::UnknownSourceLanguageInfo()
{
	name = "unknown";
	arrayOrdering = DW_ORD_row_major;
	subrangeLowerBound = 0;
}


CFamilySourceLanguageInfo::CFamilySourceLanguageInfo()
{
	arrayOrdering = DW_ORD_row_major;
	subrangeLowerBound = 0;
}


CSourceLanguageInfo::CSourceLanguageInfo()
{
	name = "C";
}


C89SourceLanguageInfo::C89SourceLanguageInfo()
{
	name = "C 89";
}


C99SourceLanguageInfo::C99SourceLanguageInfo()
{
	name = "C 99";
}


CPlusPlusSourceLanguageInfo::CPlusPlusSourceLanguageInfo()
{
	name = "C++";
}


const UnknownSourceLanguageInfo		kUnknownLanguageInfo;
const UnknownSourceLanguageInfo		kUnsupportedLanguageInfo;
const CSourceLanguageInfo			kCLanguageInfo;
const C89SourceLanguageInfo			kC89LanguageInfo;
const C99SourceLanguageInfo			kC99LanguageInfo;
const CPlusPlusSourceLanguageInfo	kCPlusPlusLanguageInfo;
