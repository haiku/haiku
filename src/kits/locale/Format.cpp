#include <Format.h>

#include <new>

#include <Autolock.h>
#include <Locale.h>
#include <LocaleRoster.h>


BFormat::BFormat(const BLocale* locale)
{
	if (locale == NULL)
		locale = BLocaleRoster::Default()->GetDefaultLocale();

	if (locale == NULL) {
		fInitStatus = B_BAD_DATA;
		return;
	}

	_Initialize(*locale);
}


BFormat::BFormat(const BLanguage& language,
	const BFormattingConventions& conventions)
{
	_Initialize(language, conventions);
}


BFormat::BFormat(const BFormat &other)
	:
	fConventions(other.fConventions),
	fLanguage(other.fLanguage),
	fInitStatus(other.fInitStatus)
{
}


BFormat::~BFormat()
{
}


status_t
BFormat::InitCheck() const
{
	return fInitStatus;
}


status_t
BFormat::_Initialize(const BLocale& locale)
{
	BFormattingConventions conventions;
	BLanguage language;

	fInitStatus = locale.GetFormattingConventions(&conventions);
	if (fInitStatus != B_OK)
		return fInitStatus;

	fInitStatus = locale.GetLanguage(&language);
	if (fInitStatus != B_OK)
		return fInitStatus;

	return _Initialize(language, conventions);
}


status_t
BFormat::_Initialize(const BLanguage& language,
	const BFormattingConventions& conventions)
{
	fConventions = conventions;
	fLanguage = language;
	fInitStatus = B_OK;
	return fInitStatus;
}
