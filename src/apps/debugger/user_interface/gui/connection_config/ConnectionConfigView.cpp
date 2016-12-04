/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "ConnectionConfigView.h"


ConnectionConfigView::ConnectionConfigView(const char* name)
	:
	BView(name, B_WILL_DRAW)
{
}


ConnectionConfigView::~ConnectionConfigView()
{
}


status_t
ConnectionConfigView::Init(TargetHostInterfaceInfo* info, Listener* listener)
{
	fInfo = info;
	fListener = listener;

	return InitSpecific();
}


void
ConnectionConfigView::NotifyConfigurationChanged(Settings* settings)
{
	fListener->ConfigurationChanged(settings);
}


// #pragma mark - ConnectionConfigView::Listener


ConnectionConfigView::Listener::~Listener()
{
}
