/*
 * Copyright 2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef EP_APPREFFILTER_H
#define EP_APPREFFILTER_H

#include <FilePanel.h>
#include <NodeInfo.h>


class AppRefFilter : public BRefFilter {
public:
						AppRefFilter();
	virtual bool		Filter(const entry_ref *ref,
							BNode *node,
							struct stat_beos *st,
							const char *filetype);
};

#endif
