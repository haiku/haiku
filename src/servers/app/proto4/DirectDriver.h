#ifndef _DIRECTDRIVER_H_
#define _DIRECTDRIVER_H_

#include <Application.h>
#include <DirectWindow.h>
#include <GraphicsCard.h>
#include <Message.h>
#include "DisplayDriver.h"

class OBAppServerScreen : public BDirectWindow
{
public:
	OBAppServerScreen(status_t *error);
	~OBAppServerScreen(void);
	virtual void MessageReceived(BMessage *msg);
	virtual bool QuitRequested(void);
	static int32 Draw(void *data);
	virtual void DirectConnected(direct_buffer_info *dbinfo);

	int32 DrawID;
	bool redraw,connected;
	graphics_card_info *gcinfo;
	graphics_card_hook *gchooks;
};

class DirectDriver : public DisplayDriver
{
public:
	DirectDriver(void);
	virtual ~DirectDriver(void);

	virtual void Initialize(void);		// Sets the driver
	virtual void SafeMode(void);	// Easy-access functions for common tasks
	virtual void Reset(void);
	virtual void SetScreen(uint32 space);
	virtual void Clear(uint8 red,uint8 green,uint8 blue);
//	virtual void SetPixel(int x, int y, rgb_color color);
//	virtual void StrokeEllipse(BRect rect, rgb_color color);
	OBAppServerScreen *winscreen;

protected:
	// Original screen info
	uint32 old_space;
};

#endif