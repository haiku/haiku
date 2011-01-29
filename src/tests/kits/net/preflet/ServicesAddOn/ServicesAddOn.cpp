/*
 * Copyright 2004-2011 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ListView.h>
#include <ScrollView.h>
#include <String.h>

#include "ServicesAddOn.h"


NetworkSetupAddOn*
get_nth_addon(image_id image, int index)
{
	if (index != 0)
		return NULL;

	return new ServicesAddOn(image);
}


// #pragma mark -
ServicesAddOn::ServicesAddOn(image_id image)
	:
	NetworkSetupAddOn(image)
{
}


BView*
ServicesAddOn::CreateView(BRect* bounds)
{
	BRect r = *bounds;
	if (r.Width() < 100 || r.Height() < 100)
		r.Set(0, 0, 100, 100);

	BRect rlv = r.InsetByCopy(2, 2);
	rlv.right -= B_V_SCROLL_BAR_WIDTH;
	fServicesListView = new BListView(rlv, "system_services",
						B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES);

	BScrollView* sv = new BScrollView( "system_services_scrollview",
						fServicesListView, B_FOLLOW_ALL_SIDES,
						B_WILL_DRAW | B_FRAME_EVENTS, false, true);

	if (ParseInetd() == B_ERROR)
		ParseXinetd();

	*bounds = r;
	return sv;
}


status_t
ServicesAddOn::ParseInetd()
{
	FILE *f = fopen("/etc/inetd.conf", "r");
	if (f) {
		char line[1024], *l;
		char *token;

		while (fgets(line, sizeof(line), f)) {
			l = line;
			if (! *l)
				continue;

			if (line[0] == '#') {
				if (line[1] == ' ' || line[1] == '\t' ||
					line[1] == '\0' || line[1] == '\n' ||
					line[1] == '\r')
					// skip comments
					continue;
				l++;	// jump the disable/comment service mark
			}

			BString label;
			token = strtok(l, " \t");	// service name
			label << token;
			token = strtok(NULL, " \t");	// type
			label << " (" << token << ")";
			token = strtok(NULL, " \t");	// protocol
			label << " " << token;

			fServicesListView->AddItem(new BStringItem(label.String()));
		}
		fclose(f);
		return B_OK;
	} else
		return B_ERROR;
}


status_t
ServicesAddOn::ParseXinetd()
{
	FILE *f = fopen("/boot/common/settings/network/services", "r");
	if (f) {
		char line[1024], *l;
		char *token;
		char *loc;

		while (fgets(line, sizeof(line), f)) {
			l = line;

			if (! *l)
				continue;

			if (line[0] == '#' || line[0] == '\n')
				continue;
					// Skip commented out lines

			loc = strstr(l, "service ");

			if (loc) {
				BString label;
				token = strtok(loc, " ");
				token = strtok(NULL, " ");
				label << token;
				fServicesListView->AddItem(new BStringItem(label.String()));
			}

		}
		fclose(f);
		return B_OK;
	} else
		return B_ERROR;
}
