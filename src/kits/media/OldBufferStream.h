/******************************************************************************

	File:	BufferStream.h

	Copyright 1995-97, Be Incorporated

******************************************************************************/
#ifndef _BUFFER_STREAM_H
#define _BUFFER_STREAM_H

#include <stdlib.h>
#include <OS.h>
#include <SupportDefs.h>
#include <Locker.h>
#include <Messenger.h>


class BSubscriber;


/* ================
   Per-subscriber information.
   ================ */

struct _sbuf_info;

typedef struct _sub_info {
  _sub_info			*fNext;		/* next subscriber in the stream*/
  _sub_info			*fPrev;		/* previous subscriber in the stream */
  _sbuf_info		*fRel;		/* next buf to be released */
  _sbuf_info		*fAcq;		/* next buf to be acquired */
  sem_id			fSem;		/* semaphore used for blocking */
  bigtime_t			fTotalTime;	/* accumulated time between acq/rel */
  int32				fHeld;		/* # of buffers acq'd but not yet rel'd */
  sem_id			fBlockedOn;	/* the semaphore being waited on */
								/* or B_BAD_SEM_ID if not blocked */
} *subscriber_id;


/* ================
   Per-buffer information
   ================ */

typedef struct _sbuf_info {
  _sbuf_info		*fNext;		/* next "newer" buffer in the chain */
  subscriber_id		fAvailTo;	/* next subscriber to acquire this buffer */
  subscriber_id		fHeldBy;	/* subscriber that's acquired this buffer */
  bigtime_t			fAcqTime;	/* time at which this buffer was acquired */
  area_id			fAreaID;	/* for system memory allocation calls */
  char				*fAddress;
  int32				fSize;     /* usable portion can be smaller than ... */
  int32				fAreaSize; /* ... the size of the area. */
  bool				fIsFinal;	/* TRUE => stream is stopping */
} *buffer_id;


/* ================
   Interface definition for BBufferStream class
   ================ */

/* We've chosen B_MAX_SUBSCRIBER_COUNT and B_MAX_BUFFER_COUNT to be small
 * enough so that a BBufferStream structure fits in one 4096 byte page.
 */
#define	B_MAX_SUBSCRIBER_COUNT	52
#define	B_MAX_BUFFER_COUNT		32

class BBufferStream;
class BBufferStreamManager;

typedef BBufferStream* stream_id;  // for now


class BAbstractBufferStream
{
public:
#if __GNUC__ > 3
	virtual				~BAbstractBufferStream();
#endif

	virtual	status_t	GetStreamParameters(size_t *bufferSize,
											int32 *bufferCount,
											bool *isRunning,
											int32 *subscriberCount) const;

	virtual	status_t	SetStreamBuffers(size_t bufferSize, 
										 int32 bufferCount);

	virtual	status_t	StartStreaming();
	virtual	status_t	StopStreaming();

protected:

virtual	void		_ReservedAbstractBufferStream1();
virtual	void		_ReservedAbstractBufferStream2();
virtual	void		_ReservedAbstractBufferStream3();
virtual	void		_ReservedAbstractBufferStream4();

	friend class BSubscriber;
	friend class BBufferStreamManager;

	virtual stream_id	StreamID() const; 
	/* stream identifier for direct access */

	/* Create or delete a subscriber id for subsequent operations */
	virtual	status_t	Subscribe(char *name,
								  subscriber_id *subID,
								  sem_id semID);
	virtual	status_t	Unsubscribe(subscriber_id subID);

/* Enter into or quit the stream */
	virtual	status_t	EnterStream(subscriber_id subID, 
									subscriber_id neighbor,
									bool before);

	virtual	status_t	ExitStream(subscriber_id subID);

	virtual BMessenger*	Server() const;	/* message pipe to server */
	status_t 		SendRPC(BMessage* msg, BMessage* reply = NULL) const;
};


class BBufferStream : public BAbstractBufferStream
{
public:

						BBufferStream(size_t headerSize,
									  BBufferStreamManager* controller,
									  BSubscriber* headFeeder,
									  BSubscriber* tailFeeder);
		virtual			~BBufferStream();
				
/* BBufferStreams are allocated on shared memory pages */
		void			*operator new(size_t size);
		void			operator delete(void *stream, size_t size);

/* Return header size */
		size_t			HeaderSize() const;

/* These four functions are delegated to the stream controller */
		status_t		GetStreamParameters(size_t *bufferSize,
											int32 *bufferCount,
											bool *isRunning,
											int32 *subscriberCount) const;

		status_t		SetStreamBuffers(size_t bufferSize, 
										 int32 bufferCount);

		status_t		StartStreaming();
		status_t		StopStreaming();

/* Get the controller for delegation */
		BBufferStreamManager *StreamManager() const;

/* number of buffers in stream */
		int32			CountBuffers() const;

/* Create or delete a subscriber id for subsequent operations */
		status_t		Subscribe(char *name, 
								  subscriber_id *subID,
								  sem_id semID);

