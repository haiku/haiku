/*
 * Copyright 2014-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 *		John Scipione, jscipione@gmail.com
 */
#ifndef _B_STRING_FORMAT_H_
#define _B_STRING_FORMAT_H_


#include <Format.h>


#ifndef U_ICU_NAMESPACE
  #define U_ICU_NAMESPACE icu
#endif
namespace U_ICU_NAMESPACE {
	class MessageFormat;
	class UnicodeString;
}


class BStringFormat : public BFormat {
public:
								BStringFormat(const BLanguage& language,
									const BString pattern);
								BStringFormat(const BString pattern);
								~BStringFormat();

			status_t			InitCheck();

			status_t			Format(BString& buffer, const int64 arg) const;

private:
			status_t			_Initialize(const U_ICU_NAMESPACE::UnicodeString&);

private:
			U_ICU_NAMESPACE::MessageFormat*	fFormatter;
};


#endif	// _B_STRING_FORMAT_H_
