#ifndef SCROLLBAR_KNOB_ADJUSTER_H
#define SCROLLBAR_KNOB_ADJUSTER_H

#include <View.h>


class KnobSizeAdjuster : public BView
{
	public:
		KnobSizeAdjuster( BPoint position, uint32 size );
		
		void SetKnobSize( uint32 size );	
		void SetPressed( bool pressed );
		void SetTracking( bool tracking );
		bool IsPressed();
		bool IsTracking();
	private:
		void Draw( BRect updateRect );
		void MouseDown( BPoint point );
		void MouseUp( BPoint point );
		void MouseMoved( BPoint point, uint32 transit, const BMessage *message );
		uint32 KnobSize; 
		bool Pressed, Tracking;
};

#endif