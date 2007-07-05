/*
 * Copyright 2006-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef SERVICES_H
#define SERVICES_H


#include <Handler.h>
#include <Locker.h>

#include <map>
#include <string>
#include <sys/select.h>


struct service;
struct service_address;
typedef std::map<std::string, service*> ServiceNameMap;
typedef std::map<int, service_address*> ServiceSocketMap;


class Services : public BHandler {
	public:
		Services(const BMessage& services);
		virtual ~Services();

		status_t InitCheck() const;

		virtual void MessageReceived(BMessage* message);

	private:
		void _NotifyListener(bool quit = false);
		void _UpdateMinMaxSocket(int socket);
		status_t _StartService(struct service& service);
		status_t _StopService(struct service& service);
		status_t _ToService(const BMessage& message, struct service*& service);
		void _Update(const BMessage& services);
		int32 _CompareServices(struct service& a, struct service& b);
		status_t _LaunchService(struct service& service, int socket);
		status_t _Listener();
		static status_t _Listener(void* self);

		thread_id	fListener;
		BLocker		fLock;
		ServiceNameMap fNameMap;
		ServiceSocketMap fSocketMap;
		uint32		fUpdate;
		int			fReadPipe;
		int			fWritePipe;
		int			fMinSocket;
		int			fMaxSocket;
		fd_set		fSet;
};

const static uint32 kMsgUpdateServices = 'srvU';

#endif	// SERVICES_H
