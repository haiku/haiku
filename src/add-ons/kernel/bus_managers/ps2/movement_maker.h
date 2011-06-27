#ifndef MOVEMENT_MAKER_H
#define MOVEMENT_MAKER_H

#include <OS.h>

#include <keyboard_mouse_driver.h>
#include <touchpad_settings.h>


float floorf(float x);
float ceilf(float x);
float sqrtf(float x);
int32 make_small(float value);


struct touch_event {
	uint8		buttons;
	uint32		xPosition;
	uint32		yPosition;
	uint8		zPressure;
	// absolut mode (unused)
	bool		finger;
	bool		gesture;
	// absolut w mode
	uint8		wValue;
};


struct hardware_specs {
	uint16		edgeMotionWidth;

	uint16		areaStartX;
	uint16		areaEndX;
	uint16		areaStartY;
	uint16		areaEndY;

	uint16		minPressure;
	// the value you reach when you hammer really hard on the touchpad
	uint16		realMaxPressure;
	uint16		maxPressure;
};


/*! The raw movement calculation is calibrated on ths synaptics touchpad. */
// increase the touchpad size a little bit
#define SYN_AREA_TOP_LEFT_OFFSET	40
#define SYN_AREA_BOTTOM_RIGHT_OFFSET	60
#define SYN_AREA_START_X		(1472 - SYN_AREA_TOP_LEFT_OFFSET)
#define SYN_AREA_END_X			(5472 + SYN_AREA_BOTTOM_RIGHT_OFFSET)
#define SYN_WIDTH				(SYN_AREA_END_X - SYN_AREA_START_X)
#define SYN_AREA_START_Y		(1408 - SYN_AREA_TOP_LEFT_OFFSET)
#define SYN_AREA_END_Y			(4448 + SYN_AREA_BOTTOM_RIGHT_OFFSET)
#define SYN_HEIGHT				(SYN_AREA_END_Y - SYN_AREA_START_Y)


class MovementMaker {
public:
			void				SetSettings(touchpad_settings* settings);
			void				SetSpecs(hardware_specs* specs);

			int32				xDelta;
			int32				yDelta;

			float				scrolling_x;
			float				scrolling_y;
	
protected:
			void				StartNewMovment();
			void				GetMovement(uint32 posX, uint32 posY);
			void				GetScrolling(uint32 posX, uint32 posY);

			touchpad_settings*	fSettings;
			hardware_specs*		fSpecs;

			int8				fSpeed;
			int16				fAreaWidth;
			int16				fAreaHeight;

private:
			void				_GetRawMovement(uint32 posX, uint32 posY);
			void				_ComputeAcceleration(int8 accel_factor);

			
			bool				fMovementMakerStarted;

			uint32				fPreviousX;
			uint32				fPreviousY;
			int32				fDeltaSumX;
			int32				fDeltaSumY;

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
			void				Init();

			status_t			EventToMovement(touch_event *event,
									mouse_movement *movement);

			bool				TapDragStarted() { return fTapdragStarted; }
			bool				WasEdgeMotion() { return fValidEdgeMotion; }

			bigtime_t			click_speed;
private:
			bool				_EdgeMotion(mouse_movement *movement,
									touch_event *event, bool validStart);
	inline	void				_UpdateButtons(mouse_movement *movement);
	inline	void				_NoTouchToMovement(touch_event *event,
									mouse_movement *movement);
	inline	void				_MoveToMovement(touch_event *event,
									mouse_movement *movement);
	inline	bool				_CheckScrollingToMovement(touch_event *event,
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
