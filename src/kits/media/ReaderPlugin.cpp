#include "ReaderPlugin.h"

Reader::Reader()
 :	fSource(0)
{
}


Reader::~Reader()
{
}


BDataIO *
Reader::Source()
{
	return fSource;
}

void
Reader::Setup(BDataIO *source)
{
	fSource = source;
}
