/*
 * Copyright 1999, Be Incorporated.
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
