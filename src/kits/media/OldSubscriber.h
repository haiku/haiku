/******************************************************************************

	File:			Subscriber.h

	Description:	Support for communication with a buffer stream server

	Copyright 1995-97, Be Incorporated

******************************************************************************/

#ifndef _SUBSCRIBER_H
#define _SUBSCRIBER_H

#include <MediaDefs.h>

#include "OldBufferStream.h"

/* ================
   declarations
   ================ */

typedef bool (*enter_stream_hook)(void *userData, char *buffer, size_t count,
								  void *header);
typedef status_t (*exit_stream_hook)(void *userData, status_t error);

/* ================
   Class definition for BSubscriber
   ================ */

class BSubscriber
{
public:
						BSubscriber(const char *name = NULL);
	virtual				~BSubscriber();

/* ================
   Gaining access to the Buffer Stream
   ================ */

	virtual status_t	Subscribe(BAbstractBufferStream* stream);
	virtual status_t	Unsubscribe();

    subscriber_id		ID() const;
	const char 			*Name() const;

	void				SetTimeout(bigtime_t microseconds);
	bigtime_t			Timeout() const;

/* ================
   Streaming functions.
   ================ */

	virtual status_t	EnterStream(subscriber_id neighbor,
									bool before,
									void *userData,
									enter_stream_hook entryFunction,
									exit_stream_hook exitFunction,
									bool background);

	virtual status_t	ExitStream(bool synch=FALSE);

	bool				IsInStream() const;

/* ================
   Protected members (may be used by inherited classes)
   ================ */

protected:

	static status_t		_ProcessLoop(void *arg);
	virtual status_t	ProcessLoop();
	BAbstractBufferStream 	*Stream() const;

/* ================
   Private members
   ================ */

private:

virtual	void		_ReservedSubscriber1();
virtual	void		_ReservedSubscriber2();
virtual	void		_ReservedSubscriber3();

	char				*fName;			/* name given to constructor */
	subscriber_id		fSubID;		 	/* our subscriber_id */
	sem_id				fSem;			/* stream semaphore */
	BAbstractBufferStream	*fStream;	/* buffer stream */
	void				*fUserData;		/* arg to fStreamFn and fCompletionFn */
	enter_stream_hook	fStreamFn;	/* per-buffer user function */
	exit_stream_hook	fCompletionFn;	/* called after streaming stops */
	bool				fCallStreamFn;	/* true while we should call fStreamFn */
	bigtime_t			fTimeout;		/* time out while awaiting buffers */
	thread_id			fBackThread;
	sem_id				fSynchLock;

	int32				fFileID;		/* reserved for future use */
	uint32				_reserved[4];
};

#endif	// #ifdef _SUBSCRIBER_H
