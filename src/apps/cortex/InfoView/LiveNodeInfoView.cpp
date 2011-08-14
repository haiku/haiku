/*
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


// LiveNodeInfoView.cpp

#include "LiveNodeInfoView.h"
// InfoView
#include "InfoWindowManager.h"
// NodeManager
#include "NodeGroup.h"
#include "NodeRef.h"
// Support
#include "MediaIcon.h"
#include "MediaString.h"

// Media Kit
#include <MediaNode.h>

// Interface Kit
#include <Window.h>

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_METHOD(x) //PRINT (x)
#define D_MESSAGE(x) //PRINT (x)

// -------------------------------------------------------- //
// *** ctor/dtor (public)
// -------------------------------------------------------- //

LiveNodeInfoView::LiveNodeInfoView(
	const NodeRef *ref)
	: InfoView(ref->name(), "Live Media Node",
			   new MediaIcon(ref->nodeInfo(), B_LARGE_ICON)),
	  m_nodeID(ref->id())
{
	D_METHOD(("LiveNodeInfoView::LiveNodeInfoView()\n"));

	// adjust view properties
	setSideBarWidth(be_plain_font->StringWidth(" Run Mode ") + 2 * InfoView::M_H_MARGIN);

	// add "Node ID" field
	BString s;
	s << ref->id();
	addField("Node ID", s);

	// add "Port" field
	s = "";
	s << ref->node().port;
	port_info portInfo;
	if (get_port_info(ref->node().port, &portInfo) == B_OK)
	{
		s << " (" << portInfo.name << ")";
	}
	addField("Port", s);

	// add separator field
	addField("", "");

	// add "Kinds" field
	addField("Kinds", MediaString::getStringFor(static_cast<node_kind>(ref->kind())));

	// add "Run Mode" field
	BMediaNode::run_mode runMode = static_cast<BMediaNode::run_mode>(ref->runMode());
	if (runMode < 1)
	{
		NodeGroup *group = ref->group();
		if (group)
		{
			runMode = group->runMode();
		}
	}
	addField("Run Mode", MediaString::getStringFor(runMode));

	// add "Latency" field
	bigtime_t latency;
	if (ref->totalLatency(&latency) == B_OK)
	{
		s = "";
		if (latency > 0)
		{
			s << static_cast<float>(latency) / 1000.0f << " ms";
		}
		else
		{
			s = "?";
		}
		addField("Latency", s);
	}
}

LiveNodeInfoView::~LiveNodeInfoView() {
	D_METHOD(("LiveNodeInfoView::~LiveNodeInfoView()\n"));

}

// -------------------------------------------------------- //
// *** BView implementation (public)
// -------------------------------------------------------- //

void LiveNodeInfoView::DetachedFromWindow() {
	D_METHOD(("LiveNodeInfoView::DetachedFromWindow()\n"));

	InfoWindowManager *manager = InfoWindowManager::Instance();
	if (manager) {
		BMessage message(InfoWindowManager::M_LIVE_NODE_WINDOW_CLOSED);
		message.AddInt32("nodeID", m_nodeID);
		manager->PostMessage(&message);
	}
}

// END -- LiveNodeInfoView.cpp --
