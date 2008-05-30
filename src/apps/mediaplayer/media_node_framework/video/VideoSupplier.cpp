/*
 * Copyright 2001-2008 Ingo Weinhold <ingo_weinhold@gmx.de>
 * Copyright 2001-2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT licensce.
 */
#include "VideoSupplier.h"


VideoSupplier::VideoSupplier()
	: fProcessingLatency(1000)
{
}


VideoSupplier::~VideoSupplier()
{
}


void
VideoSupplier::DeleteCaches()
{
}
