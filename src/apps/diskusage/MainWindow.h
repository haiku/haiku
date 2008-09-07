/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT/X11 license.
 *
 * Copyright (c) 1999 Mike Steed. You are free to use and distribute this software
 * as long as it is accompanied by it's documentation and this copyright notice.
 * The software comes with no warranty, etc.
 */
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <Window.h>


class BVolume;
class ControlsView;
struct FileInfo;
class PieView;
class StatusView;

class MainWindow: public BWindow {
public:
								MainWindow(BRect pieRect);
	virtual						~MainWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				Zoom(BPoint origin, float width, float height);
	virtual	void				FrameResized(float width, float height);
	virtual	bool				QuitRequested();

			void				ShowInfo(const FileInfo* info);
			void				SetRescanEnabled(bool enabled);
			BVolume*			FindDeviceFor(dev_t device,
									bool invoke = false);

private:
			bool				_FixAspectRatio(float* width, float* height);

			ControlsView*		fControlsView;
			PieView*			fPieView;
			StatusView*			fStatusView;
};

#endif // MAIN_WINDOW_H
