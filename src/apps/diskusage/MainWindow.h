/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>.
 * Copyright (c) 2009 Philippe Saint-Pierre, stpere@gmail.com
 * All rights reserved. Distributed under the terms of the MIT license.
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
	virtual	bool				QuitRequested();

			void				ShowInfo(const FileInfo* info);
			void				SetRescanEnabled(bool enabled);
			BVolume*			FindDeviceFor(dev_t device,
									bool invoke = false);

private:
			ControlsView*		fControlsView;
			PieView*			fPieView;
			StatusView*			fStatusView;
};

#endif // MAIN_WINDOW_H
