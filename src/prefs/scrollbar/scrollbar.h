#ifndef SCROLLBAR_H
#define SCROLLBAR_H

#include <Application.h>

#define MAXIMUM_KNOB_SIZE	50
#define MINIMUM_KNOB_SIZE	9

typedef struct
{
	uint32 ArrowStyle;
	uint32 KnobType;
	uint32 KnobStyle;
	uint32 KnobSize;
	
} scrollbar_settings;

class ScrollBarApp : public BApplication
{
	public:
		ScrollBarApp();
	private:
		void MessageReceived( BMessage *message );
};

extern scrollbar_settings Settings;
extern scrollbar_settings RevertSettings;

#endif