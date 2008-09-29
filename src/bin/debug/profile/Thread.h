/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef THREAD_H
#define THREAD_H

#include <util/DoublyLinkedList.h>

#include "Image.h"


class Team;
class ThreadImage;
class ThreadProfileResult;


class Thread : public DoublyLinkedListLinkImpl<Thread> {
public:
								Thread(const thread_info& info, Team* team);
								~Thread();

	inline	thread_id			ID() const;
	inline	const char*			Name() const;
	inline	addr_t*				Samples() const;
	inline	Team*				GetTeam() const;

	inline	ThreadProfileResult* ProfileResult() const;
			void				SetProfileResult(ThreadProfileResult* result);

			void				UpdateInfo();

			void				SetSampleArea(area_id area, addr_t* samples);
			void				SetInterval(bigtime_t interval);

			status_t			AddImage(Image* image);

			void				AddSamples(int32 count, int32 dropped,
									int32 stackDepth, bool variableStackDepth,
									int32 event);
			void				PrintResults() const;

private:
	typedef DoublyLinkedList<ThreadImage>	ImageList;

private:
			thread_info			fInfo;
			::Team*				fTeam;
			area_id				fSampleArea;
			addr_t*				fSamples;
			ThreadProfileResult* fProfileResult;
};


class ThreadProfileResult {
public:
								ThreadProfileResult();
	virtual						~ThreadProfileResult();

	virtual	status_t			Init(Thread* thread);

			void				SetInterval(bigtime_t interval);

	virtual	status_t			AddImage(Image* image) = 0;
	virtual	void				SynchronizeImages(int32 event) = 0;

	virtual	void				AddSamples(addr_t* samples,
									int32 sampleCount) = 0;
	virtual	void				AddDroppedTicks(int32 dropped) = 0;
	virtual	void				PrintResults() = 0;

protected:
			Thread*				fThread;
			bigtime_t			fInterval;
};


class AbstractThreadProfileResult : public ThreadProfileResult {
public:
								AbstractThreadProfileResult();
	virtual						~AbstractThreadProfileResult();

	virtual	status_t			AddImage(Image* image);
	virtual	void				SynchronizeImages(int32 event);

			ThreadImage*		FindImage(addr_t address) const;

	virtual	void				AddSamples(addr_t* samples,
									int32 sampleCount) = 0;
	virtual	void				AddDroppedTicks(int32 dropped) = 0;
	virtual	void				PrintResults() = 0;

	virtual ThreadImage*		CreateThreadImage(Image* image) = 0;

protected:
	typedef DoublyLinkedList<ThreadImage>	ImageList;

			ImageList			fImages;
			ImageList			fNewImages;
			ImageList			fOldImages;
};


class BasicThreadProfileResult : public AbstractThreadProfileResult {
public:
								BasicThreadProfileResult();

	virtual	void				AddDroppedTicks(int32 dropped);
	virtual	void				PrintResults();

	virtual ThreadImage*		CreateThreadImage(Image* image);

protected:
			int64				fTotalTicks;
			int64				fUnkownTicks;
			int64				fDroppedTicks;
			int64				fTotalSampleCount;
};


class InclusiveThreadProfileResult : public BasicThreadProfileResult {
public:
	virtual	void				AddSamples(addr_t* samples,
									int32 sampleCount);
};


class ExclusiveThreadProfileResult : public BasicThreadProfileResult {
public:
	virtual	void				AddSamples(addr_t* samples,
									int32 sampleCount);
};


// #pragma mark -


thread_id
Thread::ID() const
{
	return fInfo.thread;
}


const char*
Thread::Name() const
{
	return fInfo.name;
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


ThreadProfileResult*
Thread::ProfileResult() const
{
	return fProfileResult;
}


#endif	// THREAD_H
