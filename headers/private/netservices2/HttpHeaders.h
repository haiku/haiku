/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _B_HTTP_HEADERS_H_
#define _B_HTTP_HEADERS_H_

#include <string_view>

#include <ErrorsExt.h>
#include <String.h>


namespace BPrivate {

namespace Network {


class BHttpHeader {
public:
	// Exceptions
	class InvalidInput : public BError {
	public:
								InvalidInput(const char* origin, BString input);

		virtual	const char*		Message() const noexcept override;
		virtual	BString			DebugMessage() const override;

		BString					input;
	};

	class EmptyHeader : public BError {
	public:
								EmptyHeader(const char* origin);

		virtual	const char*		Message() const noexcept override;
	};

	// Wrapper Types
	class HeaderName {
	public:
				bool			IsEmpty() const noexcept { return fName.IsEmpty(); }

		// Comparison
				bool			operator==(const BString& other) const noexcept;
				bool			operator==(const std::string_view other) const noexcept;

		// Conversion
								operator BString() const;
								operator std::string_view() const;
	private:
		friend	class			BHttpHeader;
								HeaderName(std::string_view name);
		BString					fName;
	};

	// Constructors & Destructor
						BHttpHeader();
						BHttpHeader(std::string_view name, std::string_view value);
						BHttpHeader(const BHttpHeader& other);
						BHttpHeader(BHttpHeader&& other) noexcept;
						~BHttpHeader() noexcept;

	// Assignment operators
	BHttpHeader&		operator=(const BHttpHeader& other);
	BHttpHeader&		operator=(BHttpHeader&& other) noexcept;

	// Header data modification
	void				SetName(std::string_view name);
	void				SetValue(std::string_view value);

	// Access Data
	const HeaderName&	Name() noexcept;
	std::string_view	Value() noexcept;
	bool				IsEmpty() noexcept;

private:
	HeaderName			fName;
	BString				fValue;
};


// Placeholder for a HashMap that represents HTTP Headers
class BHttpHeaderMap {

};


} // namespace Network

} // namespace BPrivate

#endif // _B_HTTP_HEADERS_H_
