/*
 * Copyright 2016 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ATTRIBUTE_UTILITIES_H
#define ATTRIBUTE_UTILITIES_H


#include <SupportDefs.h>


class BNode;


namespace BPrivate {


status_t CopyAttributes(BNode& source, BNode& destination);


}	// namespace BPrivate


#endif // ATTRIBUTE_UTILITIES_H
