/*
 * Copyright 2005-2016, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef MESSAGE_LOOPER_H
#define MESSAGE_LOOPER_H


#include <PortLink.h>
#include <Locker.h>
#include <OS.h>


class MessageLooper : public BLocker {
public:
								MessageLooper(const char* name);
	virtual						~MessageLooper();

	virtual	status_t			Run();
	virtual	void				Quit();

			status_t			PostMessage(int32 code,
									bigtime_t timeout = B_INFINITE_TIMEOUT);

			thread_id			Thread() const { return fThread; }
			bool				IsQuitting() const { return fQuitting; }
			sem_id				DeathSemaphore() const
									{ return fDeathSemaphore; }

	virtual	port_id				MessagePort() const = 0;

	static	status_t			WaitForQuit(sem_id semaphore,
									bigtime_t timeout = B_INFINITE_TIMEOUT);

private:
	virtual	void				_PrepareQuit();
	virtual	void				_GetLooperName(char* name, size_t length);
	virtual	void				_DispatchMessage(int32 code,
									BPrivate::LinkReceiver& link);
	virtual	void				_MessageLooper();

protected:
	static	int32				_message_thread(void*_looper);

protected:
			const char*			fName;
			thread_id			fThread;
			BPrivate::PortLink	fLink;
			bool				fQuitting;
			sem_id				fDeathSemaphore;
};


static const int32 kMsgQuitLooper = 'quit';


#endif	/* MESSAGE_LOOPER_H */
