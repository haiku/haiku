/*
 * Copyright 2008 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef PATH_LIST_H
#define PATH_LIST_H


#include <ObjectList.h>


class PathList {
public:
							PathList();
							~PathList();

			bool			HasPath(const char* path,
								int32* _index = NULL) const;
			status_t		AddPath(const char* path);
			status_t		RemovePath(const char* path);

			int32			CountPaths() const;
			const char*		PathAt(int32 index) const;
private:
	struct path_entry;

			BObjectList<path_entry> fPaths;
};

#endif	// _DEVICE_MANAGER_H
