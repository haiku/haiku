/*
 * Copyright 2014, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_MESSAGE_FORMAT_H_
#define _B_MESSAGE_FORMAT_H_


#include <Format.h>


namespace icu {
	class MessageFormat;
	class UnicodeString;
}


class BMessageFormat: public BFormat {
public:
								BMessageFormat(const BString pattern);
								~BMessageFormat();

			status_t			InitCheck();

			status_t			SetLanguage(const BLanguage& newLanguage);
			status_t			SetFormattingConventions(
									const BFormattingConventions&
										conventions);

			status_t			Format(BString& buffer, const int32 arg) const;

private:
			status_t			_Initialize(const icu::UnicodeString&);

private:
			status_t			fInitStatus;
			icu::MessageFormat*	fFormatter;
};


#endif
