#ifndef SCROLLBAR_FAKE_H
#define SCROLLBAR_FAKE_H

#include <View.h>


class FakeScrollBar : public BView
{
	public:
		FakeScrollBar( BPoint position, bool selected, uint32 style, 
			uint32 type, uint32 arrow, uint32 size, BMessage *message );
		
		void SetKnobStyle( uint32 style );
		void SetKnobType( uint32 type );
		void SetArrowStyle( uint32 arrow );	
		void SetKnobSize( uint32 size );	
		void SetSelected( bool selected );	
		void SetPressed( bool pressed );
		void SetTracking( bool tracking );
		bool IsPressed();
		bool IsSelected();
		bool IsTracking();
	private:
		void Draw( BRect updateRect );
		void MouseDown( BPoint point );
		void MouseUp( BPoint point );
		void MouseMoved( BPoint point, uint32 transit, const BMessage *message );
		void DrawArrowBox( uint32 x, bool facingleft );
		void DrawDotGrabber( uint32 x, uint32 size );
		void DrawLineGrabber( uint32 x, uint32 size );
		BMessage *Message;
		bool Selected, Pressed, Tracking;
		uint32 ArrowStyle,KnobType,KnobStyle,KnobSize; 
};

#endif