/**
 * @file MidiServerApp.h
 *
 * @author Matthijs Hollemans
 * @author Jerome Leveque
 */

#ifndef MIDI_SERVER_APP_H
#define MIDI_SERVER_APP_H

#include <Application.h>

#define MIDI_SERVER_SIGNATURE  "application/x-vnd.OpenBeOS-midi-server"

/**
 * The BApplication that drives the midi_server.
 */
class MidiServerApp : public BApplication
{
public:
	MidiServerApp();
	virtual ~MidiServerApp();

	virtual void AboutRequested();
	virtual void MessageReceived(BMessage* msg);

	//int32 GetNextFreeID();
	//BMidiEndpoint* NextEndPoint(int32* id);
	//BMidiRoster* GetRoster();

private:

	/** Our superclass. */
	typedef BApplication super;

	//BList* endpoints;
	//int32 nextFreeID;
	//BMidiRoster* roster;
};

#endif // MIDI_SERVER_APP_H

