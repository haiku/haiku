/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Version.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
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
	fPreRelease(NULL),
	fRelease(0)
{
}


Version::~Version()
{
	free(fMajor);
	free(fMinor);
	free(fMicro);
	free(fPreRelease);
}


status_t
Version::Init(const char* major, const char* minor, const char* micro,
	const char* preRelease, uint8 release)
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

	if (preRelease != NULL) {
		fPreRelease = strdup(preRelease);
		if (fPreRelease == NULL)
			return B_NO_MEMORY;
	}

	fRelease = release;

	return B_OK;
}


/*static*/ status_t
Version::Create(const char* major, const char* minor, const char* micro,
	const char* preRelease, uint8 release, Version*& _version)
{
	Version* version = new(std::nothrow) Version;
	if (version == NULL)
		return B_NO_MEMORY;

	status_t error = version->Init(major, minor, micro, preRelease, release);
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

	// The pre-version works differently: The empty string is greater than any
	// non-empty string (e.g. "R1" is newer than "R1-rc2"). So we catch the
	// empty string cases first.
	if (fPreRelease == NULL) {
		if (other.fPreRelease != NULL)
			return 1;
	} else if (other.fPreRelease == NULL) {
		return -1;
	} else {
		// both are non-null -- compare normally
		cmp = BPrivate::NaturalCompare(fPreRelease, other.fPreRelease);
		if (cmp != 0)
			return cmp;
	}

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
	// avoids clashes, e.g. if one version defines major and minor and one only
	// major and micro.
	const char* major = fMajor;
	const char* minor = fMinor;
	const char* micro = fMicro;

	if (micro != NULL && minor == NULL)
		minor = kVersionPartPlaceholder;
	if (minor != NULL && major == NULL)
		major = kVersionPartPlaceholder;

	size_t size = strlcpy(buffer, major, bufferSize);

	if (minor != NULL) {
		size_t offset = std::min(bufferSize, size);
		size += snprintf(buffer + offset, bufferSize - offset, ".%s", minor);
	}

	if (micro != NULL) {
		size_t offset = std::min(bufferSize, size);
		size += snprintf(buffer + offset, bufferSize - offset, ".%s", micro);
	}

	if (fPreRelease != NULL) {
		size_t offset = std::min(bufferSize, size);
		size += snprintf(buffer + offset, bufferSize - offset, "-%s",
			fPreRelease);
	}

	if (fRelease != 0) {
		size_t offset = std::min(bufferSize, size);
		size += snprintf(buffer + offset, bufferSize - offset, "-%u", fRelease);
	}

	return size;
}
