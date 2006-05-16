/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

// This file could be moved into a generic tty module.
// The whole hardware signaling stuff is missing, though - it's currently
// tailored for pseudo-TTYs. Have a look at Be's TTY includes (drivers/tty/*)


#include <util/AutoLock.h>
#include <util/kernel_cpp.h>
#include <signal.h>
#include <string.h>

#include "SemaphorePool.h"
#include "tty_private.h"


//#define TTY_TRACE
#ifdef TTY_TRACE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


/*
	Locking
	-------

	There are four locks involved. If more than one needs to be held at a
	time, they must be acquired in the order they are listed here.

	gGlobalTTYLock: Guards open/close operations. When held, tty_open(),
	tty_close(), tty_close_cookie() etc. won't be invoked by other threads,
	cookies won't be added to/removed from TTYs, and tty::open_count,
	tty_cookie::closed won't change.

	gTTYCookieLock: Guards the access to the fields
	tty_cookie::{thread_count,closed}, or more precisely makes access to them
	atomic. thread_count is the number of threads currently using the cookie
	(i.e. read(), write(), ioctl() operations in progress). Together with
	blocking_semaphore this serves the purpose to make sure that all pending
	operations are done at a certain point when closing a cookie
	(cf. tty_close_cookie() and TTYReference).

	tty::lock: Guards the access to tty::{input_buffer,termios,window_size,
	pgrp_id}. Moreover when held guarantees that tty::open_count won't drop
	to zero (both gGlobalTTYLock and tty::lock must be held to decrement
	it). A tty and the tty connected to it (master and slave) share the same
	lock. tty::lock is only valid when tty::open_count is > 0. So before
	accessing tty::lock, it must be made sure that it is still valid. Given
	a tty_cookie, TTYReference can be used to do that, or otherwise
	gGlobalTTYLock can be acquired and tty::open_count be checked.

	gTTYRequestLock: Guards access to tty::{reader,writer}_queue (most
	RequestQueue methods do the locking themselves (the lock is a
	recursive_lock)), queued Requests and associated RequestOwners.


	Reading/Writing
	---------------

	Most of the dirty work when dealing with reading/writing is done by the
	{Reader,Writer}Locker classes. Upon construction they lock the tty,
	(tty::lock) create a RequestOwner and queue Requests in the respective
	reader/writer queues (tty::{reader,writer}_queue). The
	Acquire{Reader,Writer}() methods need to be called before being allowed to
	read/write. They ensure that there is actually something to read/space for
	writing -- in blocking mode they wait, if necessary. When destroyed the
	{Reader,Writer}Locker() remove the formerly enqueued Requests and notify
	waiting reader/writer and/or send out select events, whatever is appropiate.

	Acquire{Reader,Writer}() never return without an actual event being
	occurred. Either an error has occurred (return value) -- in this case the
	caller should terminate -- or bytes are available for reading/space for
	writing (cf. AvailableBytes()).
*/


static void tty_notify_select_event(struct tty *tty, uint8 event);
static void tty_notify_if_available(struct tty *tty, struct tty *otherTTY,
				bool notifySelect);


class AbstractLocker {
public:
	AbstractLocker(tty_cookie *cookie)	: fCookie(cookie), fBytes(0) {}

	size_t AvailableBytes() const	{ return fBytes; }

protected:
	void Lock()		{ mutex_lock(fCookie->tty->lock); }
	void Unlock()	{ mutex_unlock(fCookie->tty->lock); }

	tty_cookie	*fCookie;
	size_t		fBytes;
};


class WriterLocker : public AbstractLocker {
	public:
		WriterLocker(tty_cookie *sourceCookie);

		~WriterLocker();

		status_t AcquireWriter(bool dontBlock, int32 bytesNeeded);

	private:
		size_t _CheckAvailableBytes() const;

		struct tty		*fSource;
		struct tty		*fTarget;
		RequestOwner	fRequestOwner;
		bool			fEcho;
};


class ReaderLocker : public AbstractLocker {
	public:
		ReaderLocker(tty_cookie *cookie);
		~ReaderLocker();

		status_t AcquireReader(bool dontBlock);

	private:
		struct tty		*fTTY;
		RequestOwner	fRequestOwner;
};


class TTYReferenceLocking {
public:
	inline bool Lock(tty_cookie *cookie)
	{
		MutexLocker _(gTTYCookieLock);

		if (cookie->closed || cookie->other_tty->open_count == 0)
			return false;

		cookie->thread_count++;

		return true;
	}

	inline void Unlock(tty_cookie *cookie)
	{
		MutexLocker locker(gTTYCookieLock);

		sem_id semaphore = -1;
		if (--cookie->thread_count == 0 && cookie->closed)
			semaphore = cookie->blocking_semaphore;


		locker.Unlock();

		if (semaphore >= 0) {
			TRACE(("TTYReference: cookie %p closed, last operation done, "
				"releasing blocking sem %ld\n", cookie, semaphore));

			release_sem(semaphore);
		}
	}
};

