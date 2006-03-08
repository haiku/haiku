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
