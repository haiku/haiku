/*
 * Copyright 2007-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de
 */
#ifndef _KERNEL_NOTIFICATIONS_H
#define _KERNEL_NOTIFICATIONS_H


#include <SupportDefs.h>

#include <lock.h>
#include <messaging.h>
#include <util/khash.h>


#ifdef __cplusplus

#include <Referenceable.h>

#include <util/AutoLock.h>
#include <util/KMessage.h>
#include <util/OpenHashTable.h>


class NotificationService;

class NotificationListener {
public:
	virtual						~NotificationListener();

	virtual void				EventOccurred(NotificationService& service,
									const KMessage* event);
	virtual void				AllListenersNotified(
									NotificationService& service);

	virtual bool				operator==(
									const NotificationListener& other) const;

			bool				operator!=(
									const NotificationListener& other) const
									{ return !(*this == other); }
};

class UserMessagingMessageSender {
public:
								UserMessagingMessageSender();

			void				SendMessage(const KMessage* message,
									port_id port, int32 token);
			void				FlushMessage();

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
								UserMessagingListener(
									UserMessagingMessageSender& sender,
									port_id port, int32 token);
	virtual						~UserMessagingListener();

	virtual void				EventOccurred(NotificationService& service,
									const KMessage* event);
	virtual void				AllListenersNotified(
									NotificationService& service);

			port_id				Port() const	{ return fPort; }
			int32				Token() const	{ return fToken; }

			bool				operator==(
									const NotificationListener& _other) const;

private:
	UserMessagingMessageSender&	fSender;
	port_id						fPort;
	int32						fToken;
};

inline bool
UserMessagingListener::operator==(const NotificationListener& _other) const
{
	const UserMessagingListener* other
		= dynamic_cast<const UserMessagingListener*>(&_other);
	return other != NULL && other->Port() == Port()
		&& other->Token() == Token();
}

class NotificationService : public BReferenceable {
public:
	virtual						~NotificationService();

	virtual status_t			AddListener(const KMessage* eventSpecifier,
									NotificationListener& listener) = 0;
	virtual status_t			RemoveListener(const KMessage* eventSpecifier,
									NotificationListener& listener) = 0;
	virtual status_t			UpdateListener(const KMessage* eventSpecifier,
									NotificationListener& listener) = 0;

	virtual const char*			Name() = 0;
			NotificationService*&
								Link() { return fLink; }

private:
			NotificationService* fLink;
};

struct default_listener : public DoublyLinkedListLinkImpl<default_listener> {
	~default_listener();

	uint32	eventMask;
	team_id	team;
	NotificationListener* listener;
};

typedef DoublyLinkedList<default_listener> DefaultListenerList;


class DefaultNotificationService : public NotificationService {
public:
								DefaultNotificationService(const char* name);
	virtual						~DefaultNotificationService();

	inline	bool				Lock()
									{ return recursive_lock_lock(&fLock)
										== B_OK; }
	inline	void				Unlock()
									{ recursive_lock_unlock(&fLock); }

	inline	void				Notify(const KMessage& event, uint32 eventMask);
			void				NotifyLocked(const KMessage& event,
									uint32 eventMask);

	inline	bool				HasListeners() const
									{ return !fListeners.IsEmpty(); }
	virtual status_t			AddListener(const KMessage* eventSpecifier,
									NotificationListener& listener);
	virtual status_t			UpdateListener(const KMessage* eventSpecifier,
									NotificationListener& listener);
	virtual status_t			RemoveListener(const KMessage* eventSpecifier,
									NotificationListener& listener);

	virtual const char*			Name() { return fName; }

protected:
	virtual status_t			ToEventMask(const KMessage& eventSpecifier,
									uint32& eventMask);
	virtual	void				FirstAdded();
	virtual	void				LastRemoved();

			recursive_lock		fLock;
			DefaultListenerList	fListeners;
			const char*			fName;
};

class DefaultUserNotificationService : public DefaultNotificationService,
	NotificationListener {
public:
								DefaultUserNotificationService(
									const char* name);
	virtual						~DefaultUserNotificationService();

	virtual	status_t			AddListener(const KMessage* eventSpecifier,
									NotificationListener& listener);
	virtual	status_t			UpdateListener(const KMessage* eventSpecifier,
									NotificationListener& listener);
	virtual	status_t			RemoveListener(const KMessage* eventSpecifier,
									NotificationListener& listener);

			status_t			RemoveUserListeners(port_id port, uint32 token);
			status_t			UpdateUserListener(uint32 eventMask,
									port_id port, uint32 token);

private:
	virtual void				EventOccurred(NotificationService& service,
									const KMessage* event);
	virtual void				AllListenersNotified(
									NotificationService& service);
			status_t			_AddListener(uint32 eventMask,
									NotificationListener& listener);

			UserMessagingMessageSender fSender;
};

class NotificationManager {
public:
	static NotificationManager& Manager();
	static status_t CreateManager();

			status_t			RegisterService(NotificationService& service);
			void				UnregisterService(
									NotificationService& service);

			status_t			AddListener(const char* service,
									uint32 eventMask,
									NotificationListener& listener);
			status_t			AddListener(const char* service,
									const KMessage* eventSpecifier,
									NotificationListener& listener);

			status_t			UpdateListener(const char* service,
									uint32 eventMask,
									NotificationListener& listener);
			status_t			UpdateListener(const char* service,
									const KMessage* eventSpecifier,
									NotificationListener& listener);

			status_t			RemoveListener(const char* service,
									const KMessage* eventSpecifier,
									NotificationListener& listener);

private:
								NotificationManager();
								~NotificationManager();

			status_t			_Init();
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
		NotificationService*& GetLink(
				NotificationService* service) const
			{ return service->Link(); }
	};
	typedef BOpenHashTable<HashDefinition> ServiceHash;

	static	NotificationManager	sManager;

			mutex				fLock;
			ServiceHash			fServiceHash;
};


void
DefaultNotificationService::Notify(const KMessage& event, uint32 eventMask)
{
	RecursiveLocker _(fLock);
	NotifyLocked(event, eventMask);
}


extern "C" {

#endif	// __cplusplus

void notifications_init(void);

#ifdef __cplusplus
}
#endif	// __cplusplus

#endif	// _KERNEL_NOTIFICATIONS_H
