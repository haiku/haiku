/* 
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef MESSAGING_SERVICE_H
#define MESSAGING_SERVICE_H

#include <MessagingServiceDefs.h>

// MessagingArea
class MessagingArea {
public:
	~MessagingArea();

	static status_t Create(area_id kernelAreaID, sem_id lockSem,
		sem_id counterSem, MessagingArea *&area);

	bool Lock();
	void Unlock();

	area_id ID() const;
	int32 Size() const;

	int32 CountCommands() const;
	const messaging_command *PopCommand();

	void Discard();

	area_id NextKernelAreaID() const;
	void SetNextArea(MessagingArea *area);
	MessagingArea *NextArea() const;

private:
	MessagingArea();

	messaging_area_header	*fHeader;
	area_id					fID;
	int32					fSize;
	sem_id					fLockSem;
	sem_id					fCounterSem;	// TODO: Remove, if not needed.
	MessagingArea			*fNextArea;
};

// MessagingService
class MessagingService {
public:
	MessagingService();
	~MessagingService();

	status_t Init();

private:
	static int32 _CommandProcessorEntry(void *data);
	int32 _CommandProcessor();

	sem_id			fLockSem;
	sem_id			fCounterSem;
	MessagingArea	*fFirstArea;
	thread_id		fCommandProcessor;
	volatile bool	fTerminating;
};

#endif	// MESSAGING_SERVICE_H
