// MIMEManager.h

#ifndef MIME_MANAGER_H
#define MIME_MANAGER_H

#include <Looper.h>

#include <mime/Database.h>

#include "RegistrarThreadManager.h"


class MIMEManager : public BLooper,
	private BPrivate::Storage::Mime::Database::NotificationListener {
public:
	MIMEManager();
	virtual ~MIMEManager();

	virtual void MessageReceived(BMessage *message);

private:
	// Database::NotificationListener
	virtual status_t Notify(BMessage* message, const BMessenger& target);

private:
	class DatabaseLocker;

private:
	void HandleSetParam(BMessage *message);
	void HandleDeleteParam(BMessage *message);

private:
	BPrivate::Storage::Mime::Database fDatabase;
	DatabaseLocker* fDatabaseLocker;
	RegistrarThreadManager fThreadManager;
	BMessenger fManagerMessenger;
};

#endif	// MIME_MANAGER_H
