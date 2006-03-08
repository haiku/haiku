/*
	NodeHarnessWin.h

	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef NodeHarnessWin_H
#define NodeHarnessWin_H 1

#include <interface/Window.h>
#include <media/MediaNode.h>

class BButton;
class ToneProducer;

// A handy encapsulation of a Media Kit connection, including all
// information necessary to do any post-Connect() actions.
struct Connection
{
	media_node producer, consumer;
	media_source source;
	media_destination destination;
	media_format format;
};

// The window that runs our simple test application
class NodeHarnessWin : public BWindow
{
public:
	NodeHarnessWin(BRect frame, const char* title);
	~NodeHarnessWin();

	void Quit();
	void MessageReceived(BMessage* msg);

	ToneProducer* GetToneNode() const { return mToneNode; }

private:
	void StopNodes();

	BButton* mConnectButton;
	BButton* mStartButton;
	BButton* mStopButton;

	Connection mConnection;
	ToneProducer* mToneNode;
	bool mIsConnected;
	bool mIsRunning;
	media_node mTimeSource;
};

#endif
