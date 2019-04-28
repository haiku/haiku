/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef TTY_PRIVATE_H
#define TTY_PRIVATE_H

#include <termios.h>

#include <Drivers.h>
#include <KernelExport.h>

#include <condition_variable.h>
#include <fs/select_sync_pool.h>
#include <lock.h>
#include <util/DoublyLinkedList.h>

#include "line_buffer.h"


#define TTY_BUFFER_SIZE 4096

// The only nice Rico'ism :)
#define CTRL(c)	((c) - 0100)


typedef status_t (*tty_service_func)(struct tty *tty, uint32 op);
	// not yet used...


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
	int32				opened_count;
	int32				index;
	struct mutex*		lock;
	tty_settings*		settings;
	select_sync_pool*	select_pool;
	RequestQueue		reader_queue;
	RequestQueue		writer_queue;
	TTYCookieList		cookies;
	line_buffer			input_buffer;
	tty_service_func	service_func;
	uint32				pending_eof;
	bool				is_master;
};

static const uint32 kNumTTYs = 64;

extern char *gDeviceNames[kNumTTYs * 2 + 3];

extern tty gMasterTTYs[kNumTTYs];
extern tty gSlaveTTYs[kNumTTYs];
extern tty_settings gTTYSettings[kNumTTYs];

extern device_hooks gMasterTTYHooks;
extern device_hooks gSlaveTTYHooks;

extern struct mutex gGlobalTTYLock;
extern struct mutex gTTYCookieLock;
extern struct recursive_lock gTTYRequestLock;


// functions available for master/slave TTYs

extern int32 get_tty_index(const char *name);
extern void reset_tty(struct tty *tty, int32 index, mutex* lock, bool isMaster);
extern void reset_tty_settings(tty_settings *settings, int32 index);
//extern status_t tty_input_putc(struct tty *tty, int c);
extern status_t tty_input_read(tty_cookie *cookie, void *buffer,
					size_t *_length);
extern status_t tty_output_getc(struct tty *tty, int *_c);
extern status_t tty_write_to_tty_master(tty_cookie *sourceCookie,
					const void *buffer, size_t *_length);
extern status_t tty_write_to_tty_slave(tty_cookie *sourceCookie,
					const void *buffer, size_t *_length);

extern status_t init_tty_cookie(tty_cookie *cookie, struct tty *tty,
					struct tty *otherTTY, uint32 openMode);
extern void uninit_tty_cookie(tty_cookie *cookie);
extern void add_tty_cookie(tty_cookie *cookie);
extern void tty_close_cookie(struct tty_cookie *cookie);

extern status_t tty_open(struct tty *tty, tty_service_func func);
extern status_t tty_close(struct tty *tty);
extern status_t tty_ioctl(tty_cookie *cookie, uint32 op, void *buffer,
					size_t length);
extern status_t tty_select(tty_cookie *cookie, uint8 event, uint32 ref,
					selectsync *sync);
extern status_t tty_deselect(tty_cookie *cookie, uint8 event, selectsync *sync);

extern void tty_add_debugger_commands();
extern void tty_remove_debugger_commands();

#endif	/* TTY_PRIVATE_H */
