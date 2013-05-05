/*
 * Copyright 2009, Alexandre Deckner, alex@zappotek.com
 * Distributed under the terms of the MIT License.
 */

/*!
	\class DragTrackingFilter
	\brief A simple mouse drag detection filter
	*
	* A simple mouse filter that detects the start of a mouse drag over a
	* threshold distance and sends a message with the 'what' field of your
	* choice. Especially useful for drag and drop.
	* Allows you to free your code of encumbering mouse tracking details.
	*
	* It can detect fast drags spanning outside of a small view by temporarily
	* setting the B_POINTER_EVENTS flag on the view.
*/

#include <DragTrackingFilter.h>

#include <Message.h>
#include <Messenger.h>
#include <View.h>

static const int kSquaredDragThreshold = 9;

DragTrackingFilter::DragTrackingFilter(BView* targetView, uint32 messageWhat)
	: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
	fTargetView(targetView),
	fMessageWhat(messageWhat),
	fIsTracking(false),
	fClickButtons(0)
{
}


filter_result
DragTrackingFilter::Filter(BMessage* message, BHandler** /*_target*/)
{
	if (fTargetView == NULL)
		return B_DISPATCH_MESSAGE;

	switch (message->what) {
		case B_MOUSE_DOWN:
			message->FindPoint("where", &fClickPoint);
			message->FindInt32("buttons", (int32*)&fClickButtons);
			fIsTracking = true;

			fTargetView->SetMouseEventMask(B_POINTER_EVENTS);

			return B_DISPATCH_MESSAGE;

		case B_MOUSE_UP:
			fIsTracking = false;
			return B_DISPATCH_MESSAGE;

		case B_MOUSE_MOVED:
		{
			BPoint where;
			message->FindPoint("be:view_where", &where);

			// TODO: be more flexible about buttons and pass their state
			//		 in the message
			if (fIsTracking && (fClickButtons & B_PRIMARY_MOUSE_BUTTON)) {

				BPoint delta(fClickPoint - where);
				float squaredDelta = (delta.x * delta.x) + (delta.y * delta.y);

				if (squaredDelta >= kSquaredDragThreshold) {
					BMessage dragClickMessage(fMessageWhat);
					dragClickMessage.AddPoint("be:view_where", fClickPoint);
						// name it "be:view_where" since BView::DragMessage
						// positions the dragging frame/bitmap by retrieving the
						// current message and reading that field
					BMessenger messenger(fTargetView);
					messenger.SendMessage(&dragClickMessage);

					fIsTracking = false;
				}
			}
			return B_DISPATCH_MESSAGE;
		}
		default:
			break;
	}

	return B_DISPATCH_MESSAGE;
}