typedef AutoLocker<tty_cookie, TTYReferenceLocking> TTYReference;


// #pragma mark -

Request::Request()
	: fOwner(NULL),
	  fCookie(NULL),
	  fBytesNeeded(0),
	  fNotified(false),
	  fError(false)
{
}


void
Request::Init(RequestOwner *owner, tty_cookie *cookie, int32 bytesNeeded)
{
	fOwner = owner;
	fCookie = cookie;
	fBytesNeeded = bytesNeeded;
	fNotified = false;
	fError = false;
}


void
Request::Notify(int32 bytesAvailable)
{
	if (!fNotified && bytesAvailable >= fBytesNeeded && fOwner) {
		fOwner->Notify(this);
		fNotified = true;
	}
}


void
Request::NotifyError(status_t error)
{
	if (!fError && fOwner) {
		fOwner->NotifyError(this, error);
		fError = true;
		fNotified = true;
	}
}


// #pragma mark -

RequestQueue::RequestQueue()
	: fRequests()
{
}


void
RequestQueue::Add(Request *request)
{
	if (request) {
		RecursiveLocker _(gTTYRequestLock);

		fRequests.Add(request, true);
	}
}


void
RequestQueue::Remove(Request *request)
{
	if (request) {
		RecursiveLocker _(gTTYRequestLock);

		fRequests.Remove(request);
	}
}


void
RequestQueue::NotifyFirst(int32 bytesAvailable)
{
	RecursiveLocker _(gTTYRequestLock);

	if (Request *first = First())
		first->Notify(bytesAvailable);
}

void
RequestQueue::NotifyError(status_t error)
{
	RecursiveLocker _(gTTYRequestLock);

	for (RequestList::Iterator it = fRequests.GetIterator(); it.HasNext();) {
		Request *request = it.Next();
		request->NotifyError(error);
	}
}

void
RequestQueue::NotifyError(tty_cookie *cookie, status_t error)
{
	RecursiveLocker _(gTTYRequestLock);

	for (RequestList::Iterator it = fRequests.GetIterator(); it.HasNext();) {
		Request *request = it.Next();
		if (request->TTYCookie() == cookie)
			request->NotifyError(error);
	}
}


// #pragma mark -

RequestOwner::RequestOwner()
	: fSemaphore(NULL),
	  fCookie(NULL),
	  fError(B_OK),
	  fBytesNeeded(1)
{
	fRequestQueues[0] = NULL;
	fRequestQueues[1] = NULL;
}


/**
 *	The caller must already hold the request lock.
 */
void
RequestOwner::Enqueue(tty_cookie *cookie, RequestQueue *queue1, 
	RequestQueue *queue2)
{
	TRACE(("%p->RequestOwner::Enqueue(%p, %p, %p)\n", this, cookie, queue1,
		queue2));

	fCookie = cookie;

	fRequestQueues[0] = queue1;
	fRequestQueues[1] = queue2;

	fRequests[0].Init(this, cookie, fBytesNeeded);
	if (queue1)
		queue1->Add(&fRequests[0]);
	else
		fRequests[0].Notify(fBytesNeeded);

	fRequests[1].Init(this, cookie, fBytesNeeded);
	if (queue2)
		queue2->Add(&fRequests[1]);
	else
		fRequests[1].Notify(fBytesNeeded);
}


/**
 *	The caller must already hold the request lock.
 */
void
RequestOwner::Dequeue()
{
	TRACE(("%p->RequestOwner::Dequeue()\n", this));

	if (fRequestQueues[0])
		fRequestQueues[0]->Remove(&fRequests[0]);
	if (fRequestQueues[1])
		fRequestQueues[1]->Remove(&fRequests[0]);
}


void
RequestOwner::SetBytesNeeded(int32 bytesNeeded)
{
	if (fRequestQueues[0])
		fRequests[0].Init(this, fCookie, bytesNeeded);

	if (fRequestQueues[1])
		fRequests[1].Init(this, fCookie, bytesNeeded);
}


/**
 *	The request lock MUST NOT be held!
 */
status_t
RequestOwner::Wait(bool interruptable, Semaphore *sem)
{
	TRACE(("%p->RequestOwner::Wait(%d, %p)\n", this, interruptable, sem));

	// get a semaphore (if not supplied or invalid)
	status_t error = B_OK;
	bool ownsSem = false;
	if (!sem || sem->InitCheck() != B_OK) {
		error = gSemaphorePool->Get(sem);
		if (error != B_OK)
			return error;

		ownsSem = true;
	}

	RecursiveLocker locker(gTTYRequestLock);

	// check, if already done
	if (fError == B_OK 
		&& (!fRequests[0].WasNotified() || !fRequests[0].WasNotified())) {
		// not yet done

		// set the semaphore
		fSemaphore = sem;

		locker.Unlock();

		// wait
		TRACE(("%p->RequestOwner::Wait(): acquiring semaphore...\n", this));

		error = acquire_sem_etc(sem->ID(), 1,
			(interruptable ? B_CAN_INTERRUPT : 0), 0);

		TRACE(("%p->RequestOwner::Wait(): semaphore acquired: %lx\n", this,
			error));

		// remove the semaphore
		locker.SetTo(gTTYRequestLock, false);
		fSemaphore = NULL;
	}

	// get the result
	if (error == B_OK)
		error = fError;

	locker.Unlock();

	// return the semaphore to the pool
	if (ownsSem)
		gSemaphorePool->Put(sem);

	return error;
}


