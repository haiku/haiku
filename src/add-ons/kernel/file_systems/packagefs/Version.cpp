/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Version.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <new>

#include <NaturalCompare.h>

#include "DebugSupport.h"


static const char* const kVersionPartPlaceholder = "_";


static int
compare_version_part(const char* a, const char* b)
{
	if (a == NULL)
		return b != NULL ? -1 : 0;
	if (b == NULL)
		return 1;

	return BPrivate::NaturalCompare(a, b);
}


Version::Version()
	:
	fMajor(NULL),
	fMinor(NULL),
	fMicro(NULL),
	fRelease(0)
{
}


Version::~Version()
{
	free(fMajor);
	free(fMinor);
	free(fMicro);
}


status_t
Version::Init(const char* major, const char* minor, const char* micro,
	uint8 release)
{
	if (major != NULL) {
		fMajor = strdup(major);
		if (fMajor == NULL)
			return B_NO_MEMORY;
	}

	if (minor != NULL) {
		fMinor = strdup(minor);
		if (fMinor == NULL)
			return B_NO_MEMORY;
	}

	if (micro != NULL) {
		fMicro = strdup(micro);
		if (fMicro == NULL)
			return B_NO_MEMORY;
	}

	fRelease = release;

	return B_OK;
}


/*static*/ status_t
Version::Create(const char* major, const char* minor, const char* micro,
	uint8 release, Version*& _version)
{
	Version* version = new(std::nothrow) Version;
	if (version == NULL)
		return B_NO_MEMORY;

	status_t error = version->Init(major, minor, micro, release);
	if (error != B_OK) {
		delete version;
		return error;
	}

	_version = version;
	return B_OK;
}


int
Version::Compare(const Version& other) const
{
	int cmp = compare_version_part(fMajor, other.fMajor);
	if (cmp != 0)
		return cmp;

	cmp = compare_version_part(fMinor, other.fMinor);
	if (cmp != 0)
		return cmp;

	cmp = compare_version_part(fMicro, other.fMicro);
	if (cmp != 0)
		return cmp;

	return (int)fRelease - other.fRelease;
}


bool
Version::Compare(BPackageResolvableOperator op,
	const Version& other) const
{
	int cmp = Compare(other);

	switch (op) {
		case B_PACKAGE_RESOLVABLE_OP_LESS:
			return cmp < 0;
		case B_PACKAGE_RESOLVABLE_OP_LESS_EQUAL:
			return cmp <= 0;
		case B_PACKAGE_RESOLVABLE_OP_EQUAL:
			return cmp == 0;
		case B_PACKAGE_RESOLVABLE_OP_NOT_EQUAL:
			return cmp != 0;
		case B_PACKAGE_RESOLVABLE_OP_GREATER_EQUAL:
			return cmp >= 0;
		case B_PACKAGE_RESOLVABLE_OP_GREATER:
			return cmp > 0;
		default:
			ERROR("packagefs: Version::Compare(): Invalid operator %d\n", op);
			return false;
	}
}


size_t
Version::ToString(char* buffer, size_t bufferSize) const
{
	// We need to normalize the version string somewhat. If a subpart is given,
	// make sure that also the superparts are defined, using a placeholder. This
	// avoids clashes, e.g. if one version defines only major and one only
	// micro.
	const char* major = fMajor;
	const char* minor = fMinor;
	const char* micro = fMicro;

	if (micro != NULL && minor == NULL)
		minor = kVersionPartPlaceholder;
	if (minor != NULL && major == NULL)
		major = kVersionPartPlaceholder;

	if (micro != NULL) {
		return snprintf(buffer, bufferSize, "%s.%s.%s-%u", major, minor, micro,
			fRelease);
	}

	if (minor != NULL)
		return snprintf(buffer, bufferSize, "%s.%s-%u", major, minor, fRelease);

	if (major != NULL)
		return snprintf(buffer, bufferSize, "%s-%u", major, fRelease);

	return snprintf(buffer, bufferSize, "%u", fRelease);
}
