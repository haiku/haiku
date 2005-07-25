/*
 * Copyright 2005, Haiku.
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
		virtual ~MessageLooper();

		virtual	bool	Run();
		virtual	void	Quit();

		void			PostMessage(int32 code);
		thread_id		Thread() const { return fThread; }
		bool			IsQuitting() const { return fQuitting; }

		virtual port_id	MessagePort() const = 0;

	private:
		virtual void	_PrepareQuit();
		virtual void	_GetLooperName(char* name, size_t length);
		virtual void	_DispatchMessage(int32 code, BPrivate::LinkReceiver &link);
		virtual void	_MessageLooper();

	protected:
		static int32	_message_thread(void *_looper);

	protected:
		thread_id		fThread;
		BPrivate::PortLink fLink;
		bool			fQuitting;
};

static const int32 kMsgQuitLooper = 'quit';

#endif	/* MESSAGE_LOOPER_H */
