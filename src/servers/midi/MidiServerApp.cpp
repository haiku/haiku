/**
 * @file MidiServerApp.cpp
 *
 * @author Matthijs Hollemans
 * @author Jerome Leveque
 */

#include <Alert.h>

#include "MidiServerApp.h"

//------------------------------------------------------------------------------

MidiServerApp::MidiServerApp()
	: BApplication(MIDI_SERVER_SIGNATURE)
{
	//roster = new BMidiRoster();
}

//------------------------------------------------------------------------------

MidiServerApp::~MidiServerApp(void)
{
	//delete endpoints;
}

//------------------------------------------------------------------------------

void MidiServerApp::AboutRequested()
{
	(new BAlert(0,
		"-write something funny here-",
		"Okay", 0, 0, B_WIDTH_AS_USUAL, 
		B_INFO_ALERT))->Go();
}

//------------------------------------------------------------------------------

void MidiServerApp::MessageReceived(BMessage* msg)
{
	switch (msg->what)
	{
		default: super::MessageReceived(msg); break;
	}
}

//------------------------------------------------------------------------------

/*int32 MidiServerApp::GetNextFreeID()
{
	return nextFreeID++;
}*/

//------------------------------------------------------------------------------

/*BMidiEndpoint* MidiServerApp::NextEndPoint(int32* id)
{
	int32 item = 0;
	BMidiEndpoint *endpoint;

	while ((endpoint = (BMidiEndpoint*) endpoints->ItemAt(item)) != NULL)
	{
		if (endpoint->ID() > *id)
		{
			endpoint->Acquire();
			*id = endpoint->ID();
			return endpoint;
		}
		item++;
	}
	return NULL;
}*/

//------------------------------------------------------------------------------

/*BMidiRoster* MidiServerApp::GetRoster()
{
	return roster;
}*/

//------------------------------------------------------------------------------

int main()
{
	MidiServerApp app;
	app.Run();
	return 0;
}

//------------------------------------------------------------------------------

