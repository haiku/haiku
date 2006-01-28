/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Application.h>


BApplication::BApplication(const char *signature)
{
}


BApplication::BApplication(const char *signature, status_t *error)
{
	*error = B_OK;
}


BApplication::~BApplication()
{
}

