/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TermConst.h"

#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Terminal ToolTips"


const char* const kToolTipSetTabTitlePlaceholders = B_TRANSLATE(
	"\t%d\t-\tThe current working directory of the active process.\n"
	"\t\t\tOptionally the maximum number of path components can be\n"
	"\t\t\tspecified. E.g. '%2d' for at most two components.\n"
	"\t%i\t-\tThe index of the tab.\n"
	"\t%e\t-\tThe encoding of the current tab. Not shown for UTF-8.\n"
	"\t%p\t-\tThe name of the active process.");

const char* const kToolTipSetWindowTitlePlaceholders = B_TRANSLATE(
	"\t%d\t-\tThe current working directory of the active process in the\n"
	"\t\t\tcurrent tab. Optionally the maximum number of path components\n"
	"\t\t\tcan be specified. E.g. '%2d' for at most two components.\n"
	"\t%T\t-\tThe Terminal application name for the current locale.\n"
	"\t%e\t-\tThe encoding of the current tab. Not shown for UTF-8.\n"
	"\t%i\t-\tThe index of the window.\n"
	"\t%p\t-\tThe name of the active process in the current tab.\n"
	"\t%t\t-\tThe title of the current tab.");

const char* const kToolTipCommonTitlePlaceholders = B_TRANSLATE(
	"\t%%\t-\tThe character '%'.\n"
	"\t%<\t-\tStarts a section that will only be shown if a placeholder\n"
	"\t\t\tafterwards is not empty.\n"
	"\t%>\t-\tStarts a section that will only be shown if a placeholder\n"
	"\t\t\tbetween a previous %< section and this one is not empty.\n"
	"\t%-\t-\tEnds a %< or %> section.\n\n"
	"Any non alpha numeric character between '%' and the format "
	"letter will insert a space only\nif the placeholder value is not "
	"empty. It will add to the %< section.");

const char* const kShellEscapeCharacters = " ~`#$&*()\\|[]{};'\"<>?!";
const char* const kDefaultAdditionalWordCharacters = ":@-./_~";
const char* const kURLAdditionalWordCharacters = ":/-._~[]?#@!$%&'()*+,;=";
