// MIMEManager.h

#ifndef MIME_MANAGER_H
#define MIME_MANAGER_H

#include <Looper.h>
#include <MimeDatabase.h>

class MIMEManager : public BLooper {
public:
	MIMEManager();
	virtual ~MIMEManager();

	virtual void MessageReceived(BMessage *message);
private:
	BPrivate::MimeDatabase fMimeDatabase;
};

#endif	// MIME_MANAGER_H
