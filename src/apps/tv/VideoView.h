/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef __VIDEO_VIEW_H
#define __VIDEO_VIEW_H


#include <View.h>

class BMessageRunner;
class VideoNode;


class VideoView : public BView {
public:
					VideoView(BRect frame, const char *name, uint32 resizeMask,
						uint32 flags);
					~VideoView();

	void			RemoveVideoDisplay();
	void			RemoveOverlay();

	VideoNode *		Node();

	bool			IsOverlaySupported();

	void			OverlayLockAcquire();
	void			OverlayLockRelease();

	void			OverlayScreenshotPrepare();
	void			OverlayScreenshotCleanup();

	void			DrawFrame();

private:
	void			AttachedToWindow();
	void			MessageReceived(BMessage *msg);
	void			Draw(BRect updateRect);

	void			DrawTestImage();
	void			CheckActivity();

private:
	VideoNode *		fVideoNode;
	bool			fVideoActive;
	bool			fOverlayActive;
	rgb_color		fOverlayKeyColor;
	BMessageRunner *fActivityCheckMsgRunner;
	bigtime_t		fLastFrame;
};

#endif	// __VIDEO_VIEW_H
