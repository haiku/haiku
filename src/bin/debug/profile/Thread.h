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


class Thread : public DoublyLinkedListLinkImpl<Thread> {
public:
								Thread(const thread_info& info, Team* team);
								~Thread();

	inline	thread_id			ID() const;
	inline	const char*			Name() const;
	inline	addr_t*				Samples() const;
	inline	Team*				GetTeam() const;

			void				UpdateInfo();

			void				SetSampleArea(area_id area, addr_t* samples);
			void				SetInterval(bigtime_t interval);

			status_t			AddImage(Image* image);
			void				ImageRemoved(Image* image);
			ThreadImage*		FindImage(addr_t address) const;

			void				AddSamples(int32 count, int32 dropped,
									int32 stackDepth, bool variableStackDepth,
									int32 event);
			void				PrintResults() const;

private:
	typedef DoublyLinkedList<ThreadImage>	ImageList;

			void				_SynchronizeImages();

			void				_AddNewImages(ImageList& newImages,
									int32 event);
			void				_RemoveObsoleteImages(int32 event);

			ThreadImage*		_FindImageByID(image_id id) const;

private:
			thread_info			fInfo;
			::Team*				fTeam;
			area_id				fSampleArea;
			addr_t*				fSamples;
			ImageList			fImages;
			ImageList			fOldImages;
			int64				fTotalTicks;
			int64				fUnkownTicks;
			int64				fDroppedTicks;
			bigtime_t			fInterval;
			int64				fTotalSampleCount;
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


#endif	// THREAD_H
