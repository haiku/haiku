//
//	$Id: MessageQueue.h 1686 2002-10-26 18:59:16Z beveloper $
//
//	This is the BMessageQueue interface for OpenBeOS.  It has been created
//  to be source and binary compatible with the BeOS version of
//  BMessageQueue.
//


#ifndef	_OPENBEOS_MESSAGEQUEUE_H
#define	_OPENBEOS_MESSAGEQUEUE_H


#include <Locker.h>
#include <Message.h>	/* For convenience */


#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif

class BMessageQueue {
public:
	BMessageQueue();
	virtual ~BMessageQueue();
	
	void AddMessage(BMessage *message);
	void RemoveMessage(BMessage *message);
	
	int32 CountMessages(void) const;
	bool IsEmpty(void) const;
	
	BMessage *FindMessage(int32 index) const;
	BMessage *FindMessage(uint32 what, int32 index=0) const;
	
	bool Lock(void);
	void Unlock(void);
	bool IsLocked(void);

	BMessage *NextMessage(void);

private:

	// Reserved space in the vtable for future changes to BMessageQueue
	virtual void _ReservedMessageQueue1(void);
	virtual void _ReservedMessageQueue2(void);
	virtual void _ReservedMessageQueue3(void);
	
	BMessageQueue(const BMessageQueue &);
	BMessageQueue &operator=(const BMessageQueue &);
	
	BMessage *fTheQueue;
	BMessage *fQueueTail;
	int32 fMessageCount;
	BLocker fLocker;
	
	// Reserved space for future changes to BMessageQueue
	uint32 fReservedSpace[3];
};

#ifdef USE_OPENBEOS_NAMESPACE
}
#endif

#endif // _OPENBEOS_MESSAGEQUEUE_H
