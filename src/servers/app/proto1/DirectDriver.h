#ifndef _DIRECTDRIVER_H_
#define _DIRECTDRIVER_H_

#include <Application.h>
#include <WindowScreen.h>
#include <GraphicsCard.h>
#include <Message.h>
#include "DisplayDriver.h"

class OBAppServerScreen : public BWindowScreen
{
public:
	OBAppServerScreen(status_t *error);
	~OBAppServerScreen(void);
	virtual void MessageReceived(BMessage *msg);
	virtual bool QuitRequested(void);
	static int32 Draw(void *data);
	virtual void ScreenConnected(bool connected);

	// Graphics stuff implemented here for the moment
	void Clear(uint8 red,uint8 green,uint8 blue);
	void SimWin(BRect rect);

	int32 DrawID;
	bool redraw,connected;
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
	OBAppServerScreen *winscreen;
protected:
	// Original screen info
	uint32 old_space;
};

#endif