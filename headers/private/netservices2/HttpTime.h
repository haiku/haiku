/*
 * Copyright 2010-2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_HTTP_TIME_H_
#define _B_HTTP_TIME_H_

#include <ctime>

#include <DateTime.h>
#include <ErrorsExt.h>
#include <String.h>

namespace BPrivate {

namespace Network {

enum class BHttpTimeFormat : int8 { RFC1123 = 0, RFC850, AscTime };


class BHttpTime
{
public:
	// Error type
	class InvalidInput;

	// Constructors
								BHttpTime() noexcept;
								BHttpTime(BDateTime date);
								BHttpTime(const BString& dateString);

	// Date modification
			void				SetTo(const BString& string);
			void				SetTo(BDateTime date);


	// Date Access
			BDateTime			DateTime() const noexcept;
			BHttpTimeFormat		DateTimeFormat() const noexcept;
			BString				ToString(BHttpTimeFormat outputFormat
									= BHttpTimeFormat::RFC1123) const;

private:
			void				_Parse(const BString& dateString);

			BDateTime			fDate;
			BHttpTimeFormat		fDateFormat;
};


class BHttpTime::InvalidInput : public BError
{
public:
								InvalidInput(const char* origin, BString input);

	virtual	const char*			Message() const noexcept override;
	virtual	BString				DebugMessage() const override;

			BString				input;
};


// Convenience functions
BDateTime parse_http_time(const BString& string);
BString format_http_time(BDateTime timestamp,
	BHttpTimeFormat outputFormat = BHttpTimeFormat::RFC1123);


} // namespace Network

} // namespace BPrivate

#endif // _B_HTTP_TIME_H_
