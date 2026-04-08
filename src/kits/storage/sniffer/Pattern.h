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


#include <string>

#include "Range.h"


namespace BPrivate {
namespace Storage {
namespace Sniffer {

class Err;
struct Data;


//! A byte string and optional mask to be compared against a data stream.
/*! The byte string and mask (if supplied) must be of the same length. */
class Pattern {
public:
	static Pattern* Create(bool caseInsensitive,
		const std::string& string, std::string mask = std::string());
	~Pattern();

	status_t InitCheck() const;
	Err* GetErr() const;

	bool Sniff(Range range, const Data& data) const;
	ssize_t BytesNeeded() const;

protected:
	friend class RPattern;

	static void* _Create(size_t baseSize, size_t offset,
		bool caseInsensitive, const std::string& string, std::string mask);
	Pattern(bool caseInsensitive, const std::string& string, std::string mask);

private:
	bool _SniffNext(off_t& start, off_t end, const Data& data) const;

	void SetStatus(status_t status, const char *msg = NULL);
	void SetErrorMessage(const char *msg);

private:
	status_t fCStatus;
	Err *fErrorMessage;

	bool fCaseInsensitive;
	uint32 fStringLength;
	uint32 fUnmaskedStartLength;
	uint8 fData[];
};


}; // namespace Sniffer
}; // namespace Storage
}; // namespace BPrivate


#endif // _SNIFFER_PATTERN_H