bool
RequestOwner::IsFirstInQueues()
{
	RecursiveLocker locker(gTTYRequestLock);

	for (int i = 0; i < 2; i++) {
		if (fRequestQueues[i] && fRequestQueues[i]->First() != &fRequests[i])
			return false;
	}

	return true;
}


void
RequestOwner::Notify(Request *request)
{
	TRACE(("%p->RequestOwner::Notify(%p)\n", this, request));

	if (fError == B_OK && !request->WasNotified()) {
		bool releaseSem = false;

		if (&fRequests[0] == request) {
			releaseSem = fRequests[1].WasNotified();
		} else if (&fRequests[1] == request) {
			releaseSem = fRequests[0].WasNotified();
		} else {
			// spurious call
		}

		if (releaseSem && fSemaphore)
			release_sem(fSemaphore->ID());
	}
}


void
RequestOwner::NotifyError(Request *request, status_t error)
{
	TRACE(("%p->RequestOwner::NotifyError(%p, %lx)\n", this, request, error));

	if (fError == B_OK) {
		fError = error;

		if (!fRequests[0].WasNotified() || !fRequests[1].WasNotified()) {
			if (fSemaphore)
				release_sem(fSemaphore->ID());
		}
	}
}


// #pragma mark -

WriterLocker::WriterLocker(tty_cookie *sourceCookie)
	:
	AbstractLocker(sourceCookie),
	fSource(fCookie->tty),
	fTarget(fCookie->other_tty),
	fRequestOwner(),
	fEcho(false)
{
	Lock();

	// Now that the tty pair is locked, we can check, whether the target is
	// open at all.
	if (fTarget->open_count > 0) {
		// The target tty is open. As soon as we have appended a request to
		// the writer queue of the target, it is guaranteed to remain valid
		// until we have removed the request (and notified the
		// tty_close_cookie() pseudo request).

		// get the echo mode
		fEcho = (fTarget->termios.c_lflag & ECHO) != 0;

		// enqueue ourselves in the respective request queues
		RecursiveLocker locker(gTTYRequestLock);
		fRequestOwner.Enqueue(fCookie, &fTarget->writer_queue,
			(fEcho ? &fSource->writer_queue : NULL));
	} else {
		// target is not open: we set it to NULL; all further operations on
		// this locker will fail
		fTarget = NULL;
	}
}


WriterLocker::~WriterLocker()
{
	// dequeue from request queues
	RecursiveLocker locker(gTTYRequestLock);
	fRequestOwner.Dequeue();

	// check the tty queues and notify the next in line, and send out select
	// events
	if (fTarget)
		tty_notify_if_available(fTarget, fSource, true);
	if (fEcho)
		tty_notify_if_available(fSource, fTarget, true);

	locker.Unlock();

	Unlock();
}


size_t
WriterLocker::_CheckAvailableBytes() const
{
	size_t writable = line_buffer_writable(fTarget->input_buffer);
	if (fEcho) {
		// we can only write as much as is available on both ends
		size_t locallyWritable = line_buffer_writable(fSource->input_buffer);
		if (locallyWritable < writable)
			writable = locallyWritable;
	}
	return writable;
}


status_t 
WriterLocker::AcquireWriter(bool dontBlock, int32 bytesNeeded)
{
	if (!fTarget)
		return B_FILE_ERROR;
	if (fEcho && fCookie->closed)
		return B_FILE_ERROR;

	RecursiveLocker requestLocker(gTTYRequestLock);

	// check, if we're first in queue, and if there is space to write
	if (fRequestOwner.IsFirstInQueues()) {
		fBytes = _CheckAvailableBytes();
		if (fBytes > 0)
			return B_OK;
	}

	// We are not the first in queue or currently there's nothing to read:
	// bail out, if we shall not block.
	if (dontBlock)
		return B_WOULD_BLOCK;

	// set the number of bytes we need and notify, just in case we're first in
	// one of the queues (RequestOwner::SetBytesNeeded() resets the notification
	// state)
	if (bytesNeeded != fRequestOwner.BytesNeeded()) {
		fRequestOwner.SetBytesNeeded(bytesNeeded);
		if (fTarget)
			tty_notify_if_available(fTarget, fSource, false);
		if (fEcho)
			tty_notify_if_available(fSource, fTarget, false);
	}

	requestLocker.Unlock();

	// block until something happens
	Unlock();
	status_t status = fRequestOwner.Wait(true);
	Lock();

	// RequestOwner::Wait() returns the error, but to avoid a race condition
	// when closing a tty, we re-get the error with the tty lock being held.
	if (status == B_OK) {
		RecursiveLocker _(gTTYRequestLock);
		status = fRequestOwner.Error();
	}

	if (status == B_OK) {
		if (fTarget->open_count > 0)
			fBytes = _CheckAvailableBytes();
		else
			status = B_FILE_ERROR;
	}

	return status;
}


