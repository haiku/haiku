/*
 * Copyright 2006 Haiku, Inc. All Rights Reserved.
 * Copyright 1997, 1998 R3 Software Ltd. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Timothy Wayper <timmy@wunderbear.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "CalcApplication.h"

int
main(int argc, char* argv[])
{
	CalcApplication* app = new CalcApplication();
	
	app->Run();
	delete app;

	return 0;
}
