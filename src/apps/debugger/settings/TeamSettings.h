/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2013-2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_SETTINGS_H
#define TEAM_SETTINGS_H


#include <String.h>

#include <ObjectList.h>


class BMessage;
class BreakpointSetting;
class Team;
class TeamFileManagerSettings;
class TeamSignalSettings;
class TeamUiSettings;


class TeamSettings {
public:
								TeamSettings();
								TeamSettings(const TeamSettings& other);
									// throws std::bad_alloc
								~TeamSettings();

			status_t			SetTo(Team* team);
			status_t			SetTo(const BMessage& archive);
			status_t			WriteTo(BMessage& archive) const;

			const BString&		TeamName() const	{ return fTeamName; }

			int32				CountBreakpoints() const;
			const BreakpointSetting* BreakpointAt(int32 index) const;

			int32				CountUiSettings() const;
			const TeamUiSettings*	UiSettingAt(int32 index) const;
			const TeamUiSettings*	UiSettingFor(const char* id) const;
			status_t			AddUiSettings(TeamUiSettings* settings);

			TeamSettings&		operator=(const TeamSettings& other);
									// throws std::bad_alloc

			TeamFileManagerSettings*
								FileManagerSettings() const;
			status_t			SetFileManagerSettings(
									TeamFileManagerSettings* settings);

			TeamSignalSettings* SignalSettings() const;
			status_t			SetSignalSettings(
									TeamSignalSettings* settings);

private:
			typedef BObjectList<BreakpointSetting> BreakpointList;
			typedef BObjectList<TeamUiSettings> UiSettingsList;

private:
			void				_Unset();

private:
			BreakpointList		fBreakpoints;
			UiSettingsList		fUiSettings;
			TeamFileManagerSettings*
								fFileManagerSettings;
			TeamSignalSettings*	fSignalSettings;
			BString				fTeamName;
};


#endif	// TEAM_SETTINGS_H