//	#pragma mark -


ReaderLocker::ReaderLocker(tty_cookie *cookie)
	:
	AbstractLocker(cookie),
	fTTY(cookie->tty),
	fRequestOwner()
{
	Lock();

	// enqueue ourselves in the reader request queue
	RecursiveLocker locker(gTTYRequestLock);
	fRequestOwner.Enqueue(fCookie, &fTTY->reader_queue);
}


ReaderLocker::~ReaderLocker()
{
	// dequeue from reader request queue
	RecursiveLocker locker(gTTYRequestLock);
	fRequestOwner.Dequeue();

	// check the tty queues and notify the next in line, and send out select
	// events
	struct tty *otherTTY = fCookie->other_tty;
	tty_notify_if_available(fTTY, (otherTTY->open_count > 0 ? otherTTY : NULL),
		true);

	locker.Unlock();

	Unlock();
}


status_t 
ReaderLocker::AcquireReader(bool dontBlock)
{
	if (fCookie->closed)
		return B_FILE_ERROR;

	// check, if we're first in queue, and if there is something to read
	if (fRequestOwner.IsFirstInQueues()) {
		fBytes = line_buffer_readable(fTTY->input_buffer);
		if (fBytes > 0)
			return B_OK;
	}

	// We are not the first in queue or currently there's nothing to read:
	// bail out, if we shall not block.
	if (dontBlock)
		return B_WOULD_BLOCK;

	// block until something happens
	Unlock();
	status_t status = fRequestOwner.Wait(true);
	Lock();

	if (status == B_OK)
		fBytes = line_buffer_readable(fTTY->input_buffer);

	return status;
}


//	#pragma mark -


int32
get_tty_index(const char *name)
{
	// device names follow this form: "pt/%c%x"
	int8 digit = name[4];
	if (digit >= 'a') {
		// hexadecimal digits
		digit -= 'a' - 10;
	} else
		digit -= '0';

	return (name[3] - 'p') * 16 + digit;
}


void
reset_termios(struct termios &termios)
{
	memset(&termios, 0, sizeof(struct termios));

	termios.c_iflag = 0;
	termios.c_oflag = 0;
	termios.c_cflag = B19200 | CS8 | CREAD | HUPCL;
		// enable receiver, hang up on last close
	termios.c_lflag = 0;

	// control characters	
	termios.c_cc[VINTR] = CTRL('C');
	termios.c_cc[VQUIT] = CTRL('\\');
	termios.c_cc[VERASE] = CTRL('H');
	termios.c_cc[VKILL] = CTRL('U');
	termios.c_cc[VEOF] = CTRL('D');
	termios.c_cc[VSTART] = CTRL('S');
	termios.c_cc[VSTOP] = CTRL('Q');
}


void
reset_tty(struct tty *tty, int32 index)
{
	reset_termios(tty->termios);

	tty->open_count = 0;
	tty->index = index;
	tty->lock = NULL;

	tty->pgrp_id = 0;
		// this value prevents any signal of being sent

	// some initial window size - the TTY in question should set these values
	tty->window_size.ws_col = 80;
	tty->window_size.ws_row = 25;
	tty->window_size.ws_xpixel = tty->window_size.ws_col * 8;
	tty->window_size.ws_ypixel = tty->window_size.ws_row * 8;
}


status_t
tty_output_getc(struct tty *tty, int *_c)
{
	return B_ERROR;
}


/**	Processes the input character and puts it into the TTY's input buffer.
 *	Depending on the termios flags set, signals may be sent, the input
 *	character changed or removed, etc.
 */

static void
tty_input_putc_locked(struct tty *tty, int c)
{
	tcflag_t flags = tty->termios.c_iflag;

	// process signals if needed

	if ((tty->termios.c_lflag & ISIG) != 0) {
		// enable signals, process INTR, QUIT, and SUSP
		int signal = -1;

		if (c == tty->termios.c_cc[VINTR])
			signal = SIGINT;
		else if (c == tty->termios.c_cc[VQUIT])
			signal = SIGQUIT;
		else if (c == tty->termios.c_cc[VSUSP])
			// ToDo: what to do here?
			signal = -1;

		// do we need to deliver a signal?
		if (signal != -1) {
			// we may have to flush the input buffer
			if ((tty->termios.c_lflag & NOFLSH) == 0)
				clear_line_buffer(tty->input_buffer);

			if (tty->pgrp_id != 0)
				send_signal(-tty->pgrp_id, signal);
			return;
		}
	}

	// process special canonical input characters

	if ((tty->termios.c_lflag & ICANON) != 0) {
		// canonical mode, process ERASE and KILL
		if (c == tty->termios.c_cc[VERASE]) {
			// ToDo: erase one character
			return;
		} else if (c == tty->termios.c_cc[VKILL]) {
			// ToDo: erase line
			return;
		}
	}

	// character conversions

	if (c == '\n' && (flags & INLCR) != 0) {
		// map NL to CR
		c = '\r';
	} else if (c == '\r') {
		if (flags & IGNCR)	// ignore CR
			return;
		if (flags & ICRNL)	// map CR to NL
			c = '\n';
	} if (flags & ISTRIP)	// strip the highest bit
		c &= 0x7f;

	line_buffer_putc(tty->input_buffer, c);
}


