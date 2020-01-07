/*
 * Copyright 2011, Alexandre Deckner, alex@zappotek.com
 * Distributed under the terms of the MIT License.
 */

/*!
	\class LongAndDragTrackingFilter
	\brief A simple long mouse down and drag detection filter
	*
	* A simple mouse filter that detects long clicks and pointer drags.
	* A long click message is sent when the mouse button is kept down
	* for a duration longer than a given threshold while the pointer stays
	* within the limits of a given threshold radius.
	* A drag message is triggered if the mouse goes further than the
	* threshold radius before the duration threshold elapsed.
	*
	* The messages contain the pointer position and the buttons state at
	* the moment of the click. The drag message is ready to use with the
	* be/haiku drag and drop API cf. comment in code.
	*
	* Current limitation: A long mouse down or a drag can be detected for
	* any mouse button, but any released button cancels the tracking.
	*
*/


#include <LongAndDragTrackingFilter.h>

#include <Message.h>
#include <Messenger.h>
#include <MessageRunner.h>
#include <View.h>

#include <new>


LongAndDragTrackingFilter::LongAndDragTrackingFilter(uint32 longMessageWhat,
	uint32 dragMessageWhat, float radiusThreshold,
	bigtime_t durationThreshold)
	:
	BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
	fLongMessageWhat(longMessageWhat),
	fDragMessageWhat(dragMessageWhat),
	fMessageRunner(NULL),
	fClickButtons(0),
	fSquaredRadiusThreshold(radiusThreshold * radiusThreshold),
	fDurationThreshold(durationThreshold)
{
	if (durationThreshold == 0) {
		get_click_speed(&fDurationThreshold);
			// use system's doubleClickSpeed as default threshold
	}
}


LongAndDragTrackingFilter::~LongAndDragTrackingFilter()
{
	delete fMessageRunner;
}


void
LongAndDragTrackingFilter::_StopTracking()
{
	delete fMessageRunner;
	fMessageRunner = NULL;
}


filter_result
LongAndDragTrackingFilter::Filter(BMessage* message, BHandler** target)
{
	if (*target == NULL)
		return B_DISPATCH_MESSAGE;

	switch (message->what) {
		case B_MOUSE_DOWN: {
			int32 clicks = 0;

			message->FindInt32("buttons", (int32*)&fClickButtons);
			message->FindInt32("clicks", (int32*)&clicks);

			if (fClickButtons != 0 && clicks == 1) {

				BView* targetView = dynamic_cast<BView*>(*target);
				if (targetView != NULL)
					targetView->SetMouseEventMask(B_POINTER_EVENTS);

				message->FindPoint("where", &fClickPoint);
				BMessage message(fLongMessageWhat);
				message.AddPoint("where", fClickPoint);
				message.AddInt32("buttons", fClickButtons);

				delete fMessageRunner;
				fMessageRunner = new (std::nothrow) BMessageRunner(
					BMessenger(*target), &message, fDurationThreshold, 1);
			}
			return B_DISPATCH_MESSAGE;
		}

		case B_MOUSE_UP:
			_StopTracking();
			message->AddInt32("last_buttons", (int32)fClickButtons);
			message->FindInt32("buttons", (int32*)&fClickButtons);
			return B_DISPATCH_MESSAGE;

		case B_MOUSE_MOVED:
		{
			if (fMessageRunner != NULL) {
				BPoint where;
				message->FindPoint("be:view_where", &where);

				BPoint delta(fClickPoint - where);
				float squaredDelta = (delta.x * delta.x) + (delta.y * delta.y);

				if (squaredDelta >= fSquaredRadiusThreshold) {
					BMessage dragMessage(fDragMessageWhat);
					dragMessage.AddPoint("be:view_where", fClickPoint);
						// name it "be:view_where" since BView::DragMessage
						// positions the dragging frame/bitmap by retrieving
						// the current message and reading that field
					dragMessage.AddInt32("buttons", (int32)fClickButtons);
					BMessenger messenger(*target);
					messenger.SendMessage(&dragMessage);

					_StopTracking();
				}
			}
			return B_DISPATCH_MESSAGE;
		}

		default:
			if (message->what == fLongMessageWhat) {
				_StopTracking();
				return B_DISPATCH_MESSAGE;
			}
			break;
	}

	return B_DISPATCH_MESSAGE;
}
