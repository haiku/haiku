/*
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef SEMAPHORE_INFO_H
#define SEMAPHORE_INFO_H

#include <OS.h>
#include <String.h>

#include "Types.h"


class SemaphoreInfo {
public:
								SemaphoreInfo();
								SemaphoreInfo(const SemaphoreInfo& other);
								SemaphoreInfo(team_id team, sem_id semaphore,
									const BString& name, int32 count,
									thread_id latestHolder);

			void				SetTo(team_id team, sem_id semaphore,
									const BString& name, int32 count,
									thread_id latestHolder);

			team_id				TeamID() const	{ return fTeam; }
			area_id				SemID() const	{ return fSemaphore; }
			const BString&		Name() const	{ return fName; }

			int32				Count() const	{ return fCount; }
			thread_id			LatestHolder() const
												{ return fLatestHolder; }
private:
			team_id				fTeam;
			sem_id				fSemaphore;
			BString				fName;
			int32				fCount;
			thread_id			fLatestHolder;
};


#endif // AREA_INFO_H
