/*
 * Copyright 2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "AutoconfigClient.h"


AutoconfigClient::AutoconfigClient(const char* name, BMessenger target,
		const char* device)
	: BHandler(name),
	fTarget(target),
	fDevice(device)
{
}


AutoconfigClient::~AutoconfigClient()
{
}


status_t
AutoconfigClient::Initialize()
{
	return B_NOT_SUPPORTED;
}