#if 0
status_t
tty_input_putc(struct tty *tty, int c)
{
	status_t status = acquire_sem_etc(tty->write_sem, 1, B_CAN_INTERRUPT, 0);
	if (status != B_OK)
		return status;

	MutexLocker locker(&tty->lock);

	bool wasEmpty = line_buffer_readable(tty->input_buffer) == 0;

	tty_input_putc_locked(tty, c);

	// If the buffer was empty before, we can now start other readers on it.
	// We assume that most of the time more than one character will be written
	// using this function, so we don't want to reschedule after every character
	if (wasEmpty)
		release_sem_etc(tty->read_sem, 1, B_DO_NOT_RESCHEDULE);

	// We only wrote one char - we give others the opportunity
	// to write if there is still space left in the buffer
	if (line_buffer_writable(tty->input_buffer))
		release_sem_etc(tty->write_sem, 1, B_DO_NOT_RESCHEDULE);

	return B_OK;
}
#endif // 0


/**
 * The global lock must be held.
 */
status_t
init_tty_cookie(tty_cookie *cookie, struct tty *tty, struct tty *otherTTY,
	uint32 openMode)
{
	cookie->blocking_semaphore = create_sem(0, "wait for tty close");
	if (cookie->blocking_semaphore < 0)
		return cookie->blocking_semaphore;

	cookie->tty = tty;
	cookie->other_tty = otherTTY;
	cookie->open_mode = openMode;
	cookie->select_pool = NULL;
	cookie->thread_count = 0;
	cookie->closed = false;

	return B_OK;
}


void
uninit_tty_cookie(tty_cookie *cookie)
{
	if (cookie->blocking_semaphore >= 0) {
		delete_sem(cookie->blocking_semaphore);
		cookie->blocking_semaphore = -1;
	}

	cookie->tty = NULL;
	cookie->thread_count = 0;
	cookie->closed = false;
}


/**
 * The global lock must be held.
 */
void
add_tty_cookie(tty_cookie *cookie)
{
	MutexLocker locker(cookie->tty->lock);

	// add to the TTY's cookie list
	cookie->tty->cookies.Add(cookie);
	cookie->tty->open_count++;
}


/**
 * The global lock must be held.
 */
