/*
 * Copyright 2017, Dario Casalinuovo
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "MediaStreamer.h"

#include <CodecRoster.h>

#include <stdio.h>
#include <string.h>

#include "MediaDebug.h"
#include "PluginManager.h"


namespace BCodecKit {


BMediaStreamer::BMediaStreamer(BUrl url)
	:
	fUrl(url),
	fStreamer(NULL),
	fInitCheck(B_OK),
	fOpened(false)
{
	// TODO: ideally we should see if the url IsValid() and
	// matches our protocol.
	CALLED();
}


BMediaStreamer::~BMediaStreamer()
{
	CALLED();

	if (fOpened)
		Close();
}


status_t
BMediaStreamer::InitCheck() const
{
	return fInitCheck;
}


status_t
BMediaStreamer::Open()
{
	CALLED();

	fOpened = true;

	// NOTE: Consider splitting the streamer creation and
	// sniff in PluginManager.
	if (fStreamer != NULL)
		BCodecRoster::ReleaseStreamer(fStreamer);

	return BCodecRoster::InstantiateStreamer(&fStreamer, fUrl);
}


void
BMediaStreamer::Close()
{
	if (fStreamer != NULL)
		BCodecRoster::ReleaseStreamer(fStreamer);
}


bool
BMediaStreamer::IsOpened() const
{
	return fOpened;
}


BMediaIO*
BMediaStreamer::Adapter() const
{
	return fStreamer->Adapter();
}


void
BMediaStreamer::MouseMoved(uint32 x, uint32 y)
{
	fStreamer->MouseMoved(x, y);
}


void
BMediaStreamer::MouseDown(uint32 x, uint32 y)
{
	fStreamer->MouseDown(x, y);
}


} // namespace BCodecKit
