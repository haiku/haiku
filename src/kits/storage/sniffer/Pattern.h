/*
 * Copyright 2002, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 */

/*!
	\file sniffer/Pattern.h
	Mime Sniffer Pattern declarations
*/

#ifndef _SNIFFER_PATTERN_H
#define _SNIFFER_PATTERN_H

#include "Range.h"
#include <SupportDefs.h>
#include <string>


namespace BPrivate {
namespace Storage {
namespace Sniffer {

class Err;
struct Data;


//! A byte string and optional mask to be compared against a data stream.
/*! The byte string and mask (if supplied) must be of the same length. */
class Pattern {
public:
	Pattern(bool caseInsensitive, const std::string &string, std::string mask = std::string());
	~Pattern();

	status_t InitCheck() const;
	Err* GetErr() const;

	bool Sniff(Range range, const Data& data) const;
	ssize_t BytesNeeded() const;

	status_t SetTo(bool caseInsensitive, const std::string &string, const std::string &mask);

private:
	bool Sniff(off_t start, const Data& data) const;
	
	void SetStatus(status_t status, const char *msg = NULL);
	void SetErrorMessage(const char *msg);

	std::string fString;
	std::string fMask;
	bool fCaseInsensitive;

	status_t fCStatus;
	Err *fErrorMessage;
};


}; // namespace Sniffer
}; // namespace Storage
}; // namespace BPrivate


#endif // _SNIFFER_PATTERN_H
