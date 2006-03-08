// DormantNodeInfoView.cpp

#include "DormantNodeInfoView.h"
// InfoView
#include "InfoWindowManager.h"
// Support
#include "MediaIcon.h"
#include "MediaString.h"

// Media Kit
#include <MediaAddOn.h>
#include <MediaRoster.h>

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_METHOD(x) //PRINT (x)

// -------------------------------------------------------- //
// *** ctor/dtor (public)
// -------------------------------------------------------- //

DormantNodeInfoView::DormantNodeInfoView(
	const dormant_node_info &info)
	: InfoView(info.name, "Dormant Media Node",
			   new MediaIcon(info, B_LARGE_ICON)),
	  m_addOnID(info.addon),
	  m_flavorID(info.flavor_id)
{
	D_METHOD(("DormantNodeInfoView::DormantNodeInfoView()\n"));

	// adjust view properties
	setSideBarWidth(be_plain_font->StringWidth(" Output Formats ") + 2 * InfoView::M_H_MARGIN);

	BString s;

	// add the "AddOn ID" field
	s = "";
	s << info.addon;
	addField("AddOn ID", s);

	// add the "Flavor ID" field
	s = "";
	s << info.flavor_id;
	addField("Flavor ID", s);
	
	// add separator field
	addField("", "");

	dormant_flavor_info flavorInfo;
	BMediaRoster *roster = BMediaRoster::Roster();
	if (roster && (roster->GetDormantFlavorInfoFor(info, &flavorInfo) == B_OK))
	{
		// add the "Description" field
		s = flavorInfo.info;
		addField("Description", s);

		// add "Kinds" field
		addField("Kinds", MediaString::getStringFor(static_cast<node_kind>(flavorInfo.kinds)));

		// add "Flavor Flags" field
		addField("Flavor Flags", "?");

		// add "Max Instances" field
		if (flavorInfo.possible_count > 0)
		{
			s = "";
			s << flavorInfo.possible_count;
		}
		else
		{
			s = "Any number";
		}
		addField("Max Instances", s);

		// add "Input Formats" field
		if (flavorInfo.in_format_count > 0)
		{
			if (flavorInfo.in_format_count == 1)
			{
				addField("Input Format", MediaString::getStringFor(flavorInfo.in_formats[0], false));
			}
			else
			{
				addField("Input Formats", "");
				for (int32 i = 0; i < flavorInfo.in_format_count; i++)
				{
					s = "";
					s << "(" << i + 1 << ")";
					addField(s, MediaString::getStringFor(flavorInfo.in_formats[i], false));
				}
			}
		}
		
		// add "Output Formats" field
		if (flavorInfo.out_format_count > 0)
		{
			if (flavorInfo.out_format_count == 1)
			{
				addField("Output Format", MediaString::getStringFor(flavorInfo.out_formats[0], false));
			}
			else
			{
				addField("Output Formats", "");
				for (int32 i = 0; i < flavorInfo.out_format_count; i++)
				{
					s = "";
					s << "(" << i + 1 << ")";
					addField(s, MediaString::getStringFor(flavorInfo.out_formats[i], false));
				}
			}
		}
	}
}

DormantNodeInfoView::~DormantNodeInfoView()
{
	D_METHOD(("DormantNodeInfoView::~DormantNodeInfoView()\n"));
}

// -------------------------------------------------------- //
// *** BView implementation (public)
// -------------------------------------------------------- //

void DormantNodeInfoView::DetachedFromWindow() {
	D_METHOD(("DormantNodeInfoView::DetachedFromWindow()\n"));

	InfoWindowManager *manager = InfoWindowManager::Instance();
	if (manager) {
		BMessage message(InfoWindowManager::M_DORMANT_NODE_WINDOW_CLOSED);
		message.AddInt32("addOnID", m_addOnID);
		message.AddInt32("flavorID", m_flavorID);
		manager->PostMessage(&message);
	}
}

// END -- DormantNodeInfoView.cpp --
