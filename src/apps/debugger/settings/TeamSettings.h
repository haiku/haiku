/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_SETTINGS_H
#define TEAM_SETTINGS_H


#include <String.h>

#include <ObjectList.h>


class BMessage;
class Team;
class BreakpointSetting;


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

			TeamSettings&		operator=(const TeamSettings& other);
									// throws std::bad_alloc

private:
			typedef BObjectList<BreakpointSetting> BreakpointList;

private:
			void				_Unset();

private:
			BreakpointList		fBreakpoints;
			BString				fTeamName;
};


#endif	// TEAM_SETTINGS_H
