#include "ReaderPlugin.h"

Reader::Reader()
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
