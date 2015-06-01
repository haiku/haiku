/*
 * Copyright 2014-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 *		John Scipione, jscipione@gmail.com
 */
#ifndef _B_MESSAGE_FORMAT_H_
#define _B_MESSAGE_FORMAT_H_


#include <Format.h>


#ifndef U_ICU_NAMESPACE
  #define U_ICU_NAMESPACE icu
#endif
namespace U_ICU_NAMESPACE {
	class MessageFormat;
	class UnicodeString;
}


class BMessageFormat: public BFormat {
public:
								BMessageFormat(const BLanguage& language,
									const BString pattern);
								BMessageFormat(const BString pattern);
								~BMessageFormat();

			status_t			InitCheck();

			status_t			Format(BString& buffer, const int64 arg) const;

private:
			status_t			_Initialize(const U_ICU_NAMESPACE::UnicodeString&);

private:
			U_ICU_NAMESPACE::MessageFormat*	fFormatter;
};


#endif	// _B_MESSAGE_FORMAT_H_
