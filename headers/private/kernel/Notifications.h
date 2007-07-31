/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de
 */
#ifndef _KERNEL_NOTIFICATIONS_H
#define _KERNEL_NOTIFICATIONS_H


#include <SupportDefs.h>

#include <Referenceable.h>

#include <lock.h>
#include <messaging.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <util/khash.h>
#include <util/KMessage.h>
#include <util/OpenHashTable.h>


#ifdef __cplusplus

class NotificationService;

class NotificationListener
	: public DoublyLinkedListLinkImpl<NotificationListener> {
	public:
		virtual ~NotificationListener();

		virtual void EventOccured(NotificationService& service,
			const KMessage* event);
		virtual void AllListenersNotified();
};

class UserMessagingMessageSender {
	public:
		UserMessagingMessageSender();

		void SendMessage(const KMessage* message, port_id port, int32 token);
		void FlushMessage();

	private:
		enum {
			MAX_MESSAGING_TARGET_COUNT	= 16,
		};

		const KMessage*		fMessage;
		messaging_target	fTargets[MAX_MESSAGING_TARGET_COUNT];
		int32				fTargetCount;
};

class UserMessagingListener : public NotificationListener {
	public:
		UserMessagingListener(UserMessagingMessageSender& sender, port_id port,
			int32 token);
		virtual ~UserMessagingListener();

		virtual void EventOccured(NotificationService& service,
			const KMessage* event);
		virtual void AllListenersNotified();

		port_id Port()	{ return fPort; }
		int32 Token()	{ return fToken; }

	private:
		UserMessagingMessageSender&	fSender;
		port_id						fPort;
		int32						fToken;
};

class NotificationListenerUpdater {
	public:
		enum update_action {
			UPDATED,
			SKIP,
			DELETE,
			REMOVE
		};

		NotificationListenerUpdater(const KMessage* eventSpecifier);
		virtual ~NotificationListenerUpdater();

		virtual status_t UpdateListener(NotificationListener& listener,
			enum update_action& action);

		virtual status_t CreateListener(NotificationListener** _listener);

		virtual void SetEventSpecifier(const KMessage* eventSpecifier);
		const KMessage* EventSpecifier() const	{ return fEventSpecifier; }

	protected:
		const KMessage* fEventSpecifier;
};

class UserMessagingListenerUpdater : public NotificationListenerUpdater {
	public:
		UserMessagingListenerUpdater(const KMessage* eventSpecifier, port_id port,
			int32 token);

		virtual status_t UpdateListener(NotificationListener& listener,
			enum update_action& action);

	protected:
		virtual status_t UpdateListener(UserMessagingListener& listener,
			enum update_action& action) = 0;

		port_id	fPort;
		int32	fToken;
};

class NotificationService : public Referenceable {
	public:
		virtual ~NotificationService() = 0;

		virtual status_t AddListener(const KMessage* eventSpecifier,
			NotificationListener& listener) = 0;
		virtual status_t RemoveListener(const KMessage* eventSpecifier,
			NotificationListener& listener) = 0;
		virtual status_t UpdateListener(NotificationListenerUpdater& updater) = 0;

		virtual const char* Name() = 0;
		HashTableLink<NotificationService>& Link() { return fLink; }

	private:
		HashTableLink<NotificationService> fLink;
};

class NotificationManager {
	public:
		static NotificationManager& Manager();
		static status_t CreateManager();

		status_t RegisterService(NotificationService& service);
		void UnregisterService(NotificationService& service);

		NotificationService* GetService(const char* name);
		void PutService(NotificationService* service);

		status_t AddListener(const char* service, uint32 eventMask,
			NotificationListener& listener);
		status_t AddListener(const char* service,
			const KMessage* eventSpecifier, NotificationListener& listener);

		status_t RemoveListener(const char* service, uint32 eventMask,
			NotificationListener& listener);
		status_t RemoveListener(const char* service,
			const KMessage* eventSpecifier, NotificationListener& listener);

		status_t UpdateListener(const char* service,
			NotificationListenerUpdater& updater);

	private:
		NotificationManager();
		~NotificationManager();

		status_t _Init();
		NotificationService* _ServiceFor(const char* name);

		struct HashDefinition {
			typedef const char* KeyType;
			typedef	NotificationService ValueType;

			size_t HashKey(const char* key) const
				{ return hash_hash_string(key); }
			size_t Hash(NotificationService *service) const
				{ return hash_hash_string(service->Name()); }
			bool Compare(const char* key, NotificationService* service) const
				{ return !strcmp(key, service->Name()); }
			HashTableLink<NotificationService>* GetLink(
					NotificationService* service) const
				{ return &service->Link(); }
		};

		static NotificationManager sManager;

		mutex fLock;
		typedef OpenHashTable<HashDefinition> ServiceHash;
		ServiceHash	fServiceHash;
};

extern "C" {

#endif	// __cplusplus

void notifications_init(void);

#ifdef __cplusplus
}
#endif	// __cplusplus

#endif	// _KERNEL_NOTIFICATIONS_H
