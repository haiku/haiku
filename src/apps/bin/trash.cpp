/*
 * [un]trash command for Haiku
 * Copyright (c) 2004, Francois Revol - revol@free.fr
 * provided under the MIT licence
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <app/Message.h>
#include <app/Messenger.h>
#include <kernel/fs_attr.h>
#include <kernel/fs_info.h>
#include <storage/Directory.h>
#include <storage/Entry.h>
#include <storage/FindDirectory.h>
#include <storage/Node.h>
#include <storage/Path.h>
#include <support/TypeConstants.h>

static const char *kAttrOriginalPath = "_trk/original_path";
static const char *kTrackerSig = "application/x-vnd.Be-TRAK";

int usage(int ret)
{
	printf("\nSend files to trash, or restore them.\nUsage:\n");
	printf("trash [--empty] file ...\n");
	printf("\t--empty\tempty the Trash\n");
	printf("untrash [--all] [file ...]\n");
	//printf("restore [--all] [file ...]\n");
	return ret;
}

status_t untrash(const char *f)
{
	status_t err;
	char original_path[B_PATH_NAME_LENGTH];
	BPath path(f);
	BNode node(f);
	err = node.InitCheck();
	if (err)
		return err;
	err = node.ReadAttr(kAttrOriginalPath, B_STRING_TYPE, 0LL, original_path, B_PATH_NAME_LENGTH);
	if (err < 0)
		return err;
	err = rename(path.Path(), original_path);
	if (err < 0)
		return err;
	node.RemoveAttr(kAttrOriginalPath);
	return 0;
}

status_t trash(const char *f)
{
	status_t err;
	attr_info ai;
	dev_t dev = -1;
	int nr;
	const char *original_path;
	char trash_dir[B_PATH_NAME_LENGTH];
	char trashed_file[B_PATH_NAME_LENGTH];
	dev = dev_for_path(f);
	err = find_directory(B_TRASH_DIRECTORY, dev, false, trash_dir, B_PATH_NAME_LENGTH);
	if (err < 0)
		return err;
	BNode node(f);
	err = node.InitCheck();
	if (err < 0)
		return err;
	err = node.GetAttrInfo(kAttrOriginalPath, &ai);
	if (err == B_OK)
		return EALREADY;
	if (!strncmp(f, trash_dir, strlen(trash_dir)))
		return EALREADY;
	entry_ref er;
	err = get_ref_for_path(f, &er);
	BPath orgPath(&er);
	err = orgPath.InitCheck();
	if (err < 0)
		return err;
	original_path = orgPath.Path();
	BDirectory trashDir(trash_dir);
	err = trashDir.InitCheck();
	if (err < 0)
		return err;
	for (nr = 0; ; nr++) {
		if (nr > INT_MAX - 1)
			return B_ERROR;
		if (nr)
			snprintf(trashed_file, B_PATH_NAME_LENGTH-1, "%s/%s %d", trash_dir, er.name, nr);
		else
			snprintf(trashed_file, B_PATH_NAME_LENGTH-1, "%s/%s", trash_dir, er.name);
		if (!trashDir.Contains(trashed_file))
			break;
	}
	err = rename(original_path, trashed_file);
	if (err < 0)
		return err;
	
	err = node.WriteAttr(kAttrOriginalPath, B_STRING_TYPE, 0LL, original_path, strlen(original_path)+1);
	if (err < 0)
		return err;
	return 0;
}

int main(int argc, char **argv)
{
	int dountrash = 0;
	int i;
	int err = 0;
	int fd;
	char trash_dir[B_PATH_NAME_LENGTH];
	if (strstr(argv[0], "untrash") || strstr(argv[0], "restore"))
		dountrash = 1;
	if (argc < 2)
		return usage(1);
	if (!strcmp(argv[1], "--help"))
		return usage(0);
	if (!dountrash && !strcmp(argv[1], "--empty")) {
		/* XXX: clean that */
		BMessage msg(B_DELETE_PROPERTY);
		msg.AddSpecifier("Trash");
		BMessenger msgr(kTrackerSig);
		err = msgr.SendMessage(&msg);
		if (err < 0) {
			fprintf(stderr, "Emptying Trash: %s\n", strerror(err));
			return 1;
		}
		return 0;
	}
	if (dountrash && !strcmp(argv[1], "--all")) {
		/* restore all trashed files */
		//for each in all volumes
		
			//for each in trash_dir
			
		return 0;
	}
	/* restore files... */
	if (dountrash) {
		for (i = 1; i < argc; i++) {
			err = untrash(argv[i]);
			if (err) {
				fprintf(stderr, "%s: %s\n", argv[i], strerror(err));
				return 1;
			}
		}
		return err;
	}
	/* or trash them */
	for (i = 1; i < argc; i++) {
		err = trash(argv[i]);
		if (err) {
			fprintf(stderr, "%s: %s\n", argv[i], strerror(err));
			return 1;
		}
	}
	
	return err;

}
