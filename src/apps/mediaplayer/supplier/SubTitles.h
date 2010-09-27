/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SUB_TITLES_H
#define SUB_TITLES_H


#include <Point.h>
#include <String.h>


class BFile;


struct SubTitle {
	BString		text;
	BPoint		placement;
	bigtime_t	startTime;
	bigtime_t	duration;
};


class SubTitles {
public:
								SubTitles();
	virtual						~SubTitles();

	virtual	const char*			Name() const = 0;
	virtual	const SubTitle*		SubTitleAt(bigtime_t time) const = 0;
};


#endif //SUB_TITLES_H
