/**
 * @file MidiServerApp.h
 *
 * @author Matthijs Hollemans
 */

#ifndef MIDI_SERVER_APP_H
#define MIDI_SERVER_APP_H

#include <Application.h>

/**
 * The BApplication that drives the midi_server.
 */
class MidiServerApp : public BApplication
{
public:
	MidiServerApp();
	virtual void AboutRequested();
};

#endif // MIDI_SERVER_APP_H

