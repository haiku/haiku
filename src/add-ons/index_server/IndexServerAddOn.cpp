/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */

#include "IndexServerAddOn.h"

#include <Debug.h>
#include <Directory.h>
#include <File.h>
#include <Path.h>

#include "IndexServerPrivate.h"


analyser_settings::analyser_settings()
	:
	catchUpEnabled(true),

	syncPosition(0),
	watchingStart(0),
	watchingPosition(0)
{
	
}


const char* kAnalyserStatusFile = "AnalyserStatus";

const char* kCatchUpEnabledAttr = "CatchUpEnabled";
const char* kSyncPositionAttr = "SyncPosition";
const char* kWatchingStartAttr = "WatchingStart";
const char* kWatchingPositionAttr = "WatchingPosition";


AnalyserSettings::AnalyserSettings(const BString& name, const BVolume& volume)
	:
	fName(name),
	fVolume(volume)
{
	ReadSettings();
}


bool
AnalyserSettings::ReadSettings()
{
	BAutolock _(fSettingsLock);

	BDirectory rootDir;
	fVolume.GetRootDirectory(&rootDir);
	BPath path(&rootDir);
	path.Append(kIndexServerDirectory);
	path.Append(fName);
	path.Append(kAnalyserStatusFile);

	BFile file(path.Path(), B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return false;

	uint32 value;
	file.ReadAttr(kCatchUpEnabledAttr, B_UINT32_TYPE, 0, &value,
		sizeof(uint32));
	fAnalyserSettings.catchUpEnabled = value != 0 ? true : false;
	file.ReadAttr(kSyncPositionAttr, B_INT64_TYPE, 0,
		&fAnalyserSettings.syncPosition, sizeof(int64));
	file.ReadAttr(kWatchingStartAttr, B_INT64_TYPE, 0,
		&fAnalyserSettings.watchingStart, sizeof(int64));
	file.ReadAttr(kWatchingPositionAttr, B_INT64_TYPE, 0,
		&fAnalyserSettings.watchingPosition, sizeof(int64));

	return true;
}


bool
AnalyserSettings::WriteSettings()
{
	BAutolock _(fSettingsLock);

	BDirectory rootDir;
	fVolume.GetRootDirectory(&rootDir);
	BPath path(&rootDir);
	path.Append(kIndexServerDirectory);
	path.Append(fName);
	if (create_directory(path.Path(), 777) != B_OK)
		return false;
	path.Append(kAnalyserStatusFile);
	
	BFile file(path.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	if (file.InitCheck() != B_OK)
		return false;

	uint32 value = fAnalyserSettings.catchUpEnabled ? 1 : 0;
	file.WriteAttr(kCatchUpEnabledAttr, B_UINT32_TYPE, 0, &value,
		sizeof(uint32));
	file.WriteAttr(kSyncPositionAttr, B_INT64_TYPE, 0,
		&fAnalyserSettings.syncPosition, sizeof(int64));
	file.WriteAttr(kWatchingStartAttr, B_INT64_TYPE, 0,
		&fAnalyserSettings.watchingStart, sizeof(int64));
	file.WriteAttr(kWatchingPositionAttr, B_INT64_TYPE, 0,
		&fAnalyserSettings.watchingPosition, sizeof(int64));

	return true;
}


analyser_settings
AnalyserSettings::RawSettings()
{
	BAutolock _(fSettingsLock);

	return fAnalyserSettings;
}


void
AnalyserSettings::SetCatchUpEnabled(bool enabled)
{
	BAutolock _(fSettingsLock);

	fAnalyserSettings.catchUpEnabled = enabled;
}


void
AnalyserSettings::SetSyncPosition(bigtime_t time)
{
	BAutolock _(fSettingsLock);

	fAnalyserSettings.syncPosition = time;
}


void
AnalyserSettings::SetWatchingStart(bigtime_t time)
{
	BAutolock _(fSettingsLock);

	fAnalyserSettings.watchingStart = time;
}


void
AnalyserSettings::SetWatchingPosition(bigtime_t time)
{
	BAutolock _(fSettingsLock);

	fAnalyserSettings.watchingPosition = time;
}


bool
AnalyserSettings::CatchUpEnabled()
{
	BAutolock _(fSettingsLock);

	return fAnalyserSettings.catchUpEnabled;
}


bigtime_t
AnalyserSettings::SyncPosition()
{
	BAutolock _(fSettingsLock);

	return fAnalyserSettings.syncPosition;
}


bigtime_t
AnalyserSettings::WatchingStart()
{
	BAutolock _(fSettingsLock);

	return fAnalyserSettings.watchingStart;
}


bigtime_t
AnalyserSettings::WatchingPosition()
{
	BAutolock _(fSettingsLock);

	return fAnalyserSettings.watchingPosition;
}


FileAnalyser::FileAnalyser(const BString& name, const BVolume& volume)
	:
	fVolume(volume),
	fName(name)
{

}


void
FileAnalyser::SetSettings(AnalyserSettings* settings)
{
	ASSERT(fName == settings->Name() && fVolume == settings->Volume());

	fAnalyserSettings = settings;
	ASSERT(fAnalyserSettings.Get());
	UpdateSettingsCache();
}


AnalyserSettings*
FileAnalyser::Settings() const
{
	return fAnalyserSettings;
}


const analyser_settings&
FileAnalyser::CachedSettings() const
{
	return fCachedSettings;
}


void
FileAnalyser::UpdateSettingsCache()
{
	fCachedSettings = fAnalyserSettings->RawSettings();
}
