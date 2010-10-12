/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT/X11 license.
 *
 * Copyright (c) 1999 Mike Steed. You are free to use and distribute this software
 * as long as it is accompanied by it's documentation and this copyright notice.
 * The software comes with no warranty, etc.
 */
#ifndef CONTROLS_VIEW_H
#define CONTROLS_VIEW_H

#include <Button.h>
#include <View.h>


class BMessage;
class BVolume;

struct FileInfo;

class ControlsView: public BView {
public:
								ControlsView();
	virtual						~ControlsView();

	virtual	void				MessageReceived(BMessage* msg);

			BVolume*			FindDeviceFor(dev_t device,
									bool invoke = false);

			void				SetRescanEnabled(bool enabled);
			void				ShowInfo(const FileInfo* info);

private:
			class 				VolumeTabView;

			VolumeTabView*		fVolumeTabView;
};


#endif // CONTROLS_VIEW_H
