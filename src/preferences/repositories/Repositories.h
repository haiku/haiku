/*
 * Copyright 2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Hill
 */
#ifndef REPOSITORIES_H
#define REPOSITORIES_H


#include <Application.h>

#include "RepositoriesWindow.h"


class RepositoriesApplication : public BApplication {
public:
							RepositoriesApplication();

private:
	RepositoriesWindow*		fWindow;
};


#endif
