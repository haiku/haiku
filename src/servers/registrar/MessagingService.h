/* 
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef MESSAGING_SERVICE_H
#define MESSAGING_SERVICE_H

#include <Locker.h>
#include <MessagingServiceDefs.h>

// MessagingCommandHandler
class MessagingCommandHandler {
public:
	MessagingCommandHandler();
	virtual ~MessagingCommandHandler();

	virtual void HandleMessagingCommand(uint32 command, const void *data,
		int32 dataSize) = 0;
};

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
private:
	MessagingService();
	~MessagingService();

	status_t Init();

public:
	static status_t CreateDefault();
	static void DeleteDefault();
	static MessagingService *Default();

	void SetCommandHandler(uint32 command, MessagingCommandHandler *handler);

private:
	MessagingCommandHandler *_GetCommandHandler(uint32 command) const;

	static int32 _CommandProcessorEntry(void *data);
	int32 _CommandProcessor();

	class DefaultSendCommandHandler;
	struct CommandHandlerMap;

	static MessagingService	*sService;

	mutable BLocker			fLock;
	sem_id					fLockSem;
	sem_id					fCounterSem;
	MessagingArea			*fFirstArea;
	CommandHandlerMap		*fCommandHandlers;
	thread_id				fCommandProcessor;
	volatile bool			fTerminating;
};

#endif	// MESSAGING_SERVICE_H
