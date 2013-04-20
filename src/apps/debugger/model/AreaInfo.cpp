/*
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "AreaInfo.h"


AreaInfo::AreaInfo()
	:
	fTeam(-1),
	fArea(-1),
	fName(),
	fAddress(0),
	fSize(0),
	fRamSize(0),
	fLock(0),
	fProtection(0)
{
}


AreaInfo::AreaInfo(const AreaInfo &other)
	:
	fTeam(other.fTeam),
	fArea(other.fArea),
	fName(other.fName),
	fAddress(other.fAddress),
	fSize(other.fSize),
	fRamSize(other.fRamSize),
	fLock(other.fLock),
	fProtection(other.fProtection)
{
}


AreaInfo::AreaInfo(team_id team, area_id area, const BString& name,
	target_addr_t address, target_size_t size, target_size_t ramSize,
	uint32 lock, uint32 protection)
	:
	fTeam(team),
	fArea(area),
	fName(name),
	fAddress(address),
	fSize(size),
	fRamSize(ramSize),
	fLock(lock),
	fProtection(protection)
{
}


void
AreaInfo::SetTo(team_id team, area_id area, const BString& name,
	target_addr_t address, target_size_t size, target_size_t ramSize,
	uint32 lock, uint32 protection)
{
	fTeam = team;
	fArea = area;
	fName = name;
	fAddress = address;
	fSize = size;
	fRamSize = ramSize;
	fLock = lock;
	fProtection = protection;
}