		status_t		Unsubscribe(subscriber_id subID);

/* Enter into or quit the stream */
		status_t		EnterStream(subscriber_id subID, 
									subscriber_id neighbor,
									bool before);

		status_t		ExitStream(subscriber_id subID);

/* queries about a subscriber */
		bool			IsSubscribed(subscriber_id subID);
		bool			IsEntered(subscriber_id subID);

		status_t		SubscriberInfo(subscriber_id subID,
									   char** name,
									   stream_id* streamID,
									   int32* position);

/* Force an error return of a subscriber if it's blocked */
		status_t		UnblockSubscriber(subscriber_id subID);

/* Acquire and release a buffer */
		status_t		AcquireBuffer(subscriber_id subID, 
									  buffer_id *bufID,
									  bigtime_t timeout);
		status_t		ReleaseBuffer(subscriber_id subID);

/* Get the attributes of a particular buffer */
		size_t			BufferSize(buffer_id bufID) const;
		char			*BufferData(buffer_id bufID) const;
		bool			IsFinalBuffer(buffer_id bufID) const;

/* Get attributes of a particular subscriber */
		int32			CountBuffersHeld(subscriber_id subID);

/* Queries for the BBufferStream */
		int32			CountSubscribers() const;
		int32			CountEnteredSubscribers() const;

		subscriber_id	FirstSubscriber() const;
		subscriber_id	LastSubscriber() const;
		subscriber_id	NextSubscriber(subscriber_id subID);
		subscriber_id	PrevSubscriber(subscriber_id subID);

/* debugging aids */
		void			PrintStream();
		void			PrintBuffers();
		void			PrintSubscribers();

/* gaining exclusive access to the BBufferStream */
		bool 			Lock();
		void			Unlock();

/* introduce a new buffer into the "newest" end of the chain */
		status_t		AddBuffer(buffer_id bufID);

/* remove a buffer from the "oldest" end of the chain */
		buffer_id		RemoveBuffer(bool force);

/* allocate a buffer from shared memory and create a bufID for it. */
		buffer_id		CreateBuffer(size_t size, bool isFinal);

/* deallocate a buffer and returns its bufID to the freelist */
		void			DestroyBuffer(buffer_id bufID);

/* remove and destroy any "newest" buffers from the head of the chain
 * that have not yet been claimed by any subscribers.  If there are
 * no subscribers, this clears the entire chain.
 */
		void			RescindBuffers();

/* ================
   Private member functions that assume locking already has been done.
   ================ */

private:

virtual	void		_ReservedBufferStream1();
virtual	void		_ReservedBufferStream2();
virtual	void		_ReservedBufferStream3();
virtual	void		_ReservedBufferStream4();

/* initialize the free list of subscribers */
		void			InitSubscribers();

/* return TRUE if subID appears valid */
		bool			IsSubscribedSafe(subscriber_id subID) const;

/* return TRUE if subID is entered into the stream */
		bool			IsEnteredSafe(subscriber_id subID) const;

/* initialize the free list of buffer IDs */
		void			InitBuffers();

/* Wake a blocked subscriber */
		status_t		WakeSubscriber(subscriber_id subID);

/* Give subID all the buffers it can get */
		void			InheritBuffers(subscriber_id subID);

/* Relinquish any buffers held by subID */
		void			BequeathBuffers(subscriber_id subID);

/* Fast version of ReleaseBuffer() */
		status_t		ReleaseBufferSafe(subscriber_id subID);

/* Release a buffer to a subscriber */
		status_t		ReleaseBufferTo(buffer_id bufID, subscriber_id subID);

/* deallocate all buffers */
		void			FreeAllBuffers();

/* deallocate all subscribers */
		void			FreeAllSubscribers();

/* ================
   Private data members
   ================ */

		BLocker				fLock;
		area_id				fAreaID;		/* area id for this BBufferStream */
		BBufferStreamManager	*fStreamManager;
		BSubscriber			*fHeadFeeder;
		BSubscriber			*fTailFeeder;
		size_t				fHeaderSize;

		/* ================
		   subscribers
		   ================ */

		_sub_info			*fFreeSubs;		/* free list of subscribers */
		_sub_info			*fFirstSub;		/* first entered in itinierary */
		_sub_info			*fLastSub;		/* last entered in itinerary */

		sem_id				fFirstSem;		/* semaphore used by fFirstSub */
		int32				fSubCount;
		int32				fEnteredSubCount;

		_sub_info			fSubscribers[B_MAX_SUBSCRIBER_COUNT];

		/* ================
		   buffers
		   ================ */

		_sbuf_info			*fFreeBuffers;
		_sbuf_info			*fOldestBuffer;	/* first in line */
		_sbuf_info			*fNewestBuffer;	/* fNewest->fNext = NULL */
		int32				fCountBuffers;

		_sbuf_info			fBuffers[B_MAX_BUFFER_COUNT];

		uint32				_reserved[4];
};

#endif 	// #ifdef _BUFFER_STREAM_H
