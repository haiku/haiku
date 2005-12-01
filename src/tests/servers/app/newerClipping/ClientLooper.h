
#ifndef CLIENT_LOOPER_H
#define CLIENT_LOOPER_H

#include <Looper.h>

class WindowLayer;

enum {
	MSG_UPDATE			= 'updt',
	MSG_VIEWS_ADDED		= 'vwad',
	MSG_VIEWS_REMOVED	= 'vwrm',

	MSG_WINDOW_HIDDEN	= 'whdn',
};

class ClientLooper : public BLooper {
 public:
									ClientLooper(const char* name,
												 WindowLayer* serverWindow);
	virtual							~ClientLooper();

	virtual	void					MessageReceived(BMessage* message);

 private:
			WindowLayer*			fServerWindow;
			int32					fViewCount;
};

#endif // CLIENT_LOOPER_H
