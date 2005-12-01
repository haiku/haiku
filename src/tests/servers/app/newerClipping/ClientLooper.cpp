
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <Message.h>
#include <MessageQueue.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <Rect.h>
#include <String.h>

#include "WindowLayer.h"

#include "ClientLooper.h"

#define SLOW_DRAWING 0

#define SPEED 2.0

// random_number_between
static float
random_number_between(float v1, float v2)
{
	if (v1 < v2)
		return v1 + fmod(rand() / 1000.0, (v2 - v1));
	else if (v2 < v1)
		return v2 + fmod(rand() / 1000.0, (v1 - v2));
	return v1;
}

// init_polygon
static void
init_polygon(const BRect& b, point* polygon)
{
	polygon[0].x = b.left;
	polygon[0].y = b.top;
	polygon[0].direction_x = random_number_between(-SPEED, SPEED);
	polygon[0].direction_y = random_number_between(-SPEED, SPEED);
	polygon[1].x = b.right;
	polygon[1].y = b.top;
	polygon[1].direction_x = random_number_between(-SPEED, SPEED);
	polygon[1].direction_y = random_number_between(-SPEED, SPEED);
	polygon[2].x = b.right;
	polygon[2].y = b.bottom;
	polygon[2].direction_x = random_number_between(-SPEED, SPEED);
	polygon[2].direction_y = random_number_between(-SPEED, SPEED);
	polygon[3].x = b.left;
	polygon[3].y = b.bottom;
	polygon[3].direction_x = random_number_between(-SPEED, SPEED);
	polygon[3].direction_y = random_number_between(-SPEED, SPEED);
}

// morph
static inline void
morph(double* value, double* direction, double min, double max)
{
	*value += *direction;
	if (*value < min) {
		*value = min;
		*direction = -*direction;
	} else if (*value > max) {
		*value = max;
		*direction = -*direction;
	}
}

// morph_polygon
static inline void
morph_polygon(const BRect& b, point* polygon)
{
	morph(&polygon[0].x, &polygon[0].direction_x, b.left, b.right);
	morph(&polygon[1].x, &polygon[1].direction_x, b.left, b.right);
	morph(&polygon[2].x, &polygon[2].direction_x, b.left, b.right);
	morph(&polygon[3].x, &polygon[3].direction_x, b.left, b.right);
	morph(&polygon[0].y, &polygon[0].direction_y, b.top, b.bottom);
	morph(&polygon[1].y, &polygon[1].direction_y, b.top, b.bottom);
	morph(&polygon[2].y, &polygon[2].direction_y, b.top, b.bottom);
	morph(&polygon[3].y, &polygon[3].direction_y, b.top, b.bottom);
}



// constructor
ClientLooper::ClientLooper(const char* name, WindowLayer* serverWindow)
	: BLooper("", B_DISPLAY_PRIORITY),
	  fServerWindow(serverWindow),
	  fViewCount(0)
{
	BString clientName(name);
	clientName << " client";
	SetName(clientName.String());

	BMessenger messenger(this);
	fTicker = new BMessageRunner(messenger, new BMessage(MSG_TICK), 40000);

	init_polygon(BRect(0, 0, 100, 100), fPolygon);
}

// destructor
ClientLooper::~ClientLooper()
{
	delete fTicker;
}

// MessageReceived
void
ClientLooper::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_UPDATE:

			fServerWindow->PostMessage(MSG_BEGIN_UPDATE);

			for (int32 i = 0; i < fViewCount; i++) {
				// the client is slow
//				snooze(40000L);
				// send the command to redraw a view
				if (i == 5) {
					_DrawAnimatedLayer(i);
				} else {
					BMessage command(MSG_DRAWING_COMMAND);
					command.AddInt32("token", i);
					fServerWindow->PostMessage(&command);
				}
			}

			fServerWindow->PostMessage(MSG_END_UPDATE);

			break;
		case MSG_VIEWS_ADDED: {
			int32 count;
			if (message->FindInt32("count", &count) >= B_OK) {
				fViewCount += count;
			}
			break;
		}
		case MSG_VIEWS_REMOVED: {
			int32 count;
			if (message->FindInt32("count", &count) >= B_OK)
				fViewCount -= count;
			break;
		}

		case MSG_WINDOW_HIDDEN:
			// there is no way we're going to accept this
			// discrimination for longer than 2 seconds!
			snooze(2000000);
			fServerWindow->PostMessage(MSG_SHOW);
			break;

		case MSG_TICK: {
			BMessage invalidate(MSG_INVALIDATE_VIEW);
			invalidate.AddInt32("token", 5);
			fServerWindow->PostMessage(&invalidate);

			morph_polygon(BRect(0, 0, 100, 100), fPolygon);
			break;
		}

		default:
			BLooper::MessageReceived(message);
			break;
	}
}

// _DrawAnimatedLayer
void
ClientLooper::_DrawAnimatedLayer(int32 token)
{
	BMessage message(MSG_DRAW_POLYGON);
	message.AddInt32("token", token);

	message.AddPoint("point", BPoint(fPolygon[0].x, fPolygon[0].y));
	message.AddPoint("point", BPoint(fPolygon[1].x, fPolygon[1].y));
	message.AddPoint("point", BPoint(fPolygon[2].x, fPolygon[2].y));
	message.AddPoint("point", BPoint(fPolygon[3].x, fPolygon[3].y));

	fServerWindow->PostMessage(&message);
}

