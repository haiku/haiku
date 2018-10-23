/*
 * Copyright 2017, Dario Casalinuovo
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "MediaStreamer.h"

#include <stdio.h>
#include <string.h>

#include "MediaDebug.h"

#include "PluginManager.h"


MediaStreamer::MediaStreamer(BUrl url)
	:
	fStreamer(NULL)
{
	CALLED();

	fUrl = url;
}


MediaStreamer::~MediaStreamer()
{
	CALLED();

	if (fStreamer != NULL)
		gPluginManager.DestroyStreamer(fStreamer);
}


status_t
MediaStreamer::CreateAdapter(BDataIO** adapter)
{
	CALLED();

	// NOTE: Consider splitting the streamer creation and
	// sniff in PluginManager.
	if (fStreamer != NULL)
		gPluginManager.DestroyStreamer(fStreamer);

	return gPluginManager.CreateStreamer(&fStreamer, fUrl, adapter);
}
