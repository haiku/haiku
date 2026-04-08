/*
 * Copyright 2002, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 */

/*!
	\file sniffer/RPattern.h
	Mime Sniffer RPattern declarations
*/

#ifndef _SNIFFER_R_PATTERN_H
#define _SNIFFER_R_PATTERN_H

#include "Range.h"
#include "Pattern.h"

namespace BPrivate {
namespace Storage {
namespace Sniffer {

class Err;
struct Data;


//! A Pattern and a Range, bundled into one.
class RPattern {
public:
	static RPattern* Create(Range range,
		bool caseInsensitive, const std::string& string, std::string mask);
	~RPattern();

	status_t InitCheck() const;
	Err* GetErr() const;

	bool Sniff(const Data& data) const;
	ssize_t BytesNeeded() const;

private:
	Range fRange;
	Pattern fPattern;
};


}; // namespace Sniffer
}; // namespace Storage
}; // namespace BPrivate

#endif // _SNIFFER_R_PATTERN_H
