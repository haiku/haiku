//****************************************************************************************
//
//	File:		PulseApp.h
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#ifndef PULSEAPP_H
#define PULSEAPP_H


#include <Application.h>
#include <Catalog.h>

#include "Prefs.h"
#include "PrefsWindow.h"


class PulseApp : public BApplication {
public:
							PulseApp(int argc, char **argv);
							~PulseApp();

	virtual void			MessageReceived(BMessage* message);
	virtual void			ReadyToRun();
	virtual	void			AboutRequested();

private:
			void			BuildPulse();

public:
			Prefs*			fPrefs;

private:
			bool			fRunFromReplicant;
			bool			fIsRunning;
			PrefsWindow*	fPrefsWindow;
};

extern bool LastEnabledCPU(unsigned int cpu);
extern int GetMinimumViewWidth();
extern bool LoadInDeskbar();
extern void Usage();

#endif	// PULSEAPP_H
