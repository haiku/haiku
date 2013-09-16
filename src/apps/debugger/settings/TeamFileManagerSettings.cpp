/*
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "TeamFileManagerSettings.h"

TeamFileManagerSettings::TeamFileManagerSettings()
	:
	fValues()
{
}


TeamFileManagerSettings::~TeamFileManagerSettings()
{
}


TeamFileManagerSettings&
TeamFileManagerSettings::operator=(const TeamFileManagerSettings& other)
{
	fValues = other.fValues;

	return *this;
}


const char*
TeamFileManagerSettings::ID() const
{
	return "FileManager";
}


status_t
TeamFileManagerSettings::SetTo(const BMessage& archive)
{
	try {
		fValues = archive;
	} catch (...) {
		return B_NO_MEMORY;
	}

	return B_OK;
}


status_t
TeamFileManagerSettings::WriteTo(BMessage& archive) const
{
	try {
		archive = fValues;
	} catch (...) {
		return B_NO_MEMORY;
	}

	return B_OK;
}


int32
TeamFileManagerSettings::CountSourceMappings() const
{
	type_code type;
	int32 count = 0;

	if (fValues.GetInfo("source:mapping", &type, &count) == B_OK)
		return count;

	return 0;
}


status_t
TeamFileManagerSettings::AddSourceMapping(const BString& sourcePath,
	const BString& locatedPath)
{
	BMessage mapping;
	if (mapping.AddString("source:path", sourcePath) != B_OK
		|| mapping.AddString("source:locatedpath", locatedPath) != B_OK
		|| fValues.AddMessage("source:mapping", &mapping) != B_OK) {
		return B_NO_MEMORY;
	}

	return B_OK;
}


status_t
TeamFileManagerSettings::RemoveSourceMappingAt(int32 index)
{
	return fValues.RemoveData("source:mapping", index);
}


status_t
TeamFileManagerSettings::GetSourceMappingAt(int32 index, BString& sourcePath,
	BString& locatedPath)
{
	BMessage mapping;
	status_t error = fValues.FindMessage("source:mapping", index, &mapping);
	if (error != B_OK)
		return error;

	error = mapping.FindString("source:path", &sourcePath);
	if (error != B_OK)
		return error;

	return mapping.FindString("source:locatedpath", &locatedPath);
}


TeamFileManagerSettings*
TeamFileManagerSettings::Clone() const
{
	TeamFileManagerSettings* settings = new(std::nothrow)
		TeamFileManagerSettings();

	if (settings == NULL)
		return NULL;

	if (settings->SetTo(fValues) != B_OK) {
		delete settings;
		return NULL;
	}

	return settings;
}
