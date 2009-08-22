/*
 * Copyright 2009, Alexandre Deckner, alex@zappotek.com
 * Distributed under the terms of the MIT License.
 */
#ifndef SHAKE_TRACKING_FILTER_H
#define SHAKE_TRACKING_FILTER_H


#include <MessageFilter.h>
#include <Point.h>

class BView;
class BHandler;
class BMessageRunner;

namespace BPrivate {

class LowPassFilter {
public:
						LowPassFilter(uint32 size);
						~LowPassFilter();

			void	    Input(const BPoint& p);
			BPoint		Output() const;
private:
	BPoint* fPoints;
	uint32	fSize;
	BPoint	fSum;
};


class ShakeTrackingFilter : public BMessageFilter {
public:
								ShakeTrackingFilter(
									BView* targetView,
									uint32 messageWhat,
									uint32 countThreshold = 2,
									bigtime_t timeTreshold = 400000);

								~ShakeTrackingFilter();

			filter_result		Filter(BMessage* message, BHandler** _target);

private:
			BView*				fTargetView;
			uint32				fMessageWhat;

			BMessageRunner*		fCancelRunner;
			LowPassFilter		fLowPass;
			BPoint				fLastDelta;
			BPoint				fLastPosition;
			uint32				fCounter;
			uint32				fCountThreshold;
			bigtime_t			fTimeThreshold;
};

}	// namespace BPrivate

using BPrivate::ShakeTrackingFilter;
using BPrivate::LowPassFilter;

#endif	// SHAKE_TRACKING_FILTER_H
