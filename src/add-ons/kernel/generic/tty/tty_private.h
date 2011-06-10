/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TTY_PRIVATE_H
#define TTY_PRIVATE_H

#include <termios.h>

#include <Drivers.h>
#include <KernelExport.h>
#include <tty/tty_module.h>

#include <condition_variable.h>
#include <fs/select_sync_pool.h>
#include <lock.h>
#include <util/DoublyLinkedList.h>

#include "line_buffer.h"


#define TTY_BUFFER_SIZE 4096

// The only nice Rico'ism :)
#define CTRL(c)	((c) - 0100)


class RequestOwner;
class Semaphore;
struct tty;
struct tty_cookie;

class Request : public DoublyLinkedListLinkImpl<Request> {
	public:
		Request();

		void Init(RequestOwner *owner, tty_cookie *cookie, size_t bytesNeeded);

		tty_cookie *TTYCookie() const	{ return fCookie; }

		void Notify(size_t bytesAvailable);
		void NotifyError(status_t error);

		bool WasNotified() const		{ return fNotified; }
		bool HasError() const			{ return fError; }

		void Dump(const char* prefix);

	private:
		RequestOwner	*fOwner;
		tty_cookie		*fCookie;
		size_t			fBytesNeeded;
		bool			fNotified;
		bool			fError;
};

class RequestQueue {
	public:
		RequestQueue();
		~RequestQueue()	{}

		void Add(Request *request);
		void Remove(Request *request);

		Request *First() const				{ return fRequests.First(); }
		bool IsEmpty() const				{ return fRequests.IsEmpty(); }

		void NotifyFirst(size_t bytesAvailable);
		void NotifyError(status_t error);
		void NotifyError(tty_cookie *cookie, status_t error);

		void Dump(const char* prefix);

	private:
		typedef DoublyLinkedList<Request> RequestList;

		RequestList	fRequests;
};

class RequestOwner {
	public:
		RequestOwner();

		void Enqueue(tty_cookie *cookie, RequestQueue *queue1,
			RequestQueue *queue2 = NULL);
		void Dequeue();

		void SetBytesNeeded(size_t bytesNeeded);
		size_t BytesNeeded() const	{ return fBytesNeeded; }

		status_t Wait(bool interruptable, bigtime_t timeout = B_INFINITE_TIMEOUT);

		bool IsFirstInQueues();

		void Notify(Request *request);
		void NotifyError(Request *request, status_t error);

		status_t Error() const	{ return fError; }

	private:
		ConditionVariable*		fConditionVariable;
		tty_cookie*				fCookie;
		status_t				fError;
		RequestQueue*			fRequestQueues[2];
		Request					fRequests[2];
		size_t					fBytesNeeded;
};


struct tty_cookie : DoublyLinkedListLinkImpl<tty_cookie> {
	struct tty			*tty;
	struct tty			*other_tty;
	uint32				open_mode;
	int32				thread_count;
	sem_id				blocking_semaphore;
	bool				closed;
};

typedef DoublyLinkedList<tty_cookie> TTYCookieList;

struct tty_settings {
	pid_t				pgrp_id;
	pid_t				session_id;
	struct termios		termios;
	struct winsize		window_size;
};

struct tty {
	int32				ref_count;	// referenced by cookies
	int32				open_count;
	struct mutex		lock;
	tty_settings		settings;
	select_sync_pool*	select_pool;
	RequestQueue		reader_queue;
	RequestQueue		writer_queue;
	TTYCookieList		cookies;
	line_buffer			input_buffer;
	tty_service_func	service_func;
	uint32				pending_eof;
	bool				is_master;
	uint8				hardware_bits;
};


extern struct mutex gTTYCookieLock;
extern struct recursive_lock gTTYRequestLock;

extern struct tty *tty_create(tty_service_func func, bool isMaster);
extern void tty_destroy(struct tty *tty);

extern tty_cookie *tty_create_cookie(struct tty *tty, struct tty *otherTTY,
	uint32 openMode);
extern void tty_destroy_cookie(tty_cookie *cookie);
extern void tty_close_cookie(tty_cookie *cookie);

extern status_t tty_read(tty_cookie *cookie, void *buffer, size_t *_length);
extern status_t tty_write(tty_cookie *sourceCookie, const void *buffer,
	size_t *_length);
extern status_t tty_control(tty_cookie *cookie, uint32 op, void *buffer,
	size_t length);
extern status_t tty_select(tty_cookie *cookie, uint8 event, uint32 ref,
	selectsync *sync);
extern status_t tty_deselect(tty_cookie *cookie, uint8 event, selectsync *sync);

extern status_t tty_input_lock(tty_cookie* cookie, bool lock);
extern status_t tty_hardware_signal(tty_cookie* cookie, int signal, bool);

#endif	/* TTY_PRIVATE_H */
