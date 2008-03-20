#include "ReaderPlugin.h"


Reader::Reader()
 :	fSource(0)
{
}


Reader::~Reader()
{
}


status_t
Reader::Seek(void* cookie, uint32 flags, int64* frame, bigtime_t* time)
{
	return B_NOT_SUPPORTED;
}


status_t
Reader::FindKeyFrame(void* cookie, uint32 flags, int64* frame, bigtime_t* time)
{
	return B_NOT_SUPPORTED;
}


BDataIO*
Reader::Source() const
{
	return fSource;
}


void
Reader::Setup(BDataIO *source)
{
	fSource = source;
}
