/*
 * Copyright 2006-2022, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DEFAULT_COLORS_H
#define _DEFAULT_COLORS_H


#include <InterfaceDefs.h>

namespace BPrivate {

extern const rgb_color* kDefaultColors;

rgb_color GetSystemColor(color_which, bool darkVariant);

} // namespace BPrivate

#endif // _DEFAULT_COLORS_H
