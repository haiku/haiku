// MIMEManager.h

#ifndef MIME_MANAGER_H
#define MIME_MANAGER_H

#include <Looper.h>
#include <mime/Database.h>
#include <ThreadManager.h>

class MIMEManager : public BLooper {
public:
	MIMEManager();
	virtual ~MIMEManager();

	virtual void MessageReceived(BMessage *message);
private:
	void HandleSetParam(BMessage *message);
	void HandleDeleteParam(BMessage *message);
	
	BPrivate::Storage::Mime::Database fDatabase;
	ThreadManager fThreadManager;
	BMessenger fManagerMessenger;
};

#endif	// MIME_MANAGER_H
