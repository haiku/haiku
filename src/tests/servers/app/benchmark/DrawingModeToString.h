/*
 * Copyright (C) 2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef DRAWING_MODE_TO_STRING_H
#define DRAWING_MODE_TO_STRING_H

#include <InterfaceDefs.h>


bool ToDrawingMode(const char* string, drawing_mode& mode);
bool ToString(drawing_mode mode, const char*& string);


#endif // DRAWING_MODE_TO_STRING_H
