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


class PulseApp : public BApplication {
public:
						PulseApp(int argc, char **argv);
						~PulseApp();

	virtual	void		AboutRequested();
	static	void		ShowAbout(bool asApplication);

			Prefs*		prefs;

private:
			void		BuildPulse();
};

extern bool LastEnabledCPU(int cpu);
extern int GetMinimumViewWidth();
extern bool LoadInDeskbar();
extern void Usage();

#endif	// PULSEAPP_H
