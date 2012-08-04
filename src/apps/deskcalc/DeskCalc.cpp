/*
 * Copyright 2006 Haiku, Inc. All Rights Reserved.
 * Copyright 1997, 1998 R3 Software Ltd. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Timothy Wayper <timmy@wunderbear.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include <stdio.h>

#include "CalcApplication.h"
#include "ExpressionParser.h"


int
main(int argc, char* argv[])
{
	if (argc == 1) {
		// run GUI
		CalcApplication* app = new CalcApplication();

		app->Run();
		delete app;
	} else {
		// evaluate expression from command line
		BString expression;
		int32 i = 1;
		while (i < argc) {
			expression << argv[i];
			i++;
		}

		try {
			ExpressionParser parser;
			BString result = parser.Evaluate(expression.String());
			printf("%s\n", result.String());
		} catch (ParseException e) {
			printf("%s at %ld\n", e.message.String(), e.position + 1);
			return 1;
		}
	}

	return 0;
}
