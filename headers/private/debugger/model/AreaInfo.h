/*
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef AREA_INFO_H
#define AREA_INFO_H

#include <OS.h>
#include <String.h>

#include "Types.h"


class AreaInfo {
public:
								AreaInfo();
								AreaInfo(const AreaInfo& other);
								AreaInfo(team_id team, area_id area,
									const BString& name, target_addr_t address,
									target_size_t size, target_size_t ram_size,
									uint32 lock, uint32 protection);

			void				SetTo(team_id team, area_id area,
									const BString& name, target_addr_t address,
									target_size_t size, target_size_t ram_size,
									uint32 lock, uint32 protection);

			team_id				TeamID() const	{ return fTeam; }
			area_id				AreaID() const	{ return fArea; }
			const BString&		Name() const	{ return fName; }

			target_addr_t		BaseAddress() const	{ return fAddress; }
			target_size_t		Size() const		{ return fSize; }
			target_size_t		RamSize() const 	{ return fRamSize; }
			uint32				Lock() const		{ return fLock; }
			uint32				Protection() const	{ return fProtection; }


private:
			team_id				fTeam;
			area_id				fArea;
			BString				fName;
			target_addr_t		fAddress;
			target_size_t		fSize;
			target_size_t		fRamSize;
			uint32				fLock;
			uint32				fProtection;
};


#endif // AREA_INFO_H
