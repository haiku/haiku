/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Image.h"


Image::Image(Team* team,const image_info& imageInfo)
	:
	fTeam(team),
	fInfo(imageInfo)
{
}


Image::~Image()
{
}


status_t
Image::Init()
{
	return B_OK;
}
