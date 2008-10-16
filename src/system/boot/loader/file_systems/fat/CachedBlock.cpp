/*
 * Copyright 2008, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol <revol@free.fr>
 */


#include "CachedBlock.h"
#include "Stream.h"
#include "Directory.h"
#include "File.h"

#include <util/kernel_cpp.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>


using namespace FATFS;


CachedBlock::CachedBlock(Volume &volume)
	:
	fVolume(volume),
	fBlockNumber(-1LL),
	fBlock(NULL)
{
}


CachedBlock::CachedBlock(Volume &volume, off_t block)
	:
	fVolume(volume),
	fBlockNumber(-1LL),
	fBlock(NULL)
{
	SetTo(block);
}


CachedBlock::~CachedBlock()
{
	free(fBlock);
}


