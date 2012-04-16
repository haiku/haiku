/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TermConst.h"

#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Terminal ToolTips"


const char* const kTooTipSetTabTitlePlaceholders = B_TRANSLATE(
	"\t%d\t-\tThe current working directory of the active process.\n"
	"\t\t\tOptionally the maximum number of path components can be\n"
	"\t\t\tspecified. E.g. '%2d' for at most two components.\n"
	"\t%i\t-\tThe index of the tab.\n"
	"\t%p\t-\tThe name of the active process.\n"
	"\t%%\t-\tThe character '%'.");

const char* const kTooTipSetWindowTitlePlaceholders = B_TRANSLATE(
	"\t%d\t-\tThe current working directory of the active process in the\n"
	"\t\t\tcurrent tab. Optionally the maximum number of path components\n"
	"\t\t\tcan be specified. E.g. '%2d' for at most two components.\n"
	"\t%i\t-\tThe index of the window.\n"
	"\t%p\t-\tThe name of the active process in the current tab.\n"
	"\t%t\t-\tThe title of the current tab.\n"
	"\t%%\t-\tThe character '%'.");
