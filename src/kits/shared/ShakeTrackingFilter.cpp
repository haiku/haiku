/*
 * Copyright 2009, Alexandre Deckner, alex@zappotek.com
 * Distributed under the terms of the MIT License.
 */

/*!
	\class ShakeTrackingFilter
	\brief A simple mouse shake detection filter
	*
	* A simple mouse filter that detects quick mouse shakes.
	*
	* It's detecting rough edges (u-turns) in the mouse movement
	* and counts them within a time window.
	* You can configure the message sent, the u-turn count threshold
	* and the time threshold.
	* It sends the count along with the message.
	* For now, detection is limited within the view bounds, but
	* it might be modified to accept a BRegion mask.
	*
*/


#include <ShakeTrackingFilter.h>

#include <Message.h>
#include <Messenger.h>
#include <MessageRunner.h>
#include <View.h>


const uint32 kMsgCancel = 'Canc';


ShakeTrackingFilter::ShakeTrackingFilter(BView* targetView, uint32 messageWhat,
	uint32 countThreshold, bigtime_t timeThreshold)
	:
	BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
	fTargetView(targetView),
	fMessageWhat(messageWhat),
	fCancelRunner(NULL),
	fLowPass(8),
	fLastDelta(0, 0),
	fCounter(0),
	fCountThreshold(countThreshold),
	fTimeThreshold(timeThreshold)
{
}


ShakeTrackingFilter::~ShakeTrackingFilter()
{
	delete fCancelRunner;
}


filter_result
ShakeTrackingFilter::Filter(BMessage* message, BHandler** /*_target*/)
{
	if (fTargetView == NULL)
		return B_DISPATCH_MESSAGE;

	switch (message->what) {
		case B_MOUSE_MOVED:
		{
			BPoint position;
			message->FindPoint("be:view_where", &position);

			// TODO: allow using BRegion masks
			if (!fTargetView->Bounds().Contains(position))
				return B_DISPATCH_MESSAGE;

			fLowPass.Input(position - fLastPosition);

			BPoint delta = fLowPass.Output();

			// normalized dot product
			float norm = delta.x * delta.x + delta.y * delta.y;
			if (norm > 0.01) {
				delta.x /= norm;
				delta.y /= norm;
			}

			norm = fLastDelta.x * fLastDelta.x + fLastDelta.y * fLastDelta.y;
			if (norm > 0.01) {
				fLastDelta.x /= norm;
				fLastDelta.y /= norm;
			}

			float dot = delta.x * fLastDelta.x + delta.y * fLastDelta.y;

			if (dot < 0.0) {
				if (fCounter == 0) {
					BMessage * cancelMessage = new BMessage(kMsgCancel);
		 			fCancelRunner = new BMessageRunner(BMessenger(fTargetView),
		 				cancelMessage, fTimeThreshold, 1);
				}

				fCounter++;

				if (fCounter >= fCountThreshold) {
					BMessage shakeMessage(fMessageWhat);
					shakeMessage.AddUInt32("count", fCounter);
					BMessenger messenger(fTargetView);
					messenger.SendMessage(&shakeMessage);
				}
			}

			fLastDelta = fLowPass.Output();
			fLastPosition = position;

			return B_DISPATCH_MESSAGE;
		}

		case kMsgCancel:
			delete fCancelRunner;
			fCancelRunner = NULL;
			fCounter = 0;
			return B_SKIP_MESSAGE;

		default:
			break;
	}

	return B_DISPATCH_MESSAGE;
}


//	#pragma mark -


LowPassFilter::LowPassFilter(uint32 size)
	:
	fSize(size)
{
	fPoints = new BPoint[fSize];
}


LowPassFilter::~LowPassFilter()
{
	delete [] fPoints;
}


void
LowPassFilter::Input(const BPoint& p)
{
	// A fifo buffer that maintains a sum of its elements
	fSum -= fPoints[0];
	for (uint32 i = 0; i < fSize - 1; i++)
		fPoints[i] = fPoints[i + 1];
	fPoints[fSize - 1] = p;
	fSum += p;
}


BPoint
LowPassFilter::Output() const
{
	return BPoint(fSum.x / (float) fSize, fSum.y / (float) fSize);
}
