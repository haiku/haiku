/*
 * Copyright 2008, François Revol <revol@free.fr>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef _BOOT_FILE_MAP_DISK_H
#define _BOOT_FILE_MAP_DISK_H

#include <boot/vfs.h>
#include <boot/partitions.h>

#define FMAP_FOLDER_NAME "BEOS"
#define FMAP_IMAGE_NAME "IMAGE.BE"
//#define FMAP_FOLDER_NAME "HAIKU"
//#define FMAP_IMAGE_NAME "IMAGE.BFS"

#define FMAP_MAX_RUNS 128

struct file_map_run {
	off_t offset;
	off_t block;
	off_t len;
};

struct file_map_boot_item {
	int32 block_size;
	int32 num_runs;
	struct file_map_run runs[FMAP_MAX_RUNS];
};

#ifdef _BOOT_MODE

class FileMap {
public:
	FileMap();
	~FileMap();
	void AddRun(off_t offset, off_t block, off_t len);

private:
	struct file_map_run fRuns[FMAP_MAX_RUNS];
};

class FileMapDisk : public Node {
	public:
		FileMapDisk();
		virtual ~FileMapDisk();

		status_t Init(Node *node/*Partition *partition, FileMap *map, off_t imageSize*/);

		virtual status_t Open(void **_cookie, int mode);
		virtual status_t Close(void *cookie);


		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer,
			size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer,
			size_t bufferSize);

		virtual status_t GetName(char *nameBuffer, size_t bufferSize) const;
		virtual status_t GetFileMap(struct file_map_run *runs, int32 *count);
		virtual off_t Size() const;

		static FileMapDisk *FindAnyFileMapDisk(Directory *volume);

		status_t RegisterFileMapBootItem();

	private:
		Node		*fNode;
		/*
		Partition	*fPartition;
		FileMap		*fMap;
		off_t		fImageSize;
		*/
	
};

#endif // _BOOT_MODE

#endif	// _BOOT_FILE_MAP_DISK_H
