#ifndef _GFX_DRIVER_H_
#define _GFX_DRIVER_H_

#include <GraphicsCard.h>
#include "Desktop.h"

typedef struct
{
	uchar *xormask, *andmask;
	int32 width, height;
	int32 hotx, hoty;

} cursor_data;

#ifndef HOOK_DEFINE_CURSOR

#define HOOK_DEFINE_CURSOR		0
#define HOOK_MOVE_CURSOR		1
#define HOOK_SHOW_CURSOR		2
#define HOOK_DRAW_LINE_8BIT		3
#define HOOK_DRAW_LINE_16BIT	12
#define HOOK_DRAW_LINE_32BIT	4
#define HOOK_DRAW_RECT_8BIT		5
#define HOOK_DRAW_RECT_16BIT	13
#define HOOK_DRAW_RECT_32BIT	6
#define HOOK_BLIT				7
#define HOOK_DRAW_ARRAY_8BIT	8
#define HOOK_DRAW_ARRAY_16BIT	14	// Not implemented in current R5 drivers
#define HOOK_DRAW_ARRAY_32BIT	9
#define HOOK_SYNC				10
#define HOOK_INVERT_RECT		11

#endif

class DisplayDriver
{
public:
	DisplayDriver(void);
	virtual ~DisplayDriver(void);

	virtual void Initialize(void);		// Sets the driver
	virtual void SafeMode(void);	// Easy-access functions for common tasks
	virtual void Reset(void);
	virtual void SetScreen(uint32 space);

	virtual bool IsInitialized(void);
	graphics_card_hook ghooks[48];
	graphics_card_info *ginfo;
	cursor_data current_cursor;
	bool cursor_visible;

protected:
	bool is_initialized;
};

#endif