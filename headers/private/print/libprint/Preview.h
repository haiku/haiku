/*
 * Preview.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

//#define USE_PREVIEW_FOR_DEBUG

#ifdef USE_PREVIEW_FOR_DEBUG
#ifndef __PREVIEW_H
#define __PREVIEW_H

#include <Window.h>

class PreviewWindow : public BWindow {
public:
	PreviewWindow(BRect, const char *, BBitmap *);
	virtual bool QuitRequested();
	int Go();

protected:
	PreviewWindow(const PreviewWindow &);
	PreviewWindow &operator = (const PreviewWindow &);

private:
	long __semaphore;
};

#endif	/* __PREVIEW_H */
#endif	/* USE_PREVIEW_FOR_DEBUG */
