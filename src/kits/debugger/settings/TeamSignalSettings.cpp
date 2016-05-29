/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "TeamSignalSettings.h"


static const char* skDefaultSignalFieldName = "signal:default_disposition";
static const char* skSignalNumberFieldName = "signal:number";
static const char* skSignalDispositionFieldName = "signal:disposition";
static const char* skSignalSettingName = "signal:setting";


TeamSignalSettings::TeamSignalSettings()
	:
	fValues()
{
}


TeamSignalSettings::~TeamSignalSettings()
{
}


TeamSignalSettings&
TeamSignalSettings::operator=(const TeamSignalSettings& other)
{
	fValues = other.fValues;

	return *this;
}


const char*
TeamSignalSettings::ID() const
{
	return "Signals";
}


status_t
TeamSignalSettings::SetTo(const BMessage& archive)
{
	try {
		fValues = archive;
	} catch (...) {
		return B_NO_MEMORY;
	}

	return B_OK;
}


status_t
TeamSignalSettings::WriteTo(BMessage& archive) const
{
	try {
		archive = fValues;
	} catch (...) {
		return B_NO_MEMORY;
	}

	return B_OK;
}


void
TeamSignalSettings::Unset()
{
	fValues.MakeEmpty();
}


void
TeamSignalSettings::SetDefaultSignalDisposition(int32 disposition)
{
	fValues.SetInt32(skDefaultSignalFieldName, disposition);
}


int32
TeamSignalSettings::DefaultSignalDisposition() const
{
	return fValues.GetInt32(skDefaultSignalFieldName,
		SIGNAL_DISPOSITION_IGNORE);
}


int32
TeamSignalSettings::CountCustomSignalDispositions() const
{
	type_code type;
	int32 count = 0;

	if (fValues.GetInfo(skSignalSettingName, &type, &count) == B_OK)
		return count;

	return 0;
}


status_t
TeamSignalSettings::AddCustomSignalDisposition(int32 signal, int32 disposition)
{
	BMessage setting;
	if (setting.AddInt32(skSignalNumberFieldName, signal) != B_OK
		|| setting.AddInt32(skSignalDispositionFieldName, disposition) != B_OK
		|| fValues.AddMessage(skSignalSettingName, &setting) != B_OK) {
		return B_NO_MEMORY;
	}

	return B_OK;
}


status_t
TeamSignalSettings::RemoveCustomSignalDispositionAt(int32 index)
{
	return fValues.RemoveData(skSignalSettingName, index);
}


status_t
TeamSignalSettings::GetCustomSignalDispositionAt(int32 index, int32& signal,
	int32& disposition) const
{
	BMessage setting;
	status_t error = fValues.FindMessage(skSignalSettingName, index, &setting);
	if (error != B_OK)
		return error;

	error = setting.FindInt32(skSignalNumberFieldName, &signal);
	if (error != B_OK)
		return error;

	return setting.FindInt32(skSignalDispositionFieldName, &disposition);
}


TeamSignalSettings*
TeamSignalSettings::Clone() const
{
	TeamSignalSettings* settings = new(std::nothrow)
		TeamSignalSettings();

	if (settings == NULL)
		return NULL;

	if (settings->SetTo(fValues) != B_OK) {
		delete settings;
		return NULL;
	}

	return settings;
}
