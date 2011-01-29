/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/PackageVersion.h>

#include <NaturalCompare.h>


using BPrivate::NaturalCompare;


namespace BPackageKit {


BPackageVersion::BPackageVersion()
{
}


BPackageVersion::BPackageVersion(const BString& major, const BString& minor,
	const BString& micro, uint8 release)
	:
	fMajor(major),
	fMinor(minor),
	fMicro(micro),
	fRelease(release)
{
}


status_t
BPackageVersion::InitCheck() const
{
	return fMajor.Length() > 0 ? B_OK : B_NO_INIT;
}


int
BPackageVersion::Compare(const BPackageVersion& other) const
{
	int majorDiff = NaturalCompare(fMajor.String(), other.fMajor.String());
	if (majorDiff != 0)
		return majorDiff;

	int minorDiff = NaturalCompare(fMinor.String(), other.fMinor.String());
	if (minorDiff != 0)
		return minorDiff;

	int microDiff = NaturalCompare(fMicro.String(), other.fMicro.String());
	if (microDiff != 0)
		return microDiff;

	return (int)fRelease - (int)other.fRelease;
}


void
BPackageVersion::GetAsString(BString& string) const
{
	string = fMajor;

	if (fMinor.Length() > 0) {
		string << '.' << fMinor;
		if (fMicro.Length() > 0)
			string << '.' << fMicro;
	}

	if (fRelease > 0)
		string << '-' << fRelease;
}


void
BPackageVersion::SetTo(const BString& major, const BString& minor,
	const BString& micro, uint8 release)
{
	fMajor = major;
	fMinor = minor;
	fMicro = micro;
	fRelease = release;
}


void
BPackageVersion::Clear()
{
	fMajor.Truncate(0);
	fMinor.Truncate(0);
	fMicro.Truncate(0);
	fRelease = 0;
}


}	// namespace BPackageKit
