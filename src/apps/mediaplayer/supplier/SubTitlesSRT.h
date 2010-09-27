/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SUB_TITLES_SRT_H
#define SUB_TITLES_SRT_H


#include <List.h>

#include "SubTitles.h"


class BFile;


class SubTitlesSRT : public SubTitles {
public:
								SubTitlesSRT(BFile* file, const char* name);
	virtual						~SubTitlesSRT();

	virtual	const char*			Name() const;
	virtual	const SubTitle*		SubTitleAt(bigtime_t time) const;

private:
			int32				_IndexFor(bigtime_t startTime) const;

			BString				fName;
			BList				fSubTitles;
};


#endif //SUB_TITLES_SRT_H
