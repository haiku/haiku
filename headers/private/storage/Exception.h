// Exception

#ifndef _sk_exception_
#define _sk_exception_

#include <stdarg.h>
#include <stdio.h>

#include <String.h>

namespace StorageKit {

class Exception {
public:
	// constructor
	Exception()
		: fError(B_OK),
		  fDescription()
	{
	}

	// constructor
	Exception(BString description)
		: fError(B_OK),
		  fDescription(description)
	{
	}

	// constructor
	Exception(const char* format,...)
		: fError(B_OK),
		  fDescription()
	{
		va_list args;
		va_start(args, format);
		SetTo(B_OK, format, args);
		va_end(args);
	}

	// constructor
	Exception(status_t error)
		: fError(error),
		  fDescription()
	{
	}

	// constructor
	Exception(status_t error, BString description)
		: fError(error),
		  fDescription(description)
	{
	}

	// constructor
	Exception(status_t error, const char* format,...)
		: fError(error),
		  fDescription()
	{
		va_list args;
		va_start(args, format);
		SetTo(error, format, args);
		va_end(args);
	}

	// copy constructor
	Exception(const Exception& exception)
		: fError(exception.fError),
		  fDescription(exception.fDescription)
	{
	}

	// destructor
	~Exception()
	{
	}

	// SetTo
	void SetTo(status_t error, BString description)
	{
		fError = error;
		fDescription.SetTo(description);
	}

	// SetTo
	void SetTo(status_t error, const char* format, va_list arg)
	{
		char buffer[2048];
		vsprintf(buffer, format, arg);
		SetTo(error, BString(buffer));
	}

	// GetError
	status_t Error() const
	{
		return fError;
	}

	// GetDescription
	const char* Description() const
	{
		return fDescription.String();
	}

private:
	status_t	fError;
	BString		fDescription;
};

};	// namespace StorageKit

#endif	// _sk_exception_
