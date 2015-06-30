/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2013-2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "TeamSettings.h"

#include <new>

#include <Message.h>

#include <AutoLocker.h>

#include "ArchivingUtils.h"
#include "BreakpointSetting.h"
#include "Team.h"
#include "TeamFileManagerSettings.h"
#include "TeamSignalSettings.h"
#include "TeamUiSettings.h"
#include "TeamUiSettingsFactory.h"
#include "UserBreakpoint.h"


TeamSettings::TeamSettings()
{
	fFileManagerSettings = new TeamFileManagerSettings();
	fSignalSettings = new TeamSignalSettings();
}


TeamSettings::TeamSettings(const TeamSettings& other)
{
	try {
		*this = other;
	} catch (...) {
		_Unset();
		throw;
	}
}


TeamSettings::~TeamSettings()
{
	_Unset();
}


status_t
TeamSettings::SetTo(Team* team)
{
	_Unset();

	AutoLocker<Team> locker(team);

	fTeamName = team->Name();

	// add breakpoints
	for (UserBreakpointList::ConstIterator it
			= team->UserBreakpoints().GetIterator();
		UserBreakpoint* breakpoint = it.Next();) {
		BreakpointSetting* breakpointSetting
			= new(std::nothrow) BreakpointSetting;
		if (breakpointSetting == NULL)
			return B_NO_MEMORY;

		status_t error = breakpointSetting->SetTo(breakpoint->Location(),
			breakpoint->IsEnabled(), breakpoint->IsHidden(),
			breakpoint->Condition());
		if (error == B_OK && !fBreakpoints.AddItem(breakpointSetting))
			error = B_NO_MEMORY;
		if (error != B_OK) {
			delete breakpointSetting;
			return error;
		}
	}

	// add signal configuration

	fSignalSettings->SetDefaultSignalDisposition(
		team->DefaultSignalDisposition());

	const SignalDispositionMappings& mappings
		= team->GetSignalDispositionMappings();

	for (SignalDispositionMappings::const_iterator it = mappings.begin();
		it != mappings.end(); ++it) {
		status_t error = fSignalSettings->AddCustomSignalDisposition(
			it->first, it->second);
		if (error != B_OK)
			return error;
	}

	return B_OK;
}


status_t
TeamSettings::SetTo(const BMessage& archive)
{
	_Unset();

	status_t error = archive.FindString("teamName", &fTeamName);
	if (error != B_OK)
		return error;

	// add breakpoints
	BMessage childArchive;
	for (int32 i = 0; archive.FindMessage("breakpoints", i, &childArchive)
			== B_OK; i++) {
		BreakpointSetting* breakpointSetting
			= new(std::nothrow) BreakpointSetting;
		if (breakpointSetting == NULL)
			return B_NO_MEMORY;

		error = breakpointSetting->SetTo(childArchive);
		if (error == B_OK && !fBreakpoints.AddItem(breakpointSetting))
			error = B_NO_MEMORY;
		if (error != B_OK) {
			delete breakpointSetting;
			return error;
		}
	}

	// add UI settings
	for (int32 i = 0; archive.FindMessage("uisettings", i, &childArchive)
		== B_OK; i++) {
		TeamUiSettings* setting = NULL;
		error = TeamUiSettingsFactory::Create(childArchive, setting);
		if (error == B_OK && !fUiSettings.AddItem(setting))
			error = B_NO_MEMORY;
		if (error != B_OK) {
			delete setting;
			return error;
		}
	}

	if (archive.FindMessage("filemanagersettings", &childArchive) == B_OK) {
		error = fFileManagerSettings->SetTo(childArchive);
		if (error != B_OK)
			return error;
	}

	if (archive.FindMessage("signalsettings", &childArchive) == B_OK) {
		error = fSignalSettings->SetTo(childArchive);
		if (error != B_OK)
			return error;
	}

	return B_OK;
}


