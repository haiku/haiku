// mime/Supertype.cpp

#include "mime/Supertype.h"

#include <Message.h>

#include <stdio.h>


#define DBG(x) x
//#define DBG(x)
#define OUT printf

namespace BPrivate {
namespace Storage {
namespace Mime {

Supertype::Supertype(const char *super)
	: fCachedMessage(NULL)
	, fType(super)
{
}

Supertype::~Supertype()
{
	delete fCachedMessage;
}


} // namespace Mime
} // namespace Storage
} // namespace BPrivate

