/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

// This file could be moved into a generic tty module.
// The whole hardware signaling stuff is missing, though - it's currently
// tailored for pseudo-TTYs. Have a look at Be's TTY includes (drivers/tty/*)


#include "tty_private.h"

#include <util/kernel_cpp.h>
#include <signal.h>
#include <string.h>


#define TTY_TRACE
#ifdef TTY_TRACE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


class WriterLocker {
	public:
		WriterLocker(struct tty *source, struct tty *target, bool echo, bool sourceIsMaster);
		~WriterLocker();

		status_t AcquireWriter(bool dontBlock);
		void ReportWritten(size_t written);

	private:
		void Lock();
		void Unlock();

		struct tty	*fSource, *fTarget;
		size_t		fSourceBytes, fTargetBytes;
		bool		fEcho;
};

class ReaderLocker {
	public:
		ReaderLocker(struct tty *tty);
		~ReaderLocker();

		status_t AcquireReader(bool dontBlock);
		void ReportRead(size_t bytesRead);

	private:
		void Lock();
		void Unlock();

		struct tty	*fTTY;
		size_t		fBytes;
};

class MutexLocker {
	public:
		MutexLocker(struct mutex *mutex)
			: fMutex(mutex)
		{
			mutex_lock(mutex);
		}

		~MutexLocker()
		{
			mutex_unlock(fMutex);
		}

	private:
		struct mutex	*fMutex;
};


WriterLocker::WriterLocker(struct tty *source, struct tty *target, bool echo, bool sourceIsMaster)
	:
	fSource(source),
	fTarget(target),
	fEcho(echo)
{
	if (echo && sourceIsMaster) {
		// just switch the two, we have to lock both of
		// them anyway - master is always locked first
		fSource = target;
		fTarget = source;
	}

	Lock();
}


WriterLocker::~WriterLocker()
{
	Unlock();
}


void
WriterLocker::Lock()
{
	mutex_lock(&fTarget->lock);
	if (fEcho)
		mutex_lock(&fSource->lock);
}


void
WriterLocker::Unlock()
{
	if (fEcho)
		mutex_unlock(&fSource->lock);

	mutex_unlock(&fTarget->lock);
}


status_t 
WriterLocker::AcquireWriter(bool dontBlock)
{
	// Release TTY lock in order not to block. We only need to do this
	// if we could have to wait for the buffer to become writable.
	if (!dontBlock)
		Unlock();

	status_t status = acquire_sem_etc(fTarget->write_sem, 1, (dontBlock ? B_TIMEOUT : 0) | B_CAN_INTERRUPT, 0);
	if (status == B_OK && fEcho) {
		// We need to hold two write semaphores in order to echo the output to
		// the local TTY as well. We need to make sure that these semaphores
		// are always acquired in the same order to prevent deadlocks
		status = acquire_sem_etc(fSource->write_sem, 1, (dontBlock ? B_TIMEOUT : 0) | B_CAN_INTERRUPT, 0);
		if (status != B_OK)
			release_sem(fTarget->write_sem);
	}

	// reacquire TTY lock
	if (!dontBlock)
		Lock();

	if (status == B_OK) {
		fTargetBytes = line_buffer_writable(fTarget->input_buffer);
		if (fEcho)
			fSourceBytes = line_buffer_writable(fSource->input_buffer);
	}
	return status;
}


void 
WriterLocker::ReportWritten(size_t written)
{
	if (written < fTargetBytes)
		release_sem_etc(fTarget->write_sem, 1, fEcho ? B_DO_NOT_RESCHEDULE : 0);

	if (fEcho && written < fSourceBytes)
		release_sem(fSource->write_sem);

	// there is now probably something to read, too

	if (written > 0) {
		release_sem(fTarget->read_sem);
		if (fEcho)
			release_sem(fSource->read_sem);
	}
}


//	#pragma mark -


ReaderLocker::ReaderLocker(struct tty *tty)
	:
	fTTY(tty)
{
	Lock();
}


ReaderLocker::~ReaderLocker()
{
	Unlock();
}


status_t 
ReaderLocker::AcquireReader(bool dontBlock)
{
	// Release TTY lock in order not to block. We only need to do this
	// if we could have to wait for the buffer to become readable.
	if (!dontBlock)
		Unlock();

	status_t status = acquire_sem_etc(fTTY->read_sem, 1, (dontBlock ? B_TIMEOUT : 0) | B_CAN_INTERRUPT, 0);

	// reacquire TTY lock
	if (!dontBlock)
		Lock();

	if (status == B_OK)
		fBytes = line_buffer_readable(fTTY->input_buffer);

	return status;
}


void 
ReaderLocker::ReportRead(size_t bytesRead)
{
	if (bytesRead < fBytes && bytesRead != 0)
		release_sem(fTTY->read_sem);

	// there may be space to write too again
	if (bytesRead > 0)
		release_sem(fTTY->write_sem);
}


void 
ReaderLocker::Lock()
{
	mutex_lock(&fTTY->lock);
}


void 
ReaderLocker::Unlock()
{
	mutex_unlock(&fTTY->lock);
}


//	#pragma mark -


