/*
 * Copyright 2012 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "StringForRate.h"

#include <stdio.h>

#include <SystemCatalog.h>

using BPrivate::gSystemCatalog;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "StringForRate"


namespace BPrivate {


const char*
string_for_rate(double rate, char* string, size_t stringSize, double base)
{
	double kbps = rate / base;
	if (kbps < 1.0) {
		const char* trKey = B_TRANSLATE_MARK("%.0f bps");
		snprintf(string, stringSize, gSystemCatalog.GetString(trKey,
			B_TRANSLATION_CONTEXT), (int)rate);
		return string;
	}
	double mbps = kbps / base;
	if (mbps < 1.0) {
		const char* trKey = B_TRANSLATE_MARK("%.0f kbps");
		snprintf(string, stringSize, gSystemCatalog.GetString(trKey,
			B_TRANSLATION_CONTEXT), kbps);
		return string;
	}
	double gbps = mbps / base;
	if (gbps < 1.0) {
		const char* trKey = B_TRANSLATE_MARK("%.0f mbps");
		snprintf(string, stringSize, gSystemCatalog.GetString(trKey,
			B_TRANSLATION_CONTEXT), mbps);
		return string;
	}
	double tbps = gbps / base;
	if (tbps < 1.0) {
		const char* trKey = B_TRANSLATE_MARK("%.0f gbps");
		snprintf(string, stringSize, gSystemCatalog.GetString(trKey,
			B_TRANSLATION_CONTEXT), gbps);
		return string;
	}
	const char* trKey = B_TRANSLATE_MARK("%.0f tbps");
	snprintf(string, stringSize, gSystemCatalog.GetString(trKey,
		B_TRANSLATION_CONTEXT), tbps);
	return string;
}


}	// namespace BPrivate

