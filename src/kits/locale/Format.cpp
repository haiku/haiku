#include <Format.h>

#include <new>

#include <Locale.h>


BFormat::BFormat()
	:
	fInitStatus(B_NO_INIT),
	fLocale(NULL)
{
}


BFormat::BFormat(const BFormat &other)
	:
	fInitStatus(other.fInitStatus),
	fLocale(other.fLocale != NULL
		? new (std::nothrow) BLocale(*other.fLocale)
		: NULL)
{
}


BFormat::~BFormat()
{
	delete fLocale;
}


BFormat &
BFormat::operator=(const BFormat& other)
{
	if (this == &other)
		return *this;

	fInitStatus = other.fInitStatus;
	delete fLocale;
	fLocale = other.fLocale != NULL
		? new (std::nothrow) BLocale(*other.fLocale) : NULL;

	if (fLocale == NULL && other.fLocale != NULL)
		fInitStatus = B_NO_MEMORY;

	return *this;
}


status_t
BFormat::InitCheck() const
{
	return fInitStatus;
}


status_t
BFormat::SetLocale(const BLocale* locale)
{
	if (locale != NULL) {
		if (fLocale == NULL) {
			fLocale = new (std::nothrow) BLocale(*locale);
			if (fLocale == NULL)
				return B_NO_MEMORY;
		} else
			*fLocale = *locale;
	} else {
		delete fLocale;
		fLocale = NULL;
	}

	return B_OK;
}