void
tty_close_cookie(struct tty_cookie *cookie)
{
	MutexLocker locker(gTTYCookieLock);

	// Already closed? This can happen for slaves that have been closed when
	// the master was closed.
	if (cookie->closed)
		return;

	// set the cookie's `closed' flag
	cookie->closed = true;
	bool unblock = (cookie->thread_count > 0);

	// unblock blocking threads
	if (unblock) {
		cookie->tty->reader_queue.NotifyError(cookie, B_FILE_ERROR);
		cookie->tty->writer_queue.NotifyError(cookie, B_FILE_ERROR);

		if (cookie->other_tty->open_count > 0) {
			cookie->other_tty->reader_queue.NotifyError(cookie, B_FILE_ERROR);
			cookie->other_tty->writer_queue.NotifyError(cookie, B_FILE_ERROR);
		}
	}

	locker.Unlock();

	// wait till all blocking (and now unblocked) threads have left the
	// critical code
	if (unblock) {
		TRACE(("tty_close_cookie(): cookie %p, there're still pending "
			"operations, acquire blocking sem %ld\n", cookie,
			cookie->blocking_semaphore));

		acquire_sem(cookie->blocking_semaphore);
	}

	// For the removal of the cookie acquire the TTY's lock. This ensures, that
	// cookies will not be removed from a TTY (or added -- cf. add_tty_cookie())
	// as long as the TTY's lock is being held. This is required for the select
	// support, since we need to iterate through the cookies of a TTY without
	// having to acquire the global lock.
	MutexLocker ttyLocker(cookie->tty->lock);

	// remove the cookie from the TTY's cookie list
	cookie->tty->cookies.Remove(cookie);

	// close the tty, if no longer used
	if (--cookie->tty->open_count == 0) {
		// The last cookie of this tty has been closed. We're going to close
		// the TTY and need to unblock all write requests before. There should
		// be no read requests, since only a cookie of this TTY issues those.
		// We do this by first notifying all queued requests of the error
		// condition. We then clear the line buffer for the TTY and queue
		// an own request.

		// Notify the other TTY first; it doesn't accept any read/writes
		// while there is only one end.
		cookie->other_tty->reader_queue.NotifyError(B_FILE_ERROR);
		cookie->other_tty->writer_queue.NotifyError(B_FILE_ERROR);

		RecursiveLocker requestLocker(gTTYRequestLock);

		// we only need to do all this, if the writer queue is not empty
		if (!cookie->tty->writer_queue.IsEmpty()) {
	 		// notify the blocking writers
			cookie->tty->writer_queue.NotifyError(B_FILE_ERROR);

			// enqueue our request
			RequestOwner requestOwner;
			requestOwner.Enqueue(cookie, &cookie->tty->writer_queue);

			requestLocker.Unlock();

			// clear the line buffer
			clear_line_buffer(cookie->tty->input_buffer);

			ttyLocker.Unlock();

			// wait for our turn (we reuse the blocking semaphore, so Wait()
			// can't fail due to not being able to get a semaphore)
			Semaphore sem(cookie->blocking_semaphore);
			cookie->blocking_semaphore = -1;
			requestOwner.Wait(false, &sem);

			// re-lock
			ttyLocker.SetTo(cookie->tty->lock, false);
			requestLocker.SetTo(gTTYRequestLock, false);

			// dequeue our request
			requestOwner.Dequeue();
		}

		requestLocker.Unlock();

		// finally close the tty
		tty_close(cookie->tty);
	}

	// notify all pending selects and clean up the pool
	if (cookie->select_pool) {
		notify_select_event_pool(cookie->select_pool, B_SELECT_READ);
		notify_select_event_pool(cookie->select_pool, B_SELECT_WRITE);
		notify_select_event_pool(cookie->select_pool, B_SELECT_ERROR);

		delete_select_sync_pool(cookie->select_pool);
		cookie->select_pool = NULL;
	}

	// notify a select write event on the other tty, if we've closed this tty
	if (cookie->tty->open_count == 0 && cookie->other_tty->open_count > 0)
		tty_notify_select_event(cookie->other_tty, B_SELECT_WRITE);
}


static void
tty_notify_select_event(struct tty *tty, uint8 event)
{
	TRACE(("tty_notify_select_event(%p, %u)\n", tty, event));

	for (TTYCookieList::Iterator it = tty->cookies.GetIterator();
		 it.HasNext();) {
		tty_cookie *cookie = it.Next();

		if (cookie->select_pool)
			notify_select_event_pool(cookie->select_pool, event);
	}
}


/** \brief Checks whether bytes can be read from/written to the line buffer of
 *		   the given TTY and notifies the respective queues.
 *
 *	Also sends out \c B_SELECT_READ and \c B_SELECT_WRITE events as needed.
 *
 *	The TTY and the request lock must be held.
 *
 * \param tty The TTY.
 * \param otherTTY The connected TTY.
 */
static void
tty_notify_if_available(struct tty *tty, struct tty *otherTTY,
	bool notifySelect)
{
	if (!tty)
		return;

	int32 readable = line_buffer_readable(tty->input_buffer);
	if (readable > 0) {
		// if nobody is waiting send select events, otherwise notify the waiter
		if (!tty->reader_queue.IsEmpty())
			tty->reader_queue.NotifyFirst(readable);
		else if (notifySelect)
			tty_notify_select_event(tty, B_SELECT_READ);
	}

	int32 writable = line_buffer_writable(tty->input_buffer);
	if (writable > 0) {
		// if nobody is waiting send select events, otherwise notify the waiter
		if (!tty->writer_queue.IsEmpty()) {
			tty->writer_queue.NotifyFirst(writable);
		} else if (notifySelect) {
			if (otherTTY && otherTTY->open_count > 0) 
				tty_notify_select_event(otherTTY, B_SELECT_WRITE);
		}
	}
}


//	#pragma mark -
//	device functions


status_t
tty_close(struct tty *tty)
{
	// destroy the queues
	tty->reader_queue.~RequestQueue();
	tty->writer_queue.~RequestQueue();
	tty->cookies.~TTYCookieList();

	uninit_line_buffer(tty->input_buffer);

	return B_OK;
}


status_t
tty_open(struct tty *tty, tty_service_func func)
{
	if (init_line_buffer(tty->input_buffer, TTY_BUFFER_SIZE) < B_OK)
		return B_NO_MEMORY;

	tty->lock = NULL;
	tty->service_func = func;

	// construct the queues
	new(&tty->reader_queue) RequestQueue;
	new(&tty->writer_queue) RequestQueue;
	new(&tty->cookies) TTYCookieList;

	return B_OK;
}


