
#include <DataIO.h>

#include "MatroskaParser.h"

struct StreamIO {
	FileCache 	filecache;
	BPositionIO *source;
};

FileCache *CreateFileCache(BDataIO *dataio);
