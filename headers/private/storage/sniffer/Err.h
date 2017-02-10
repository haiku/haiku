//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file sniffer/Err.h
	Mime Sniffer Error class declarations
*/
#ifndef _SNIFFER_ERR_H
#define _SNIFFER_ERR_H

#include <SupportDefs.h>
#include <string>

namespace BPrivate {
namespace Storage {
namespace Sniffer {

//! Exception class used by the MIME Sniffer
/*! Each exception contains an error message, and an byte offset into
	the original rule that generated the error, for the sake of
	providing spiffy error messages of the following sort:
	
	<code>
	"1.0 ('abc' & 0xFFAAFFAA)"
	              ^    Sniffer pattern error: pattern and mask lengths do not match
	</code>
*/
class Err {
public:
	Err(const char *msg, const ssize_t pos);
	Err(const std::string &msg, const ssize_t pos);
	Err(const Err &ref);
	virtual ~Err();	
	Err& operator=(const Err &ref);
	
	status_t SetTo(const char *msg, const ssize_t pos);
	status_t SetTo(const std::string &msg, const ssize_t pos);
	void Unset();
	
	void SetMsg(const char *msg);
	void SetPos(ssize_t pos);

	const char* Msg() const;
	ssize_t Pos() const;
	
private:
	char *fMsg;
	ssize_t fPos;
};

};	// namespace Sniffer
};	// namespace Storage
};	// namespace BPrivate

#endif	// _SNIFFER_ERR_H


