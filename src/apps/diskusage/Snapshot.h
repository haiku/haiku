/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT/X11 license.
 *
 * Copyright (c) 1999 Mike Steed. You are free to use and distribute this software
 * as long as it is accompanied by it's documentation and this copyright notice.
 * The software comes with no warranty, etc.
 */
#ifndef SNAPSHOT_H
#define SNAPSHOT_H


#include <string>
#include <vector>

#include <Entry.h>


class BMimeType;
class BVolume;

using std::string;
using std::vector;


struct FileInfo {
								FileInfo();
								~FileInfo();

			void				GetPath(std::string& path) const;
			FileInfo*			FindChild(const char* name) const;
			BMimeType*			Type() const;

			bool				pseudo;
			entry_ref			ref;
			off_t				size;
			int					count;
			FileInfo*			parent;
			std::vector<FileInfo*>	children;

			int					color;
};


struct VolumeSnapshot {
								VolumeSnapshot(const BVolume* volume);
								~VolumeSnapshot();

			std::string			name;
			off_t				capacity;
			off_t				freeBytes;
			FileInfo*			rootDir;
			FileInfo*			freeSpace;
			FileInfo*			currentDir;
};

#endif // SNAPSHOT_H
