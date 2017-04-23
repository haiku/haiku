/*
 * Copyright 2005, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#include "PPPStatusView.h"

#include <Box.h>
#include <Button.h>
#include <StringView.h>
#include <Window.h>

#include <cstdio>
#include <String.h>

#include <PPPManager.h>


// message constants
static const uint32 kMsgDisconnect = 'DISC';

// labels
static const char *kLabelDisconnect = "Disconnect";
static const char *kLabelConnectedSince = "Connected since: ";
static const char *kLabelReceived = "Received";
static const char *kLabelSent = "Sent";

// strings
static const char *kTextBytes = "Bytes";
static const char *kTextPackets = "Packets";


PPPStatusView::PPPStatusView(BRect rect, ppp_interface_id id)
	: BView(rect, "PPPStatusView", B_FOLLOW_NONE, B_PULSE_NEEDED),
	fInterface(id)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	
	rect = Bounds();
	rect.InsetBy(5, 5);
	rect.left = rect.right - 80;
	rect.bottom = rect.top + 25;
	fButton = new BButton(rect, "DisconnectButton", kLabelDisconnect,
		new BMessage(kMsgDisconnect));
	
	rect.right = rect.left - 10;
	rect.left = rect.right - 80;
	rect.top += 5;
	rect.bottom = rect.top + 15;
	fTime = new BStringView(rect, "Time", "");
	fTime->SetAlignment(B_ALIGN_RIGHT);
	fTime->SetFont(be_fixed_font);
	rect.right = rect.left - 10;
	rect.left = 5;
	BStringView *connectedSince = new BStringView(rect, "ConnectedSince",
		kLabelConnectedSince);
	connectedSince->SetFont(be_fixed_font);
	
	rect = Bounds();
	rect.InsetBy(5, 5);
	rect.top += 35;
	rect.right = rect.left + (rect.Width() - 5) / 2;
	BBox *received = new BBox(rect, "Received");
	received->SetLabel(kLabelReceived);
	rect = received->Bounds();
	rect.InsetBy(10, 15);
	rect.bottom = rect.top + 15;
	fBytesReceived = new BStringView(rect, "BytesReceived", "");
	fBytesReceived->SetAlignment(B_ALIGN_RIGHT);
	fBytesReceived->SetFont(be_fixed_font);
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 15;
	fPacketsReceived = new BStringView(rect, "PacketsReceived", "");
	fPacketsReceived->SetAlignment(B_ALIGN_RIGHT);
	fPacketsReceived->SetFont(be_fixed_font);
	
	rect = received->Frame();
	rect.OffsetBy(rect.Width() + 5, 0);
	BBox *sent = new BBox(rect, "sent");
	sent->SetLabel(kLabelSent);
	rect = received->Bounds();
	rect.InsetBy(10, 15);
	rect.bottom = rect.top + 15;
	fBytesSent = new BStringView(rect, "BytesSent", "");
	fBytesSent->SetAlignment(B_ALIGN_RIGHT);
	fBytesSent->SetFont(be_fixed_font);
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 15;
	fPacketsSent = new BStringView(rect, "PacketsSent", "");
	fPacketsSent->SetAlignment(B_ALIGN_RIGHT);
	fPacketsSent->SetFont(be_fixed_font);
	
	received->AddChild(fBytesReceived);
	received->AddChild(fPacketsReceived);
	sent->AddChild(fBytesSent);
	sent->AddChild(fPacketsSent);
	
	AddChild(fButton);
	AddChild(fTime);
	AddChild(connectedSince);
	AddChild(received);
	AddChild(sent);
	
	ppp_interface_info_t info;
	fInterface.GetInterfaceInfo(&info);
	fConnectedSince = info.info.connectedSince;
}


void
PPPStatusView::AttachedToWindow()
{
	fButton->SetTarget(this);
	Window()->SetTitle(fInterface.Name());
}


void
PPPStatusView::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case kMsgDisconnect:
			fInterface.Down();
			Window()->Hide();
		break;
		
		default:
			BView::MessageReceived(message);
	}
}


void
PPPStatusView::Pulse()
{
	// update status
	ppp_statistics statistics;
	if(!fInterface.GetStatistics(&statistics)) {
		fBytesReceived->SetText("");
		fPacketsReceived->SetText("");
		fBytesSent->SetText("");
		fPacketsSent->SetText("");
		return;
	}
	
	BString text;
	bigtime_t time = system_time() - fConnectedSince;
	time /= 1000000;
	int32 seconds = time % 60;
	time /= 60;
	int32 minutes = time % 60;
	int32 hours = time / 60;
	char minsec[7];
	if(hours) {
		sprintf(minsec, ":%02" B_PRId32 ":%02" B_PRId32, minutes, seconds);
		text << hours << minsec;
	} else if(minutes) {
		sprintf(minsec, "%" B_PRId32 ":%02" B_PRId32, minutes, seconds);
		text << minsec;
	} else
		text << seconds;
	fTime->SetText(text.String());
	
	text = "";
	text << statistics.bytesReceived << ' ' << kTextBytes;
	fBytesReceived->SetText(text.String());
	text = "";
	text << statistics.packetsReceived << ' ' << kTextPackets;
	fPacketsReceived->SetText(text.String());
	text = "";
	text << statistics.bytesSent << ' ' << kTextBytes;
	fBytesSent->SetText(text.String());
	text = "";
	text << statistics.packetsSent << ' ' << kTextPackets;
	fPacketsSent->SetText(text.String());
}
