/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ImageInfo.h"


ImageInfo::ImageInfo()
	:
	fTeam(-1),
	fImage(-1),
	fName()
{
}

ImageInfo::ImageInfo(const ImageInfo& other)
	:
	fTeam(other.fTeam),
	fImage(other.fImage),
	fName(other.fName)
{
}


ImageInfo::ImageInfo(team_id team, image_id image, const BString& name)
	:
	fTeam(team),
	fImage(image),
	fName(name)
{
}


void
ImageInfo::SetTo(team_id team, image_id image, const BString& name)
{
	fTeam = team;
	fImage = image;
	fName = name;
}
