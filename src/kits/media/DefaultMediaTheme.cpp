/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "DefaultMediaTheme.h"
#include "debug.h"

#include <ParameterWeb.h>


using namespace BPrivate;


DefaultMediaTheme::DefaultMediaTheme()
	: BMediaTheme("BeOS Theme", "BeOS built-in theme version 0.1")
{
	CALLED();
}


BControl *
DefaultMediaTheme::MakeControlFor(BParameter *parameter)
{
	CALLED();
}


BView *
DefaultMediaTheme::MakeViewFor(BParameterWeb *web, const BRect *hintRect)
{
	CALLED();
}


BView *
DefaultMediaTheme::MakeViewFor(BParameterGroup *group, const BRect *hintRect)
{
	CALLED();
}


BControl *
DefaultMediaTheme::MakeViewFor(BParameter *parameter, const BRect *hintRect)
{
	switch (parameter->Type()) {
		case BParameter::B_NULL_PARAMETER:
			// there is no default view for a null parameter
			return NULL;

		case BParameter::B_DISCRETE_PARAMETER:
			break;

		case BParameter::B_CONTINUOUS_PARAMETER:
			break;

		default:
			FATAL("BMediaTheme: Don't know parameter type: 0x%lx\n", parameter->Type());
	}
	return NULL;
}

