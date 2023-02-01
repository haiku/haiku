/*
 * Copyright 2021 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ERRORS_EXT_H
#define _ERRORS_EXT_H

#include <iosfwd>

#include <String.h>
#include <SupportDefs.h>

class BDataIO;


namespace BPrivate {

namespace Network {


class BError
{
public:
								BError(const char* origin);
								BError(BString origin);
	virtual						~BError() noexcept;

								BError(const BError& error);
								BError(BError&& error) noexcept;

			BError&				operator=(const BError& error);
			BError&				operator=(BError&& error) noexcept;

	virtual	const char*			Message() const noexcept = 0;
	virtual	const char*			Origin() const noexcept;
	virtual	BString				DebugMessage() const;
			void				WriteToStream(std::ostream& stream) const;
			size_t				WriteToOutput(BDataIO* output) const;

private:
	virtual	void				_ReservedError1();
	virtual	void				_ReservedError2();
	virtual	void				_ReservedError3();
	virtual	void				_ReservedError4();

private:
			BString				fOrigin;
};


class BRuntimeError : public BError
{
public:
								BRuntimeError(const char* origin, const char* message);
								BRuntimeError(const char* origin, BString message);
								BRuntimeError(BString origin, BString message);

								BRuntimeError(const BRuntimeError& other);
								BRuntimeError(BRuntimeError&& other) noexcept;

			BRuntimeError&		operator=(const BRuntimeError& other);
			BRuntimeError&		operator=(BRuntimeError&& other) noexcept;

	virtual	const char*			Message() const noexcept override;

private:
			BString				fMessage;
};


class BSystemError : public BError
{
public:
								BSystemError(const char* origin, status_t error);
								BSystemError(BString origin, status_t error);

								BSystemError(const BSystemError& other);
								BSystemError& operator=(const BSystemError& other);

								BSystemError(BSystemError&& other) noexcept;
								BSystemError& operator=(BSystemError&& other) noexcept;

	virtual	const char*			Message() const noexcept override;
	virtual	BString				DebugMessage() const override;
			status_t			Error() noexcept;

private:
			status_t			fErrorCode;
};

} // namespace Network

} // Namespace BPrivate

#endif // _ERRORS_EXT_H
