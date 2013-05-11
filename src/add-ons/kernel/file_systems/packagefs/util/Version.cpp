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
compare_version_part(const String& a, const String& b)
{
	if (a.IsEmpty())
		return b.IsEmpty() ? 0 : -1;
	if (b.IsEmpty())
		return 1;

	return BPrivate::NaturalCompare(a, b);
}


Version::Version()
	:
	fMajor(),
	fMinor(),
	fMicro(),
	fPreRelease(),
	fRevision(0)
{
}


Version::~Version()
{
}


status_t
Version::Init(const char* major, const char* minor, const char* micro,
	const char* preRelease, uint32 revision)
{
	if (major != NULL) {
		if (!fMajor.SetTo(major))
			return B_NO_MEMORY;
	}

	if (minor != NULL) {
		if (!fMinor.SetTo(minor))
			return B_NO_MEMORY;
	}

	if (micro != NULL) {
		if (!fMicro.SetTo(micro))
			return B_NO_MEMORY;
	}

	if (preRelease != NULL) {
		if (!fPreRelease.SetTo(preRelease))
			return B_NO_MEMORY;
	}

	fRevision = revision;

	return B_OK;
}


/*static*/ status_t
Version::Create(const char* major, const char* minor, const char* micro,
	const char* preRelease, uint32 revision, Version*& _version)
{
	Version* version = new(std::nothrow) Version;
	if (version == NULL)
		return B_NO_MEMORY;

	status_t error = version->Init(major, minor, micro, preRelease, revision);
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
	if (fPreRelease.IsEmpty()) {
		if (!other.fPreRelease.IsEmpty())
			return 1;
	} else if (other.fPreRelease.IsEmpty()) {
		return -1;
	} else {
		// both are non-null -- compare normally
		cmp = BPrivate::NaturalCompare(fPreRelease, other.fPreRelease);
		if (cmp != 0)
			return cmp;
	}

	return fRevision == other.fRevision
		? 0 : (fRevision < other.fRevision ? -1 : 1);
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
	// major and micro. In principle that should not be necessary, though. Valid
	// packages should have valid versions, which means that the existence of a
	// subpart implies the existence of all superparts.
	const char* major = fMajor;
	const char* minor = fMinor;
	const char* micro = fMicro;

	if (micro[0] != '\0' && minor[0] == '\0')
		minor = kVersionPartPlaceholder;
	if (minor[0] != '\0' && major[0] == '\0')
		major = kVersionPartPlaceholder;

	size_t size = strlcpy(buffer, major, bufferSize);

	if (minor[0] != '\0') {
		size_t offset = std::min(bufferSize, size);
		size += snprintf(buffer + offset, bufferSize - offset, ".%s", minor);
	}

	if (micro[0] != '\0') {
		size_t offset = std::min(bufferSize, size);
		size += snprintf(buffer + offset, bufferSize - offset, ".%s", micro);
	}

	if (fPreRelease[0] != '\0') {
		size_t offset = std::min(bufferSize, size);
		size += snprintf(buffer + offset, bufferSize - offset, "~%s",
			fPreRelease.Data());
	}

	if (fRevision != 0) {
		size_t offset = std::min(bufferSize, size);
		size += snprintf(buffer + offset, bufferSize - offset, "-%" B_PRIu32,
			fRevision);
	}

	return size;
}
