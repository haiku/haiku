// misc.cpp
//
// Andrew Bachmann, 2002
//
// Some functions for general debugging and
// working around be media kit bugs.

#include "misc.h"
#include <stdio.h>

// -------------------------------------------------------- //
// lib functions
// -------------------------------------------------------- //

void print_multistream_format(media_multistream_format * format) {
	fprintf(stderr,"[");
	switch (format->format) {
	case media_multistream_format::B_ANY:			fprintf(stderr,"ANY"); break;
	case media_multistream_format::B_VID:			fprintf(stderr,"VID"); break;
	case media_multistream_format::B_AVI:			fprintf(stderr,"AVI"); break;
	case media_multistream_format::B_MPEG1:		fprintf(stderr,"MPEG1"); break;
	case media_multistream_format::B_MPEG2:		fprintf(stderr,"MPEG2"); break;
	case media_multistream_format::B_QUICKTIME:	fprintf(stderr,"QUICKTIME"); break;
	default:			fprintf(stderr,"????"); break;
	}
	fprintf(stderr," avg_bit_rate(%f) max_bit_rate(%f)",
			format->avg_bit_rate,format->max_bit_rate);
	fprintf(stderr," avg_chunk_size(%i) max_chunk_size(%i)",
			format->avg_chunk_size,format->max_chunk_size);
}	
	
void print_media_format(media_format * format) {
	fprintf(stderr,"{");
	switch (format->type) {
	case B_MEDIA_NO_TYPE:		fprintf(stderr,"NO_TYPE"); break;		
	case B_MEDIA_UNKNOWN_TYPE:	fprintf(stderr,"UNKNOWN_TYPE"); break;
	case B_MEDIA_RAW_AUDIO:		fprintf(stderr,"RAW_AUDIO"); break;
	case B_MEDIA_RAW_VIDEO:		fprintf(stderr,"RAW_VIDEO"); break;
	case B_MEDIA_VBL:			fprintf(stderr,"VBL"); break;
	case B_MEDIA_TIMECODE:		fprintf(stderr,"TIMECODE"); break;
	case B_MEDIA_MIDI:			fprintf(stderr,"MIDI"); break;
	case B_MEDIA_TEXT:			fprintf(stderr,"TEXT"); break;
	case B_MEDIA_HTML:			fprintf(stderr,"HTML"); break;
	case B_MEDIA_MULTISTREAM:	fprintf(stderr,"MULTISTREAM"); break;
	case B_MEDIA_PARAMETERS:	fprintf(stderr,"PARAMETERS"); break;
	case B_MEDIA_ENCODED_AUDIO:	fprintf(stderr,"ENCODED_AUDIO"); break;
	case B_MEDIA_ENCODED_VIDEO:	fprintf(stderr,"ENCODED_VIDEO"); break;
	default:					fprintf(stderr,"????"); break;
	}
	fprintf(stderr,":");
	switch (format->type) {
	case B_MEDIA_RAW_AUDIO:		fprintf(stderr,"RAW_AUDIO"); break;
	case B_MEDIA_RAW_VIDEO:		fprintf(stderr,"RAW_VIDEO"); break;
	case B_MEDIA_MULTISTREAM:	print_multistream_format(&format->u.multistream); break;
	case B_MEDIA_ENCODED_AUDIO:	fprintf(stderr,"ENCODED_AUDIO"); break;
	case B_MEDIA_ENCODED_VIDEO:	fprintf(stderr,"ENCODED_VIDEO"); break;
	default:					fprintf(stderr,"????"); break;
	}
	fprintf(stderr,"}");
}

bool multistream_format_is_acceptible(
						const media_multistream_format & producer_format,
						const media_multistream_format & consumer_format)
{
	// first check the format, if necessary
	if (consumer_format.format != media_multistream_format::B_ANY) {
		if (consumer_format.format != producer_format.format) {
			return false;
		}
	}
	// then check the average bit rate
	if (consumer_format.avg_bit_rate != media_multistream_format::wildcard.avg_bit_rate) {
		if (consumer_format.avg_bit_rate != producer_format.avg_bit_rate) {
			// do they have to match exactly?  I don't know.  assume yes.
			return false;
		}
	}
	// then check the maximum bit rate
	if (consumer_format.max_bit_rate != media_multistream_format::wildcard.max_bit_rate) {
		if (consumer_format.max_bit_rate != producer_format.max_bit_rate) {
			// do they have to match exactly?  I don't know.  assume yes.
			return false;
		}
	}
	// then check the average chunk size
	if (consumer_format.avg_chunk_size != media_multistream_format::wildcard.avg_chunk_size) {
		if (consumer_format.avg_chunk_size != producer_format.avg_chunk_size) {
			// do they have to match exactly?  I don't know.  assume yes.
			return false;
		}
	}
	// then check the maximum bit rate
	if (consumer_format.max_chunk_size != media_multistream_format::wildcard.max_chunk_size) {
		if (consumer_format.max_chunk_size != producer_format.max_chunk_size) {
			// do they have to match exactly?  I don't know.  assume yes.
			return false;
		}
	}
	// should also check format specific fields, and others?
	return true;
}						

bool format_is_acceptible(
						const media_format & producer_format,
						const media_format & consumer_format)
{
	// first check the type, if necessary
	if (consumer_format.type != B_MEDIA_UNKNOWN_TYPE) {
		if (consumer_format.type != producer_format.type) {
			return false;
		}
		switch (consumer_format.type) {
			case B_MEDIA_MULTISTREAM:
				if (!multistream_format_is_acceptible(producer_format.u.multistream,
													  consumer_format.u.multistream)) {
					return false;
				}
				break;
			default:
				fprintf(stderr,"format_is_acceptible : unimplemented type.\n");
				return format_is_compatible(producer_format,consumer_format);
				break;
		}
	}
	// should also check non-type fields?
	return true;
}

