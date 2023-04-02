/*
 * Copyright 2017 Julian Harnath <julian.harnath@rwth-aachen.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef BARBER_POLE_H
#define BARBER_POLE_H


#include <View.h>

#include <Polygon.h>


class BMessageRunner;


/*! Spinning barber pole progress indicator. Number and colors of the
    color stripes are configurable. By default, it will be 2 colors,
    chosen from the system color palette.
*/
class BarberPole : public BView {
public:
	enum {
		// Message codes
		kRefreshMessage = 0x1001
	};

public:
								BarberPole(const char* name);
								~BarberPole();

	virtual	void				MessageReceived(BMessage* message);
	virtual void				Draw(BRect updateRect);
	virtual	void				FrameResized(float width, float height);

	virtual	BSize				MinSize();

			void				Start();
			void				Stop();

			void				SetSpinSpeed(float speed);
			void				SetColors(const rgb_color* colors,
									uint32 numColors);

private:
			void				_Spin();
			void				_DrawSpin(BRect updateRect);
			void				_DrawNonSpin(BRect updateRect);
private:
			bool				fIsSpinning;
			float				fSpinSpeed;
			const rgb_color*	fColors;
			uint32				fNumColors;

			float				fScrollOffset;
			BPolygon			fStripe;
			float				fStripeWidth;
			uint32				fNumStripes;
};


#endif // BARBER_POLE_H
