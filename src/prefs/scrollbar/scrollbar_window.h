#ifndef SCROLLBAR_WINDOW_H
#define SCROLLBAR_WINDOW_H

#include <Window.h>
#include <Box.h>
#include <Button.h>
#include "fakescrollbar.h"
#include "knobsizeadjuster.h"

class ScrollBarWindow : public BWindow
{
	public:
		ScrollBarWindow();
	private:
		bool QuitRequested();
		void MessageReceived( BMessage *message );
		
		void LoadSettings( BMessage *message );
		void SaveSettings();
		
		FakeScrollBar *DoubleArrowView;
		FakeScrollBar *SingleArrowView;
		FakeScrollBar *ProportionalKnobView;
		FakeScrollBar *FixedKnobView;
		FakeScrollBar *FlatKnobView;
		FakeScrollBar *DotKnobView;
		FakeScrollBar *LineKnobView;	
		KnobSizeAdjuster *MinimumKnobSize;	
		BBox* MainBox;
		BButton* RevertButton;
};

#endif