/*
 * Copyright 2003-2009, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		François Revol
 *		Marcus Overhagen
 *		Jonas Sundström
 */

//! VolumeControl and link items in Deskbar

#include "desklink.h"

#include <stdio.h>
#include <stdlib.h>

#include <Alert.h>
#include <Application.h>
#include <Deskbar.h>
#include <MimeType.h>
#include <Roster.h>
#include <String.h>

#include "DeskButton.h"
#include "VolumeWindow.h"


const char *kAppSignature = "application/x-vnd.Haiku-desklink";
	// the application signature used by the replicant to find the
	// supporting code


int
main(int, char **argv)
{	
	BApplication app(kAppSignature);
	bool atLeastOnePath = false;
	BList titleList;
	BList actionList;
	BDeskbar deskbar;
	status_t err = B_OK;

	for (int32 i = 1; argv[i]!=NULL; i++) {
		if (strcmp(argv[i], "--help") == 0)
			break;
		
		if (strcmp(argv[i], "--list") == 0) {
			int32 count = deskbar.CountItems();
			int32 found = 0;
			int32 j = 0;
			printf("Deskbar items:\n");

			while (found < count) {
				const char *name = NULL;
				if (deskbar.GetItemInfo(j, &name) == B_OK) {
					printf("Item %ld: '%s'\n", j, name);
					free((void *)name);
					found++;
				}
				j++;
			}
			return 0;
		}

		if (strcmp(argv[i], "--add-volume") == 0) {
			entry_ref ref;
			if (get_ref_for_path(argv[0], &ref) == B_OK) {
				deskbar.AddItem(&ref);
			}
			return 0;
		}

		if (strcmp(argv[i], "--volume-control") == 0) {
			BWindow* window = new VolumeWindow(BRect(200, 150, 400, 200));
			window->Show();

			wait_for_thread(window->Thread(), NULL);
			return 0;
		}

		if (strncmp(argv[i], "--remove", 8) == 0) {
			BString replicant = "DeskButton";
			if (strncmp(argv[i] + 8, "=", 1) == 0) {
				if (strlen(argv[i] + 9) > 0) {
					replicant = argv[i] + 9;
				} else {
					printf("desklink: Missing replicant name.\n");
					return 1;
				}
			}
			int32 found = 0;
			int32 found_id;
			while (deskbar.GetItemInfo(replicant.String(), &found_id) == B_OK) {
				err = deskbar.RemoveItem(found_id);
				if (err != B_OK) {
					printf("desklink: Error removing replicant id %ld: %s\n",
						found_id, strerror(err));
					break;
				}
				found++;
			}
			printf("Removed %ld items.\n", found);
			return err;
		}

		if (strncmp(argv[i], "cmd=", 4) == 0) {
			BString *title = new BString(argv[i] + 4);
			int32 index = title->FindFirst(':');
			if (index <= 0) {
				printf("desklink: usage: cmd=title:action\n");
			} else {
				title->Truncate(index);
				BString *action = new BString(argv[i] + 4);
				action->Remove(0, index+1);
				titleList.AddItem(title);
				actionList.AddItem(action);
			}
			continue;
		}

		atLeastOnePath = true;

		BEntry entry(argv[i], true);
		entry_ref ref;
		
		if (entry.Exists()) {
			entry.GetRef(&ref);
		} else if (BMimeType::IsValid(argv[i])) {
			if (be_roster->FindApp(argv[i], &ref) != B_OK) {
				printf("desklink: cannot find '%s'\n", argv[i]);
				return 1;
			}
		} else {
			printf("desklink: cannot find '%s'\n", argv[i]);
			return 1;
		}
		
		err = deskbar.AddItem(&ref);
		if (err != B_OK) {
			err = deskbar.AddItem(new DeskButton(BRect(0, 0, 15, 15),
				&ref, ref.name, titleList, actionList));
			if (err != B_OK) {
				printf("desklink: Deskbar refuses link to '%s': %s\n", argv[i], strerror(err));
				return 1;
			}
		}

		titleList.MakeEmpty();
		actionList.MakeEmpty();
	}

	if (!atLeastOnePath) {
		printf(	"usage: desklink { [ --list|--remove|[cmd=title:action ... ] [ path|signature ] } ...\n"
			"--add-volume: install volume control into Deskbar.\n"
			"--volume-control: show window with global volume control.\n"
			"--list: list all Deskbar addons.\n"
			"--remove: remove all desklink addons.\n"
			"--remove=name: remove all 'name' addons.\n");
		return 1;
	}

	return 0;
}