status_t
tty_ioctl(tty_cookie *cookie, uint32 op, void *buffer, size_t length)
{
	struct tty *tty = cookie->tty;

	// bail out, if already closed
	TTYReference ttyReference(cookie);
	if (!ttyReference.IsLocked())
		return B_FILE_ERROR;

	TRACE(("tty_ioctl: tty %p, op %lu, buffer %p, length %lu\n", tty, op, buffer, length));
	MutexLocker locker(tty->lock);

	switch (op) {
		/* blocking/non-blocking mode */

		case B_SET_BLOCKING_IO:
			cookie->open_mode &= ~O_NONBLOCK;
			return B_OK;
		case B_SET_NONBLOCKING_IO:
			cookie->open_mode |= O_NONBLOCK;
			return B_OK;

		/* get and set TTY attributes */

		case TCGETA:
			TRACE(("tty: get attributes\n"));
			return user_memcpy(buffer, &tty->termios, sizeof(struct termios));

		case TCSETA:
		case TCSETAW:
		case TCSETAF:
			TRACE(("tty: set attributes (iflag = %lx, oflag = %lx, cflag = %lx, lflag = %lx)\n",
				tty->termios.c_iflag, tty->termios.c_oflag, tty->termios.c_cflag, tty->termios.c_lflag));

			return user_memcpy(&tty->termios, buffer, sizeof(struct termios));

		/* get and set process group ID */

		case TIOCGPGRP:
			TRACE(("tty: get pgrp_id\n"));
			return user_memcpy(buffer, &tty->pgrp_id, sizeof(pid_t));
		case TIOCSPGRP:
		case 'pgid':
			TRACE(("tty: set pgrp_id\n"));
			return user_memcpy(&tty->pgrp_id, buffer, sizeof(pid_t));

		/* get and set window size */

		case TIOCGWINSZ:
			TRACE(("tty: set window size\n"));
			return user_memcpy(buffer, &tty->window_size, sizeof(struct winsize));

		case TIOCSWINSZ:
		{
			uint16 oldColumns = tty->window_size.ws_col, oldRows = tty->window_size.ws_row;

			TRACE(("tty: set window size\n"));
			if (user_memcpy(&tty->window_size, buffer, sizeof(struct winsize)) < B_OK)
				return B_BAD_ADDRESS;

			// send a signal only if the window size has changed
			if ((oldColumns != tty->window_size.ws_col || oldRows != tty->window_size.ws_row)
				&& tty->pgrp_id != 0)
				send_signal(-tty->pgrp_id, SIGWINCH);

			return B_OK;
		}
	}

	TRACE(("tty: unsupported opcode %lu\n", op));
	return B_BAD_VALUE;
}


status_t
tty_input_read(tty_cookie *cookie, void *buffer, size_t *_length)
{
	struct tty *tty = cookie->tty;
	uint32 mode = cookie->open_mode;
	bool dontBlock = (mode & O_NONBLOCK) != 0;
	size_t length = *_length;
	ssize_t bytesRead = 0;

	TRACE(("tty_input_read(tty = %p, length = %lu, mode = %lu)\n", tty, length, mode));

	if (length == 0)
		return B_OK;

	// bail out, if the TTY is already closed
	TTYReference ttyReference(cookie);
	if (!ttyReference.IsLocked())
		return B_FILE_ERROR;

	ReaderLocker locker(cookie);

	while (bytesRead == 0) {
		status_t status = locker.AcquireReader(dontBlock);
		if (status != B_OK) {
			*_length = 0;
			return status;
		}

		// ToDo: add support for ICANON mode

		bytesRead = line_buffer_user_read(tty->input_buffer, (char *)buffer,
			length);
		if (bytesRead < B_OK) {
			*_length = 0;
			return bytesRead;
		}
	}

	*_length = bytesRead;
	return B_OK;
}


