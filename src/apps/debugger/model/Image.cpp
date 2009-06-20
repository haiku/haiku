/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Image.h"

#include "ImageDebugInfo.h"


Image::Image(Team* team,const ImageInfo& imageInfo)
	:
	fTeam(team),
	fInfo(imageInfo),
	fDebugInfo(NULL)
{
}


Image::~Image()
{
	if (fDebugInfo != NULL)
		fDebugInfo->RemoveReference();
}


status_t
Image::Init()
{
	return B_OK;
}


void
Image::SetImageDebugInfo(ImageDebugInfo* debugInfo)
{
	if (debugInfo == fDebugInfo)
		return;

	if (fDebugInfo != NULL)
		fDebugInfo->RemoveReference();

	fDebugInfo = debugInfo;

	if (fDebugInfo != NULL)
		fDebugInfo->AddReference();
}


bool
Image::ContainsAddress(target_addr_t address) const
{
	return (address >= fInfo.TextBase()
			&& address < fInfo.TextBase() + fInfo.TextSize())
		|| (address >= fInfo.DataBase()
			&& address < fInfo.DataBase() + fInfo.DataSize());
}
