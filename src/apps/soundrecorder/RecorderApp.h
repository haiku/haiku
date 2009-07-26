/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Inspired by SoundCapture from Be newsletter (Media Kit Basics: Consumers and Producers)
 */

#ifndef RECORDERAPP_H
#define RECORDERAPP_H

#include <Application.h>

class RecorderWindow;

class RecorderApp : public BApplication {
	public:
				RecorderApp(const char * signature);
		virtual	~RecorderApp();
				status_t InitCheck(); 
	private:	
		RecorderWindow* fRecorderWin;
};

#endif	/* RECORDERAPP_H */
