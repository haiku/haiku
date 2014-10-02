/*
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2010-2011, Oliver Tappe, zooey@hirschkaefer.de.
 * Copyright 2012, John Scipione, jscipione@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <Autolock.h>
#include <Catalog.h>
#include <Locale.h>
#include <LocaleRoster.h>


BLocale::BLocale(const BLanguage* language,
	const BFormattingConventions* conventions)
{
	if (conventions != NULL)
		fConventions = *conventions;
	else
		BLocale::Default()->GetFormattingConventions(&fConventions);

	if (language != NULL)
		fLanguage = *language;
	else
		BLocale::Default()->GetLanguage(&fLanguage);
}


BLocale::BLocale(const BLocale& other)
	:
	fConventions(other.fConventions),
	fLanguage(other.fLanguage)
{
}


/*static*/ const BLocale*
BLocale::Default()
{
	return BLocaleRoster::Default()->GetDefaultLocale();
}


BLocale&
BLocale::operator=(const BLocale& other)
{
	if (this == &other)
		return *this;

	BAutolock lock(fLock);
	BAutolock otherLock(other.fLock);
	if (!lock.IsLocked() || !otherLock.IsLocked())
		return *this;

	fConventions = other.fConventions;
	fLanguage = other.fLanguage;

	return *this;
}


BLocale::~BLocale()
{
}


status_t
BLocale::GetCollator(BCollator* collator) const
{
	if (!collator)
		return B_BAD_VALUE;

	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	*collator = fCollator;

	return B_OK;
}


status_t
BLocale::GetLanguage(BLanguage* language) const
{
	if (!language)
		return B_BAD_VALUE;

	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	*language = fLanguage;

	return B_OK;
}


status_t
BLocale::GetFormattingConventions(BFormattingConventions* conventions) const
{
	if (!conventions)
		return B_BAD_VALUE;

	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	*conventions = fConventions;

	return B_OK;
}


const char *
BLocale::GetString(uint32 id) const
{
	// Note: this code assumes a certain order of the string bases

	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return "";

	if (id >= B_OTHER_STRINGS_BASE) {
		if (id == B_CODESET)
			return "UTF-8";

		return "";
	}
	return fLanguage.GetString(id);
}


void
BLocale::SetFormattingConventions(const BFormattingConventions& conventions)
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return;

	fConventions = conventions;
}


void
BLocale::SetCollator(const BCollator& newCollator)
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return;

	fCollator = newCollator;
}


void
BLocale::SetLanguage(const BLanguage& newLanguage)
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return;

	fLanguage = newLanguage;
}
