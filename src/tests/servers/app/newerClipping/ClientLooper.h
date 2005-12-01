
#ifndef CLIENT_LOOPER_H
#define CLIENT_LOOPER_H

#include <Looper.h>

class BMessageRunner;
class WindowLayer;

enum {
	MSG_UPDATE			= 'updt',
	MSG_VIEWS_ADDED		= 'vwad',
	MSG_VIEWS_REMOVED	= 'vwrm',

	MSG_WINDOW_HIDDEN	= 'whdn',

	MSG_TICK			= 'tick',
};

struct point {
	double x;
	double y;
	double direction_x;
	double direction_y;
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

			BMessageRunner*			fTicker;

			void					_DrawAnimatedLayer(int32 token);

			point					fPolygon[4];
};

#endif // CLIENT_LOOPER_H
