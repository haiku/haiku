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


#include "NodeHarnessWin.h"
#include "ToneProducer.h"
#include <app/Application.h>
#include <interface/Button.h>
#include <storage/Entry.h>
#include <media/MediaRoster.h>
#include <media/MediaAddOn.h>
#include <media/TimeSource.h>
#include <media/MediaTheme.h>
#include <stdio.h>

const int32 BUTTON_CONNECT = 'Cnct';
const int32 BUTTON_START = 'Strt';
const int32 BUTTON_STOP = 'Stop';

#define TEST_WITH_AUDIO 1

// --------------------
// static utility functions
static void ErrorCheck(status_t err, const char* msg)
{
	if (err)
	{
		fprintf(stderr, "* FATAL ERROR (%s): %s\n", strerror(err), msg);
		exit(1);
	}
}

// --------------------
// NodeHarnessWin implementation
NodeHarnessWin::NodeHarnessWin(BRect frame, const char *title)
	:	BWindow(frame, title, B_TITLED_WINDOW, B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS),
		mToneNode(NULL), mIsConnected(false), mIsRunning(false)
{
	// build the UI
	BRect r(10, 10, 100, 40);
	mConnectButton = new BButton(r, "Connect", "Connect", new BMessage(BUTTON_CONNECT));
	mConnectButton->SetEnabled(true);
	AddChild(mConnectButton);
	r.OffsetBy(0, 40);
	mStartButton = new BButton(r, "Start", "Start", new BMessage(BUTTON_START));
	mStartButton->SetEnabled(false);
	AddChild(mStartButton);
	r.OffsetBy(0, 40);
	mStopButton = new BButton(r, "Stop", "Stop", new BMessage(BUTTON_STOP));
	mStopButton->SetEnabled(false);
	AddChild(mStopButton);

	// e.moon 2jun99: create the node
	BMediaRoster* roster = BMediaRoster::Roster();
	mToneNode = new ToneProducer();

	status_t err = roster->RegisterNode(mToneNode);
	ErrorCheck(err, "unable to register ToneProducer node!\n");
	// make sure the Media Roster knows that we're using the node
	roster->GetNodeFor(mToneNode->Node().node, &mConnection.producer);
}

NodeHarnessWin::~NodeHarnessWin()
{
	BMediaRoster* r = BMediaRoster::Roster();

	// tear down the node network
	if (mIsRunning) StopNodes();
	if (mIsConnected)
	{
		status_t err;

		// Ordinarily we'd stop *all* of the nodes in the chain at this point.  However,
		// one of the nodes is the System Mixer, and stopping the Mixer is a Bad Idea (tm).
		// So, we just disconnect from it, and release our references to the nodes that
		// we're using.  We *are* supposed to do that even for global nodes like the Mixer.
		err = r->Disconnect(mConnection.producer.node, mConnection.source,
			mConnection.consumer.node, mConnection.destination);
		if (err)
		{
			fprintf(stderr, "* Error disconnecting nodes:  %ld (%s)\n", err, strerror(err));
		}

		r->ReleaseNode(mConnection.producer);
		r->ReleaseNode(mConnection.consumer);
	}
}

void
NodeHarnessWin::Quit()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	BWindow::Quit();
}

void
NodeHarnessWin::MessageReceived(BMessage *msg)
{
	status_t err;

	switch (msg->what)
	{
	case BUTTON_CONNECT:
		mIsConnected = true;

		// set the button states appropriately
		mConnectButton->SetEnabled(false);
		mStartButton->SetEnabled(true);

		// set up the node network
		{
			BMediaRoster* r = BMediaRoster::Roster();

			// connect to the mixer
			err = r->GetAudioMixer(&mConnection.consumer);
			ErrorCheck(err, "unable to get the system mixer");

			// fire off a window with the ToneProducer's controls in it
			BParameterWeb* web;
			r->GetParameterWebFor(mConnection.producer, &web);
			BView* view = BMediaTheme::ViewFor(web);
			BWindow* win = new BWindow(BRect(250, 200, 300, 300), "Controls",
				B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS);
			win->AddChild(view);
			win->ResizeTo(view->Bounds().Width(), view->Bounds().Height());
			win->Show();

			// set the producer's time source to be the "default" time source, which
			// the Mixer uses too.
			r->GetTimeSource(&mTimeSource);
			r->SetTimeSourceFor(mConnection.producer.node, mTimeSource.node);

			// got the nodes; now we find the endpoints of the connection
			media_input mixerInput;
			media_output soundOutput;
			int32 count = 1;
			err = r->GetFreeOutputsFor(mConnection.producer, &soundOutput, 1, &count);
			ErrorCheck(err, "unable to get a free output from the producer node");
			count = 1;
			err = r->GetFreeInputsFor(mConnection.consumer, &mixerInput, 1, &count);
			ErrorCheck(err, "unable to get a free input to the mixer");

			// got the endpoints; now we connect it!
			media_format format;
			format.type = B_MEDIA_RAW_AUDIO;
			format.u.raw_audio = media_raw_audio_format::wildcard;
			err = r->Connect(soundOutput.source, mixerInput.destination, &format, &soundOutput, &mixerInput);
			ErrorCheck(err, "unable to connect nodes");

			// the inputs and outputs might have been reassigned during the
			// nodes' negotiation of the Connect().  That's why we wait until
			// after Connect() finishes to save their contents.
			mConnection.format = format;
			mConnection.source = soundOutput.source;
			mConnection.destination = mixerInput.destination;

			// Set an appropriate run mode for the producer
			r->SetRunModeNode(mConnection.producer, BMediaNode::B_INCREASE_LATENCY);
		}
		break;

	case BUTTON_START:
		mStartButton->SetEnabled(false);
		mStopButton->SetEnabled(true);

		// start the producer running
		{
			BMediaRoster* r = BMediaRoster::Roster();
			BTimeSource* ts = r->MakeTimeSourceFor(mConnection.producer);
			if (!ts)
			{
				fprintf(stderr, "* ERROR - MakeTimeSourceFor(producer) returned NULL!\n");
				exit(1);
			}

			// make sure we give the producer enough time to run buffers through
			// the node chain, otherwise it'll start up already late
			bigtime_t latency = 0;
			r->GetLatencyFor(mConnection.producer, &latency);
			r->StartNode(mConnection.producer, ts->Now() + latency);
			ts->Release();
			mIsRunning = true;
		}
		break;

	case BUTTON_STOP:
		StopNodes();
		break;

	default:
		BWindow::MessageReceived(msg);
		break;
	}
}

// Private routines
void
NodeHarnessWin::StopNodes()
{
	mStartButton->SetEnabled(true);
	mStopButton->SetEnabled(false);

	// generally, one only stops the initial producer(s), and lets the rest of the node
	// chain just follow along as the buffers drain out.
	{
		BMediaRoster* r = BMediaRoster::Roster();
		r->StopNode(mConnection.producer, 0, true);		// synchronous stop
	}
}
