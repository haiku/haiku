/******************************************************************************

	File:	BufferStreamManager.h

	Copyright 1995-97, Be Incorporated

******************************************************************************/
#ifndef _BUFFER_STREAM_MANAGER_H
#define _BUFFER_STREAM_MANAGER_H


#include <OS.h>
#include <SupportDefs.h>
#include <Locker.h>

#include "OldBufferStream.h"


/* ================
   Local Declarations
   ================ */

#define B_DEFAULT_BSTREAM_COUNT			3
#define	B_DEFAULT_BSTREAM_SIZE			B_PAGE_SIZE
#define B_DEFAULT_BSTREAM_DELAY			10000
#define	B_DEFAULT_BSTREAM_TIMEOUT		5000000


enum stream_state {
  B_IDLE = 100,		/* stream is shut down */
  B_RUNNING,		/* stream is running */
  B_STOPPING		/* waiting for final buffer to return */
  };


/* ================
   Class definition for BBufferStreamManager
   ================ */


class BBufferStreamManager {

public:

					BBufferStreamManager(char* name);
	virtual			~BBufferStreamManager();


	char			*Name() const;
	BBufferStream	*Stream() const;

	int32			BufferCount() const;
	void			SetBufferCount(int32 count);

	int32			BufferSize() const;
	void			SetBufferSize(int32 bytes);

  /* Get or set the minimum delay between sending out successive buffers.
   * Although the StreamManager automatically shuts down when there
   * are no more subscribers, setting the minimum delay can prevent
   * prevent runaway streams.  A zero or negative value means no
   * delay.
   */
	bigtime_t		BufferDelay() const;
	void			SetBufferDelay(bigtime_t usecs);

  /* If no Buffers return to the StreamManager within a period of time, the
   * StreamManager will decide that one of the subscribers is broken and
   * will go hunting for it.  When it finds the offending subscriber,
   * it will be removed from the chain with impunity.
   *
   * The default is B_DEFAULT_TIMEOUT.  Setting the timeout to 0 or a
   * negative number will disable this.
   */
	bigtime_t		Timeout() const;
	void			SetTimeout(bigtime_t usecs);

  /****************************************************************
   * Control the running of the stream.
   *
   */

  /* Set the pending state to B_RUNNING and, if required, start up
   * the processing thread.  The processing thread will start
   * emitting buffers to the stream.
   */
	stream_state	Start();

  /* Set the pending state to B_STOPPING.  The processing thread will
   * stop emitting new buffers to the stream, and when all buffers
   * are accounted for, will automatically set the desired state
   * to B_IDLE.
   */
	stream_state	Stop();

  /* Set the desired state to B_IDLE.  The processing thread will
   * stop immediately and all buffers will be "reclaimed" back
   * to the StreamManager.
   */
	stream_state	Abort();

  /* Return the current state of the stream (B_RUNNING, B_STOPPING, or B_IDLE).
   */
	stream_state	State() const;

  /* When NotificationPort is set, the receiver will get a message
   * whenever the state of the StreamManager changes.  The msg_id of the
   * message will be the new state of the StreamManager.
   */
	port_id			NotificationPort() const;
	void			SetNotificationPort(port_id port);

  /* Lock the data structures associated with this StreamManager
   */
	bool			Lock();
	void			Unlock();

  /****************************************************************
   * Subscribe functions
   */

	status_t		Subscribe(BBufferStream *stream);
	status_t		Unsubscribe();
	subscriber_id	ID() const;


/* ================
   Protected member functions.
   ================ */

protected:

  /****************************************************************
   *
   * The processing thread.  This thread waits to acquire a Buffer
   * (or for the timeout to expire) and takes appropriate action.
   */
	virtual void			StartProcessing();
	virtual void			StopProcessing();
	static status_t			_ProcessingThread(void *arg);
	virtual void			ProcessingThread();

  /* Set the state of the stream.  If newState is the same as the
   * current state, this is a no-op.  Otherwise, this method will
   * notify anyone listening on the notification port about the
   * changed state and will send a StateChange buffer through the
   * stream.
   */
	virtual void			SetState(stream_state newState);

  /* Snooze until the desired time arrives.  Returns the
   * current time upon returning.
   */
	bigtime_t				SnoozeUntil(bigtime_t sys_time);

/* ================
   Private data.
   ================ */

private:

virtual	void		_ReservedBufferStreamManager1();
virtual	void		_ReservedBufferStreamManager2();
virtual	void		_ReservedBufferStreamManager3();

  /****************************************************************
   *
   * Private fields.
   *
   */

  BBufferStream	*fStream;			/* a BBufferStream object */
  stream_state	fState;				/* running, stopping, etc. */
  sem_id		fSem;

  int32			fBufferCount;		/* desired # of buffers */
  int32			fBufferSize;		/* desired size of each buffer */
  bigtime_t		fBufferDelay;		/* minimum time between sends */
  bigtime_t		fTimeout;			/* watchdog timer */

  port_id		fNotifyPort;		/* when set, send change of state msgs */
  thread_id		fProcessingThread;	/* thread to dispatch buffers */
  subscriber_id	fManagerID;			/* StreamManager's subID in fStream */

  BLocker		fLock;
  char*			fName;
  uint32		_reserved[4];
};

#endif			// #ifdef _BUFFER_STREAM_MANAGER_H
