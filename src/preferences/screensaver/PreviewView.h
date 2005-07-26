/*
 * Copyright 2003, Michael Phipps. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef PREVIEWVIEW_H
#define PREVIEWVIEW_H

#include <View.h>
#include <Box.h>
#include <ScreenSaverThread.h>
#include <ScreenSaverPrefs.h>

class BScreenSaver;

class PreviewView : public BView
{ 
public:	
	PreviewView(BRect frame, const char *name,ScreenSaverPrefs *prefp);
	~PreviewView();
	void Draw(BRect update); 
	void SetScreenSaver(BString name);
	BScreenSaver *ScreenSaver() {return fSaver;}	
private:	
	BScreenSaver* fSaver;
	BView *fConfigView;
	ScreenSaverThread *fSst;
	thread_id fThreadID;
	ScreenSaverPrefs *fPrefPtr;
	
}; // end class PreviewView

#endif // PREVIEWVIEW_H
