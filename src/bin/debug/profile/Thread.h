/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef THREAD_H
#define THREAD_H


#include <String.h>

#include <util/DoublyLinkedList.h>

#include "ProfiledEntity.h"
#include "ProfileResult.h"


class Team;


class Thread : public ProfiledEntity, public DoublyLinkedListLinkImpl<Thread> {
public:
								Thread(thread_id threadID, const char* name,
									Team* team);
	virtual						~Thread();

	inline	thread_id			ID() const;
	inline	const char*			Name() const;
	inline	addr_t*				Samples() const;
	inline	Team*				GetTeam() const;

	virtual	int32				EntityID() const;
	virtual	const char*			EntityName() const;
	virtual	const char*			EntityType() const;

	inline	ProfileResult*		GetProfileResult() const;
			void				SetProfileResult(ProfileResult* result);

			void				UpdateInfo(const char* name);

			void				SetSampleArea(area_id area, addr_t* samples);
			void				SetInterval(bigtime_t interval);

	inline	status_t			AddImage(Image* image);
	inline	void				RemoveImage(Image* image);

			void				AddSamples(int32 count, int32 dropped,
									int32 stackDepth, bool variableStackDepth,
									int32 event);
			void				AddSamples(addr_t* samples, int32 sampleCount);
			void				PrintResults() const;

private:
			thread_id			fID;
			BString				fName;
			::Team*				fTeam;
			area_id				fSampleArea;
			addr_t*				fSamples;
			ProfileResult*		fProfileResult;
};


thread_id
Thread::ID() const
{
	return fID;
}


const char*
Thread::Name() const
{
	return fName.String();
}


addr_t*
Thread::Samples() const
{
	return fSamples;
}


Team*
Thread::GetTeam() const
{
	return fTeam;
}


ProfileResult*
Thread::GetProfileResult() const
{
	return fProfileResult;
}


status_t
Thread::AddImage(Image* image)
{
	return fProfileResult->AddImage(image);
}


void
Thread::RemoveImage(Image* image)
{
	fProfileResult->RemoveImage(image);
}


#endif	// THREAD_H
