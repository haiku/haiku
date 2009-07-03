/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ImageInfo.h"


ImageInfo::ImageInfo()
	:
	fTeam(-1),
	fImage(-1),
	fName(),
	fTextBase(0),
	fTextSize(0),
	fDataBase(0),
	fDataSize(0)
{
}

ImageInfo::ImageInfo(const ImageInfo& other)
	:
	fTeam(other.fTeam),
	fImage(other.fImage),
	fName(other.fName),
	fType(other.fType),
	fTextBase(other.fTextBase),
	fTextSize(other.fTextSize),
	fDataBase(other.fDataBase),
	fDataSize(other.fDataSize)
{
}


ImageInfo::ImageInfo(team_id team, image_id image, const BString& name,
	image_type type, target_addr_t textBase, target_size_t textSize,
	target_addr_t dataBase, target_size_t dataSize)
	:
	fTeam(team),
	fImage(image),
	fName(name),
	fType(type),
	fTextBase(textBase),
	fTextSize(textSize),
	fDataBase(dataBase),
	fDataSize(dataSize)
{
}


void
ImageInfo::SetTo(team_id team, image_id image, const BString& name,
	image_type type, target_addr_t textBase, target_size_t textSize,
	target_addr_t dataBase, target_size_t dataSize)
{
	fTeam = team;
	fImage = image;
	fName = name;
	fType = type;
	fTextBase = textBase;
	fTextSize = textSize;
	fDataBase = dataBase;
	fDataSize = dataSize;
}
