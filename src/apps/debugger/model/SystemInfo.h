/*
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <sys/utsname.h>

#include <OS.h>
#include <String.h>

#include "Types.h"


class SystemInfo {
public:
								SystemInfo();
								SystemInfo(const SystemInfo& other);
								SystemInfo(team_id team,
									const system_info& info,
									const utsname& name);

			void				SetTo(team_id team, const system_info& info,
									const utsname& name);

			team_id				TeamID() const	{ return fTeam; }

			const system_info&	GetSystemInfo() const	{ return fSystemInfo; }

			const utsname&		GetSystemName() const	{ return fSystemName; }

private:
			team_id				fTeam;
			system_info			fSystemInfo;
			utsname				fSystemName;
};


#endif // SYSTEM_INFO_H
