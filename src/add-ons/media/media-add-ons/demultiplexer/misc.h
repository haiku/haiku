// misc.h
//
// Andrew Bachmann, 2002
//
// Some functions for general debugging and
// working around be media kit bugs.

#if !defined(_MISC_H)
#define _MISC_H

#include <MediaDefs.h>

// -------------------------------------------------------- //
// lib functions
// -------------------------------------------------------- //

void print_multistream_format(media_multistream_format * format);
	
void print_media_format(media_format * format);

bool multistream_format_is_acceptible(
						const media_multistream_format & producer_format,
						const media_multistream_format & consumer_format);

bool format_is_acceptible(
						const media_format & producer_format,
						const media_format & consumer_format);

#endif // _MISC_H

