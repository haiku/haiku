//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file sniffer/CharStream.h
	Character stream class
*/
#ifndef _SNIFFER_CHAR_STREAM_H
#define _SNIFFER_CHAR_STREAM_H

#include <SupportDefs.h>

#include <string>

namespace BPrivate {
namespace Storage {
namespace Sniffer {

//! Manages a stream of characters
/*! CharStream is used by the scanner portion of the parser, which is implemented
	in TokenStream::SetTo().
	
	It's also used by BPrivate::TRoster while parsing through the the
	roster's RosterSettings file.
*/
class CharStream {
public:
	CharStream(const std::string &string);
	CharStream();
	virtual ~CharStream();
	
	status_t SetTo(const std::string &string);
	void Unset();
	status_t InitCheck() const;
	bool IsEmpty() const;
	size_t Pos() const;
	const std::string& String() const;
	
	char Get();
	void Unget();

private:
	std::string fString;
	size_t fPos;
	status_t fCStatus;
	
	CharStream(const CharStream &ref);
	CharStream& operator=(const CharStream &ref);
};

};	// namespace Sniffer
};	// namespace Storage
};	// namespace BPrivate

#endif // _SNIFFER_CHAR_STREAM_H
