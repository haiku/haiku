/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "compatibility.h"

#include "partition_support.h"

#include <errno.h>
#include <unistd.h>

#include <list>

#include "fssh_errno.h"
#include "fssh_stat.h"
#include "fssh_unistd.h"
#include "stat_priv.h"


using namespace FSShell;


namespace FSShell {


struct FileRestriction {
	FileRestriction(fssh_dev_t device, fssh_ino_t node, fssh_off_t startOffset,
			fssh_off_t endOffset)
		:
		device(device),
		node(node),
		startOffset(startOffset),
		endOffset(endOffset)
	{
	}

	fssh_dev_t	device;
	fssh_ino_t	node;
	fssh_off_t	startOffset;
	fssh_off_t	endOffset;
};


typedef std::list<FileRestriction*> FileRestrictionList;

static FileRestrictionList sFileRestrictions;


static FileRestriction*
find_file_restriction(fssh_dev_t device, fssh_ino_t node)
{
	for (FileRestrictionList::iterator it = sFileRestrictions.begin();
			it != sFileRestrictions.end(); ++it) {
		FileRestriction* restriction = *it;
		if (restriction->device == device && restriction->node == node)
			return restriction;
	}

	return NULL;
}


static FileRestriction*
find_file_restriction(int fd)
{
	struct fssh_stat st;
	if (unrestricted_fstat(fd, &st) < 0)
		return NULL;

	return find_file_restriction(st.fssh_st_dev, st.fssh_st_ino);
}


void
add_file_restriction(const char* fileName, fssh_off_t startOffset,
	fssh_off_t endOffset)
{
	struct fssh_stat st;
	if (unrestricted_stat(fileName, &st) < 0)
		return;

	fssh_dev_t device = st.fssh_st_dev;
	fssh_ino_t node = st.fssh_st_ino;

	FileRestriction* restriction = find_file_restriction(device, node);
	if (restriction)
		return;

	if (endOffset < 0)
		endOffset = st.fssh_st_size;

	restriction = new FileRestriction(device, node, startOffset, endOffset);
	sFileRestrictions.push_back(restriction);
}


void
restricted_file_opened(int fd)
{
	FileRestriction* restriction = find_file_restriction(fd);
	if (!restriction)
		return;

	lseek(fd, restriction->startOffset, SEEK_SET);
}


void
restricted_file_duped(int oldFD, int newFD)
{
}


void
restricted_file_closed(int fd)
{
}


int
restricted_file_restrict_io(int fd, fssh_off_t& pos, fssh_off_t size)
{
	FileRestriction* restriction = find_file_restriction(fd);
	if (!restriction)
		return 0;

	if (pos < 0) {
		pos = lseek(fd, 0, SEEK_CUR);
		if (pos < 0)
			return -1;
	} else
		pos += restriction->startOffset;

	if (pos < restriction->startOffset || pos > restriction->endOffset) {
		fssh_set_errno(B_BAD_VALUE);
		return -1;
	}

	fssh_off_t maxSize = restriction->endOffset - pos;
	if (size > maxSize)
		size = maxSize;

	return 0;
}


void
restricted_file_restrict_stat(struct fssh_stat* st)
{
	FileRestriction* restriction = find_file_restriction(st->fssh_st_dev,
		st->fssh_st_ino);
	if (!restriction)
		return;

	st->fssh_st_size = restriction->endOffset - restriction->startOffset;
}


static int
to_platform_seek_mode(int fsshWhence)
{
	switch (fsshWhence) {
		case FSSH_SEEK_CUR:
			return SEEK_CUR;
		case FSSH_SEEK_END:
			return SEEK_END;
		case FSSH_SEEK_SET:
		default:
			return SEEK_SET;
	}
}


}	// namespace FSShell


fssh_off_t
fssh_lseek(int fd, fssh_off_t offset, int whence)
{
	FileRestriction* restriction = find_file_restriction(fd);
	if (!restriction)
		return lseek(fd, offset, to_platform_seek_mode(whence));

	fssh_off_t pos;

	switch (whence) {
		case FSSH_SEEK_CUR:
		{
			pos = lseek(fd, 0, SEEK_CUR);
			if (pos < 0)
				return pos;
			pos += offset;
			break;
		}
		case FSSH_SEEK_END:
			pos = restriction->endOffset + offset;
			break;
		case FSSH_SEEK_SET:
		default:
			pos = restriction->startOffset + offset;
			break;
	}

	if (pos < restriction->startOffset) {
		fssh_set_errno(B_BAD_VALUE);
		return -1;
	}

	pos = lseek(fd, pos, SEEK_SET);
	if (pos >= 0)
		pos -= restriction->startOffset;

	return pos;
}
