#include <Format.h>

#include <new>

#include <Autolock.h>
#include <Locale.h>
#include <LocaleRoster.h>


BFormat::BFormat()
{
	const BLocale* locale = BLocaleRoster::Default()->GetDefaultLocale();
	SetLocale(*locale);
}


BFormat::BFormat(const BFormat &other)
{
	fInitStatus = other.fInitStatus;
	fConventions = other.fConventions;
	fLanguage = other.fLanguage;
}


BFormat::~BFormat()
{
}


BFormat &
BFormat::operator=(const BFormat& other)
{
	if (this == &other)
		return *this;

	fInitStatus = other.fInitStatus;
	fLanguage = other.fLanguage;
	fInitStatus = other.fInitStatus;

	return *this;
}


status_t
BFormat::InitCheck() const
{
	return fInitStatus;
}


status_t
BFormat::SetLocale(const BLocale& locale)
{
	BFormattingConventions conventions;
	BLanguage language;

	fInitStatus = locale.GetFormattingConventions(&conventions);
	if (fInitStatus != B_OK)
		return fInitStatus;
	fInitStatus = SetFormattingConventions(conventions);
	if (fInitStatus != B_OK)
		return fInitStatus;

	fInitStatus = locale.GetLanguage(&language);
	if (fInitStatus != B_OK)
		return fInitStatus;
	fInitStatus = SetLanguage(language);
	return fInitStatus;
}


status_t
BFormat::SetFormattingConventions(const BFormattingConventions& conventions)
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_WOULD_BLOCK;

	fConventions = conventions;
	return B_OK;
}


status_t
BFormat::SetLanguage(const BLanguage& newLanguage)
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_WOULD_BLOCK;

	fLanguage = newLanguage;
	return B_OK;
}


