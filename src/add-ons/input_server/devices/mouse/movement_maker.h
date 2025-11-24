/*
 * Copyright 2008-2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Copyright 2022-2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef MOVEMENT_MAKER_H
#define MOVEMENT_MAKER_H

#include <OS.h>

#include <keyboard_mouse_driver.h>
#include <touchpad_settings.h>


class MovementMaker {
public:
			void				SetSpecs(const touchpad_specs& specs);
			void				SetSettings(const touchpad_settings& settings);

			float				xDelta;
			float				yDelta;

			float				scrolling_x;
			float				scrolling_y;
	
protected:
			void				StartNewMovment();
			void				GetMovement(uint32 posX, uint32 posY);
			void				GetScrolling(uint32 posX, uint32 posY, bool reverse);

			touchpad_specs		fSpecs;
			touchpad_settings	fSettings;

			int8				fSpeed;
			int16				fAreaWidth;
			int16				fAreaHeight;

private:
			void				_GetRawMovement(uint32 posX, uint32 posY);
			void				_ComputeAcceleration(int8 accel_factor);

			
			bool				fMovementMakerStarted;

private:
			uint32				fPreviousX;
			uint32				fPreviousY;

			float				fDeltaSumX;
			float				fDeltaSumY;

			int8				fSmallMovement;
};


enum button_ids
{
	kNoButton = 0x00,
	kLeftButton = 0x01,
	kRightButton = 0x02,
	kMiddleButton = 0x04
};


class TouchpadMovement : public MovementMaker {
public:
								TouchpadMovement();
	virtual						~TouchpadMovement();

			status_t			EventToMovement(const touchpad_movement *event,
									mouse_movement *movement, bigtime_t &repeatTimeout);

			bigtime_t			click_speed;
private:
			void				_UpdateButtons(mouse_movement *movement);
			bool				_EdgeMotion(const touchpad_movement *event,
									mouse_movement *movement, bool validStart);
	inline	void				_NoTouchToMovement(const touchpad_movement *event,
									mouse_movement *movement);
	inline	void				_MoveToMovement(const touchpad_movement *event,
									mouse_movement *movement);
	inline	bool				_CheckScrollingToMovement(const touchpad_movement *event,
									mouse_movement *movement);


			bool				fMovementStarted;
			bool				fScrollingStarted;
			bool				fTapStarted;
			bigtime_t			fTapTime;
			int32				fTapDeltaX;
			int32				fTapDeltaY;
			int32				fTapClicks;
			bool				fTapdragStarted;

			bool				fValidEdgeMotion;
			bigtime_t			fLastEdgeMotion;
			float				fRestEdgeMotion;

			bool				fDoubleClick;

			bigtime_t			fClickLastTime;
			int32				fClickCount;
			uint32				fButtonsState;
};


#endif
