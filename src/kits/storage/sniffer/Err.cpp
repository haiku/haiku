//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file Err.cpp
	MIME sniffer Error class implementation
*/

#include <sniffer/Err.h>
#include <new>
#include <string.h>

using namespace BPrivate::Storage::Sniffer;

//------------------------------------------------------------------------------
// Err
//------------------------------------------------------------------------------

Err::Err(const char *msg, const ssize_t pos)
	: fMsg(NULL)
	, fPos(-1)
{
	SetTo(msg, pos);
}

Err::Err(const std::string &msg, const ssize_t pos)
	: fMsg(NULL)
	, fPos(-1)
{
	SetTo(msg, pos);
}

Err::Err(const Err &ref)
	: fMsg(NULL)
	, fPos(-1)
{
	*this = ref;
}

Err::~Err() {
	Unset();
}

Err&
Err::operator=(const Err &ref) {
	SetTo(ref.Msg(), ref.Pos());
	return *this;
}

status_t
Err::SetTo(const char *msg, const ssize_t pos) {
	SetMsg(msg);
	SetPos(pos);
	return B_OK;
}

status_t
Err::SetTo(const std::string &msg, const ssize_t pos) {
	return SetTo(msg.c_str(), pos);
}

void
Err::Unset() {
	delete[] fMsg;
	fMsg = NULL;
	fPos = -1;
}

const char*
Err::Msg() const {
	return fMsg;
}

ssize_t
Err::Pos() const {
	return fPos;
}

void
Err::SetMsg(const char *msg) {
	if (fMsg) {
		delete[] fMsg;
		fMsg = NULL;
	} 
	if (msg) {
		fMsg = new(std::nothrow) char[strlen(msg)+1];
		if (fMsg)
			strcpy(fMsg, msg);
	}
}

void
Err::SetPos(ssize_t pos) {
	fPos = pos;
}