status_t
tty_write_to_tty(tty_cookie *sourceCookie, const void *buffer, size_t *_length,
	bool sourceIsMaster)
{
	struct tty *source = sourceCookie->tty;
	struct tty *target = sourceCookie->other_tty;
	const char *data = (const char *)buffer;
	size_t length = *_length;
	size_t bytesWritten = 0;
	uint32 mode = sourceCookie->open_mode;
	bool dontBlock = (mode & O_NONBLOCK) != 0;

	// bail out, if source is already closed
	TTYReference sourceTTYReference(sourceCookie);
	if (!sourceTTYReference.IsLocked())
		return B_FILE_ERROR;

	if (length == 0)
		return B_OK;

	WriterLocker locker(sourceCookie);

	// if the target is not open, fail now
	if (target->open_count <= 0)
		return B_FILE_ERROR;

	bool echo = (target->termios.c_lflag & ECHO) != 0;
		// Confusingly enough, we need to echo when the target's ECHO flag is
		// set. That's because our target is supposed to echo back at us, not
		// to itself.

	TRACE(("tty_write_to_tty(source = %p, target = %p, length = %lu, %s%s)\n", source, target, length,
		sourceIsMaster ? "master" : "slave", echo ? ", echo mode" : ""));

	// ToDo: "buffer" is not yet copied or accessed in a safe way!

	int32 bytesNeeded = 1;

	while (bytesWritten < length) {
		status_t status = locker.AcquireWriter(dontBlock, bytesNeeded);
		if (status != B_OK) {
			*_length = bytesWritten;
			return status;
		}

		bytesNeeded = 1;	// reset

		size_t writable = locker.AvailableBytes();
		if (writable == 0) {
			// should never happen
			continue;
		}

		while (writable > 0 && bytesWritten < length) {
			char c = data[0];

			if (c == '\n' && (source->termios.c_oflag & (OPOST | ONLCR)) == OPOST | ONLCR) {
				// post-process output and transfrom '\n' to '\r\n'

				// If we can't write both '\r' and '\n', we won't write either,
				// or we would write the '\r' again.
				if (writable < 2) {
					// try acquiring the writer again, when enough space is available
					bytesNeeded = 2;
					writable = 0;
						// make sure we're breaking out of this loop
					continue;
				}

#if 1
	// ToDo: this doesn't fix the bug that is responsible for the doubled shell prompt
	//		and it's all wrong, too - but it cures the symptoms, and that's good enough
	//		for now :)
	//		Apparently, the shell reads only single bytes and handles both, '\r' and '\n'
	//		as a newline.
				if (!sourceIsMaster)
#endif
				tty_input_putc_locked(target, '\r');
			 	if (echo)
					tty_input_putc_locked(source, '\r');

				if (--writable == 0)
					continue;
			}

			tty_input_putc_locked(target, c);
			if (echo)
				tty_input_putc_locked(source, c);

			data++;
			bytesWritten++;
		}
	}

	return B_OK;
}


status_t
tty_select(tty_cookie *cookie, uint8 event, uint32 ref, selectsync *sync)
{
	struct tty *tty = cookie->tty;

	TRACE(("tty_select(cookie = %p, event = %u, ref = %lu, sync = %p)\n",
		cookie, event, ref, sync));

	// we don't support all kinds of events
	if (event < B_SELECT_READ || event > B_SELECT_ERROR)
		return B_BAD_VALUE;

	// if the TTY is already closed, we notify immediately
	TTYReference ttyReference(cookie);
	if (!ttyReference.IsLocked()) {
		TRACE(("tty_select() done: cookie %p already closed\n", cookie));

		notify_select_event(sync, ref, event);
		return B_OK;
	}

	// lock the TTY (allows us to freely access the cookie lists of this and
	// the other TTY)
	MutexLocker ttyLocker(tty->lock);

	// get the other TTY -- needed for `write' events
	struct tty *otherTTY = cookie->other_tty;
	if (otherTTY->open_count <= 0)
		otherTTY = NULL;

	// add the event to the TTY's pool
	status_t error = add_select_sync_pool_entry(&cookie->select_pool, sync, ref,
		event);
	if (error != B_OK) {
		TRACE(("tty_select() done: add_select_sync_pool_entry() failed: %lx\n",
			error));

		return error;
	}

	// finally also acquire the request mutex, for access to the reader/writer
	// queues
	RecursiveLocker requestLocker(gTTYRequestLock);

	// check, if the event is already present
	switch (event) {
		case B_SELECT_READ:
			if (tty->reader_queue.IsEmpty()
				&& line_buffer_readable(tty->input_buffer) > 0) {
				notify_select_event(sync, ref, event);
			}
			break;

		case B_SELECT_WRITE:
		{
			// writes go to the other TTY
			if (!otherTTY) {
				notify_select_event(sync, ref, event);
				break;
			}

			// In case the other TTY echos, we have to check, whether we can
			// currently can write to our TTY as well.
			bool echo = (otherTTY->termios.c_lflag & ECHO);

			if (otherTTY->writer_queue.IsEmpty()
				&& line_buffer_writable(otherTTY->input_buffer) > 0) {
				if (!echo
					|| (tty->writer_queue.IsEmpty()
						&& line_buffer_writable(tty->input_buffer) > 0)) {
					notify_select_event(sync, ref, event);
				}
			}

			break;
		}

		case B_SELECT_ERROR:
		default:
			break;
	}

	return B_OK;
}


status_t
tty_deselect(tty_cookie *cookie, uint8 event, selectsync *sync)
{
	struct tty *tty = cookie->tty;

	TRACE(("tty_deselect(cookie = %p, event = %u, sync = %p)\n", cookie, event,
		sync));

	// we don't support all kinds of events
	if (event < B_SELECT_READ || event > B_SELECT_ERROR)
		return B_BAD_VALUE;

	// If the TTY is already closed, we're done. Note that we don't use a
	// TTYReference here, but acquire the global lock, since we don't want
	// return before tty_close_cookie() is done (it sends out the select
	// events on close and our select() could miss one, if we don't wait).
	MutexLocker globalLocker(gGlobalTTYLock);
	if (cookie->closed)
		return B_OK;

	// lock the TTY (guards the select sync pool, among other things)
	MutexLocker ttyLocker(tty->lock);

	return remove_select_sync_pool_entry(&cookie->select_pool, sync, event);
}

