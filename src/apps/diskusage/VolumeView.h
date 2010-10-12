/*
 * Copyright (c) 2010 Philippe Saint-Pierre, stpere@gmail.com
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 * Copyright (c) 1999 Mike Steed. You are free to use and distribute this software
 * as long as it is accompanied by it's documentation and this copyright notice.
 * The software comes with no warranty, etc.
 */

#ifndef VOLUMETAB_VIEW_H
#define VOLUMETAB_VIEW_H

#include <View.h>

class BButton;
class BPath;
class BStringView;
class BVolume;
class PieView;
class MainWindow;
class StatusView;

struct FileInfo;


class VolumeView: public BView {
	
public:
								VolumeView(const char* name, BVolume* volume);
	virtual						~VolumeView();

	virtual	void				MessageReceived(BMessage* message);
			void				SetRescanEnabled(bool enabled);
			void				SetPath(BPath path);
			void				ShowInfo(const FileInfo* info);

private:
	PieView*			fPieView;
	StatusView*			fStatusView;
};



#endif
