/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _SNIFFER_DATA_H
#define _SNIFFER_DATA_H


#include <SupportDefs.h>


namespace BPrivate {
namespace Storage {
namespace Sniffer {


//! Struct containing data to be sniffed.
struct Data {
	//! Offset in the file this data was read from.
	off_t	from;
	//! The data.
	const uint8* buffer;
	//! The length of \c data.
	size_t	length;
};


}; // namespace Sniffer
}; // namespace Storage
}; // namespace BPrivate


#endif // _SNIFFER_DATA_H