int32
get_tty_index(const char *name)
{
	// device names follow this form: "pt/%c%x"
	int8 digit = name[4];
	if (digit >= 'a')
		digit -= 'a';
	else
		digit -= '0';

	return (name[3] - 'p') * digit;
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

	tty->pgrp_id = 0;
		// this value prevents any signal of being sent

	tty->lock.sem = tty->read_sem = tty->write_sem = -1;
		// the semaphores are only created when the TTY is actually in use

	// some initial window size - the TTY in question should set these values
	tty->window_size.ws_col = 80;
	tty->window_size.ws_row = 25;
	tty->window_size.ws_xpixel = tty->window_size.ws_col * 8;
	tty->window_size.ws_ypixel = tty->window_size.ws_row * 8;
}


status_t
tty_output_getc(struct tty *tty, int *_c)
{
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
}


//	#pragma mark -
//	device functions


status_t
tty_close(struct tty *tty)
{
	mutex_destroy(&tty->lock);
	delete_sem(tty->write_sem);
	delete_sem(tty->read_sem);

	uninit_line_buffer(tty->input_buffer);

	return B_OK;
}


status_t
tty_open(struct tty *tty, tty_service_func func)
{
	status_t status;

	if (init_line_buffer(tty->input_buffer, TTY_BUFFER_SIZE) < B_OK)
		return B_NO_MEMORY;

	if ((status = mutex_init(&tty->lock, "tty lock")) < B_OK)
		goto err1;

	tty->write_sem = create_sem(1, "tty write");
	if (tty->write_sem < B_OK) {
		status = tty->write_sem;
		goto err2;
	}

	tty->read_sem = create_sem(0, "tty read");
	if (tty->read_sem < B_OK) {
		status = tty->read_sem;
		goto err3;
	}

	tty->service_func = func;

	return B_OK;

err3:
	delete_sem(tty->write_sem);
err2:
	mutex_destroy(&tty->lock);
err1:
	uninit_line_buffer(tty->input_buffer);
}


status_t
tty_ioctl(struct tty *tty, uint32 op, void *buffer, size_t length)
{
	TRACE(("tty_ioctl: tty %p, op %lu, buffer %p, length %lu\n", &tty, op, buffer, length));
	MutexLocker locker(&tty->lock);

	switch (op) {
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
tty_input_read(struct tty *tty, void *buffer, size_t *_length, uint32 mode)
{
	bool dontBlock = (mode & O_NONBLOCK) != 0;
	size_t length = *_length;
	ssize_t bytesRead = 0;

	if (length == 0)
		return B_OK;

	ReaderLocker locker(tty);

	while (bytesRead == 0) {
		status_t status = locker.AcquireReader(dontBlock);
		if (status != B_OK) {
			*_length = 0;
			return status;
		}

		// ToDo: add support for ICANON mode

		bytesRead = line_buffer_user_read(tty->input_buffer, (char *)buffer, length);
		if (bytesRead < B_OK) {
			*_length = 0;
			locker.ReportRead(0);

			return bytesRead;
		}
	}

	locker.ReportRead(bytesRead);
	*_length = bytesRead;

	return B_OK;
}


status_t
tty_write_to_tty(struct tty *source, struct tty *target, const void *buffer, size_t *_length,
	uint32 mode, bool sourceIsMaster)
{
	const char *data = (const char *)buffer;
	size_t length = *_length;
	size_t bytesWritten = 0;
	bool dontBlock = (mode & O_NONBLOCK) != 0;

	bool echo = (target->termios.c_lflag & ECHO) != 0;
		// Confusingly enough, we need to echo when the target's ECHO flag is
		// set. That's because our target is supposed to echo back at us, not
		// to itself.

	// ToDo: "buffer" is not yet copied or accessed in a safe way!

	if (length == 0)
		return B_OK;

	WriterLocker locker(source, target, echo, sourceIsMaster);

	while (bytesWritten < length) {
		status_t status = locker.AcquireWriter(dontBlock);
		if (status != B_OK) {
			*_length = bytesWritten;
			return status;
		}

		size_t writable = line_buffer_writable(target->input_buffer);
		if (echo) {
			// we can only write as much as is available on both ends
			size_t locallyWritable = line_buffer_writable(source->input_buffer);
			if (locallyWritable < writable)
				writable = locallyWritable;
		}

		if (writable == 0) {
			locker.ReportWritten(0);
			continue;
		}

		while (writable > bytesWritten && bytesWritten < length) {
			char c = data[0];

			if (c == '\n' && (source->termios.c_oflag & (OPOST | ONLCR)) == OPOST | ONLCR) {
				// post-process output and transfrom '\n' to '\r\n'
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

		locker.ReportWritten(bytesWritten);
	}

	return B_OK;
}


status_t
tty_select(struct tty *tty, uint8 event, uint32 ref, selectsync *sync)
{
	TRACE(("tty_select(event = %u, ref = %lu, sync = %p\n", event, ref, sync));
	return B_OK;
}


status_t
tty_deselect(struct tty *tty, uint8 event, selectsync *sync)
{
	TRACE(("tty_deselect(event = %u, sync = %p\n", event, sync));
	return B_OK;
}