status_t
TeamSettings::WriteTo(BMessage& archive) const
{
	status_t error = archive.AddString("teamName", fTeamName);
	if (error != B_OK)
		return error;

	BMessage childArchive;
	for (int32 i = 0; BreakpointSetting* breakpoint = fBreakpoints.ItemAt(i);
			i++) {
		error = breakpoint->WriteTo(childArchive);
		if (error != B_OK)
			return error;

		error = archive.AddMessage("breakpoints", &childArchive);
		if (error != B_OK)
			return error;
	}

	for (int32 i = 0; TeamUiSettings* uiSetting = fUiSettings.ItemAt(i);
			i++) {
		error = uiSetting->WriteTo(childArchive);
		if (error != B_OK)
			return error;

		error = archive.AddMessage("uisettings", &childArchive);
		if (error != B_OK)
			return error;
	}

	error = fFileManagerSettings->WriteTo(childArchive);
	if (error != B_OK)
		return error;

	error = archive.AddMessage("filemanagersettings", &childArchive);
	if (error != B_OK)
		return error;

	error = fSignalSettings->WriteTo(childArchive);
	if (error != B_OK)
		return error;

	error = archive.AddMessage("signalsettings", &childArchive);
	if (error != B_OK)
		return error;

	return B_OK;
}


int32
TeamSettings::CountBreakpoints() const
{
	return fBreakpoints.CountItems();
}


const BreakpointSetting*
TeamSettings::BreakpointAt(int32 index) const
{
	return fBreakpoints.ItemAt(index);
}


int32
TeamSettings::CountUiSettings() const
{
	return fUiSettings.CountItems();
}


const TeamUiSettings*
TeamSettings::UiSettingAt(int32 index) const
{
	return fUiSettings.ItemAt(index);
}


const TeamUiSettings*
TeamSettings::UiSettingFor(const char* id) const
{
	for (int32 i = 0; i < fUiSettings.CountItems(); i++) {
		TeamUiSettings* settings = fUiSettings.ItemAt(i);
		if (strcmp(settings->ID(), id) == 0)
			return settings;
	}

	return NULL;
}


status_t
TeamSettings::AddUiSettings(TeamUiSettings* settings)
{
	if (!fUiSettings.AddItem(settings))
		return B_NO_MEMORY;

	return B_OK;
}


TeamSettings&
TeamSettings::operator=(const TeamSettings& other)
{
	if (this == &other)
		return *this;

	_Unset();

	fTeamName = other.fTeamName;

	for (int32 i = 0; BreakpointSetting* breakpoint
			= other.fBreakpoints.ItemAt(i); i++) {
		BreakpointSetting* clonedBreakpoint
			= new BreakpointSetting(*breakpoint);
		if (!fBreakpoints.AddItem(clonedBreakpoint)) {
			delete clonedBreakpoint;
			throw std::bad_alloc();
		}
	}

	for (int32 i = 0; TeamUiSettings* uiSetting
			= other.fUiSettings.ItemAt(i); i++) {
		TeamUiSettings* clonedSetting
			= uiSetting->Clone();
		if (!fUiSettings.AddItem(clonedSetting)) {
			delete clonedSetting;
			throw std::bad_alloc();
		}
	}

	*fFileManagerSettings = *other.fFileManagerSettings;

	*fSignalSettings = *other.fSignalSettings;

	return *this;
}


TeamFileManagerSettings*
TeamSettings::FileManagerSettings() const
{
	return fFileManagerSettings;
}


status_t
TeamSettings::SetFileManagerSettings(TeamFileManagerSettings* settings)
{
	try {
		*fFileManagerSettings = *settings;
	} catch (...) {
		return B_NO_MEMORY;
	}

	return B_OK;
}


TeamSignalSettings*
TeamSettings::SignalSettings() const
{
	return fSignalSettings;
}


status_t
TeamSettings::SetSignalSettings(TeamSignalSettings* settings)
{
	try {
		*fSignalSettings = *settings;
	} catch (...) {
		return B_NO_MEMORY;
	}

	return B_OK;
}


void
TeamSettings::_Unset()
{
	for (int32 i = 0; BreakpointSetting* breakpoint = fBreakpoints.ItemAt(i);
			i++) {
		delete breakpoint;
	}

	for (int32 i = 0; TeamUiSettings* uiSetting = fUiSettings.ItemAt(i); i++)
		delete uiSetting;

	fBreakpoints.MakeEmpty();
	fUiSettings.MakeEmpty();
	fSignalSettings->Unset();

	fTeamName.Truncate(0);
}
