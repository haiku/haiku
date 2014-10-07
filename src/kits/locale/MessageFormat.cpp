/*
 * Copyright 2014, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#include <MessageFormat.h>

#include <FormattingConventionsPrivate.h>
#include <LanguagePrivate.h>

#include <ICUWrapper.h>

#include <unicode/msgfmt.h>


status_t
BMessageFormat::Format(BString& output, const BString message, const int32 arg)
{
	UnicodeString buffer;
	UErrorCode error = U_ZERO_ERROR;

	Formattable arguments[] = {
		(int32_t)arg
	};

	Locale* icuLocale
		= fConventions.UseStringsFromPreferredLanguage()
			? BLanguage::Private(&fLanguage).ICULocale()
			: BFormattingConventions::Private(&fConventions).ICULocale();

	MessageFormat formatter(UnicodeString::fromUTF8(message.String()),
		*icuLocale, error);
	FieldPosition pos;
	buffer = formatter.format(arguments, 1, buffer, pos, error);
	if (!U_SUCCESS(error))
		return B_ERROR;

	BStringByteSink byteSink(&output);
	buffer.toUTF8(byteSink);

	return B_OK;
}
