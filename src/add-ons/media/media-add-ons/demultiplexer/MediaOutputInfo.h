// MediaOutputInfo.h
//
// Andrew Bachmann, 2002
//
// A class to encapsulate and manipulate
// all the information for a particular
// output of a media node.

#if !defined(_MEDIA_OUTPUT_INFO_H)
#define _MEDIA_OUTPUT_INFO_H

#include <MediaDefs.h>
#include <BufferGroup.h>

class MediaOutputInfo
{
public:	
	MediaOutputInfo(char * name) {
		// null some fields
		bufferGroup = 0;
		// start enabled
		outputEnabled = true;
		// don't overwrite available space, and be sure to terminate
		strncpy(output.name,name,B_MEDIA_NAME_LENGTH-1);
		output.name[B_MEDIA_NAME_LENGTH-1] = '\0';
		// initialize the output
		output.node = media_node::null;
		output.source = media_source::null;
		output.destination = media_source::null;
	}
	~MediaOutputInfo() {
		if (bufferGroup != 0) {
			BBufferGroup * group = bufferGroup;
			bufferGroup = 0;
			delete group;
		}
	}
	SetBufferGroup(BBufferGroup * group) {
//	if (fBufferGroup != 0) {
//		if (fBufferGroup == group) {
//			return B_OK; // time saver
//		}	
//		delete fBufferGroup;
//	}
//	if (group != 0) {
//		fBufferGroup = group;
//	} else {
//		// let's take advantage of this opportunity to recalculate
//		// our downstream latency and ensure that it is up to date
//		media_node_id id;
//		FindLatencyFor(output.destination, &fDownstreamLatency, &id);
//		// buffer period gets initialized in Connect() because
//		// that is the first time we get the real values for
//		// chunk size and bit rate, which are used to compute buffer period
//		// note: you can still make a buffer group before connecting (why?)
//		//       but we don't make it, you make it yourself and pass it here.
//		//       not sure why anybody would want to do that since they need
//		//       a connection anyway...
//		if (fBufferPeriod <= 0) {
//			fprintf(stderr,"<- B_NO_INIT");
//			return B_NO_INIT;
//		}
//		int32 count = int32(fDownstreamLatency/fBufferPeriod)+2;
//		fprintf(stderr,"  downstream latency = %lld, buffer period = %lld, buffer count = %i\n",
//				fDownstreamLatency,fBufferPeriod,count);
//
//		// allocate the buffers
//		fBufferGroup = new BBufferGroup(output.format.u.multistream.max_chunk_size,count);
//		if (fBufferGroup == 0) {
//			fprintf(stderr,"<- B_NO_MEMORY\n");
//			return B_NO_MEMORY;
//		}
//		status_t status = fBufferGroup->InitCheck();
//		if (status != B_OK) {
//			fprintf(stderr,"<- fBufferGroup initialization failed\n");
//			return status;
//		}
//	}
	}
	
// They made an offer to us.  We should make sure that the offer is
// acceptable, and then we can add any requirements we have on top of
// that.  We leave wildcards for anything that we don't care about.
	status_t FormatProposal(media_format * format)
	{
		if (format == 0) {
			fprintf(stderr,"<- B_BAD_VALUE\n");
			return B_BAD_VALUE; // no crashing
		}
		// Be's format_is_compatible doesn't work,
		// so use our format_is_acceptible instead
		if (!format_is_acceptible(*format,generalFormat)) {
			fprintf(stderr,"<- B_MEDIA_BAD_FORMAT\n");
			return B_MEDIA_BAD_FORMAT;
		}
		// XXX: test because we don't trust them!
		format->SpecializeTo(wildcardedFormat);
		return B_OK;
	}
	
// Presumably we have already agreed with them that this format is
// okay.  But just in case, we check the offer. (and complain if it
// is invalid)  Then as the last thing we do, we get rid of any
// remaining wilcards.
	status_t FormatChangeRequested(const media_destination & destination,
								   media_format * io_format)
	{
		if (io_format == 0) {
			fprintf(stderr,"<- B_BAD_VALUE\n");
			return B_BAD_VALUE; // no crashing
		}
		status_t status = FormatProposal(io_format);
		if (status != B_OK) {
			fprintf(stderr,"<- MediaOutputInfo::FormatProposal failed\n");
			*io_format = generalFormat;
			return status;
		}
		io_format->SpecializeTo(fullySpecifiedFormat);
		return B_OK;
	}								   
	
	status_t PrepareToConnect(const media_destination & where,
							  media_format * format,
							  media_source * out_source,
							  char * out_name)
	{
		if (output.destination != media_destination::null) {
			fprintf(stderr,"<- B_MEDIA_ALREADY_CONNECTED\n");
			return B_MEDIA_ALREADY_CONNECTED;
		}
		status_t status = FormatChangeRequested(where,format,out_name);
		if (status != B_OK) {
			fprintf(stderr,"<- MediaOutputInfo::FormatChangeRequested failed\n");
			return status;
		}
		*out_source = output.source;
		output.destination = where;
		strncpy(out_name,output.name,B_MEDIA_NAME_LENGTH-1);
		out_name[B_MEDIA_NAME_LENGTH] = '\0';
		return B_OK;
	}					  
	
	status_t Connect(const media_destination & destination,
					 const media_format & format,
					 char * io_name)
	{
		if (io_name == 0) {
			fprintf(stderr,"<- B_BAD_VALUE\n");
			return B_BAD_VALUE;
		}
		output.destination = destination;
		output.format = format;
		strncpy(io_name,output.name,B_MEDIA_NAME_LENGTH-1);
		io_name[B_MEDIA_NAME_LENGTH-1] = '\0';

		// determine our downstream latency
		media_node_id id;
		FindLatencyFor(output.destination, &downstreamLatency, &id);

		// compute the buffer period (must be done before setbuffergroup)
		bufferPeriod = computeBufferPeriod(output.format);
		SetBufferDuration(fBufferPeriod);

		// setup the buffers if they aren't setup yet
		if (bufferGroup == 0) {
			status_t status = SetBufferGroup(output.source,0);
			if (status != B_OK) {
				fprintf(stderr,"<- SetBufferGroup failed\n");
				output.destination = media_destination::null;
				output.format = generalFormat;
				return;
			}
		}
		return B_OK;	
	}

	status_t Disconnect()
	{
		output.destination = media_destination::null;
		output.format = genericFormat;
		if (bufferGroup != 0) {
			BBufferGroup * group = bufferGroup;
			bufferGroup = 0;
			delete group;
		}
	}
	
	status_t EnableOutput(bool enabled)
	{
		outputEnabled = enabled;
		return B_OK;
	}		

public:
	media_output output;
	
	bool outputEnabled;
	
	BBufferGroup * bufferGroup;
	size_t bufferSize;
	
	bigtime_t downstreamLatency;
	
	bigtime_t bufferPeriod;

	// This format is the least restrictive we can
	// support in the general case.  (no restrictions
	// based on content)
	media_format generalFormat;

	// This format is the next least restrictive.  It
	// takes into account the content that we are using.
	// It should be the same as above with a few wildcards
	// removed.  Wildcards for things we are flexible on
	// may still be present.
	media_format wildcardedFormat;
	
	// This format provides default values for all fields.
	// These defaults are used to resolve all wildcards.
	media_format fullySpecifiedFormat;
	
	// do we need media_seek_tag in here?
}

#endif // _MEDIA_OUTPUT_INFO_H

