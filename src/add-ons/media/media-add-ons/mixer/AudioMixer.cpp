// AudioMixer.cpp
/*

	By David Shipman, 2002

*/

#include "AudioMixer.h"
#include "IOStructures.h"

#include <media/Buffer.h>
#include <media/TimeSource.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// AudioMixer support classes

bool				
AudioMixer::IsValidDest( media_destination dest)
{

	int32 input_cookie = 0;
	media_input input;
	
	while (GetNextInput(&input_cookie, &input) == B_OK)
	{
		
		if (dest == input.destination)
			return true;
			
	}

	return false;
	
}

bool
AudioMixer::IsValidSource( media_source source)
{

	int32 output_cookie = 0;
	media_output output;
	
	while (GetNextOutput(&output_cookie, &output) == B_OK)
	{

		if (source == output.source)
			return true;
			
	}
	
	return false;
	
}

// 'structors... 

AudioMixer::AudioMixer(BMediaAddOn *addOn)
		:	BMediaNode("OBOS Mixer"),
			BBufferConsumer(B_MEDIA_RAW_AUDIO),
			BBufferProducer(B_MEDIA_RAW_AUDIO),
			BControllable(),
			BMediaEventLooper(),
			fAddOn(addOn),
			fWeb(NULL),
			fLatency(1),
			fInternalLatency(1),
			fStartTime(0), fNextEventTime(0),
			fFramesSent(0),
			fOutputEnabled(true),
			fBufferGroup(NULL)
			
			
{

	BMediaNode::AddNodeKind(B_SYSTEM_MIXER);
	
	// set up the preferred output format
	
	fPrefOutputFormat.type = B_MEDIA_RAW_AUDIO;
	
	fPrefOutputFormat.u.raw_audio.format = media_raw_audio_format::wildcard.format; // B_AUDIO_FLOAT
	fPrefOutputFormat.u.raw_audio.frame_rate = 44100; //media_raw_audio_format::wildcard.frame_rate;
	fPrefOutputFormat.u.raw_audio.byte_order = (B_HOST_IS_BENDIAN) ? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN;

	fPrefOutputFormat.u.raw_audio.buffer_size = media_raw_audio_format::wildcard.buffer_size; //2048
	fPrefOutputFormat.u.raw_audio.channel_count = 2; //media_raw_audio_format::wildcard.channel_count;
	
	// set up the preferred input format
	
	fPrefInputFormat.type = B_MEDIA_RAW_AUDIO;
	
	fPrefInputFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	fPrefInputFormat.u.raw_audio.frame_rate = media_raw_audio_format::wildcard.frame_rate;//44100; //
	fPrefInputFormat.u.raw_audio.byte_order = (B_HOST_IS_BENDIAN) ? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN;

	fPrefInputFormat.u.raw_audio.buffer_size = media_raw_audio_format::wildcard.buffer_size;//1024; //
	fPrefInputFormat.u.raw_audio.channel_count = 2; //media_raw_audio_format::wildcard.channel_count;

	//fInput.source = media_source::null;

	// init i/o
	
	fOutput.destination = media_destination::null;
	fOutput.format = fPrefOutputFormat;
	fOutput.source.port = ControlPort();
	fOutput.source.id = 0;
	
	strcpy(fOutput.name, "Audio Mix");

	input_channel *freechannel = new input_channel;
	
	freechannel->DataAvailable = false;
	
	media_input freeinput;
	
	freeinput.format = fPrefInputFormat;
	freeinput.source = media_source::null;
	freeinput.destination.port = ControlPort();
	freeinput.destination.id = 0;	
	
	strcpy(freeinput.name, "Free Input");
	
	freechannel->fInput = freeinput;
	
	fInputChannels.AddItem(freechannel);

}

AudioMixer::~AudioMixer()
{
	
	BMediaEventLooper::Quit();
	SetParameterWeb(NULL); 
	fWeb = NULL;	

	// any other cleanup goes here

}

//
// BMediaNode methods
//

BMediaAddOn * 
AudioMixer::AddOn(int32 *internal_id) const
{
	if(fAddOn) 
	{
		*internal_id = 0;
		return fAddOn;
	}
	else 
		return NULL;
}

//
// BControllable methods
//

status_t 
AudioMixer::GetParameterValue( int32 id, bigtime_t* last_change, 
								void* value, size_t* ioSize)
{

	switch (id)
	{
	
		default:
		
			return B_ERROR;
	
	}

	return B_OK;

}

void 
AudioMixer::SetParameterValue( int32 id, bigtime_t performance_time, 
								const void* value, size_t size)
{

}

//
// BBufferConsumer methods
//

status_t
AudioMixer::HandleMessage( int32 message, const void *data, size_t size)
{
	
	// since we're using a mediaeventlooper, there shouldn't be any messages
	
	return B_ERROR;

}

status_t
AudioMixer::AcceptFormat(const media_destination &dest, media_format *format)
{

	// we accept any raw audio 

	if (IsValidDest(dest))
	{		
		if (format->type == B_MEDIA_RAW_AUDIO)
			return B_OK;
		else
			return B_MEDIA_BAD_FORMAT;
	}
	else
	return B_MEDIA_BAD_DESTINATION;	
	
}

status_t
AudioMixer::GetNextInput(int32 *cookie, media_input *out_input)
{

	if (*cookie < fInputChannels.CountItems() && *cookie >= 0)
	{
		input_channel *channel = (input_channel *)fInputChannels.ItemAt(*cookie);
		*out_input = channel->fInput;
		*cookie += 1;
		return B_OK;
	}
	else
		return B_BAD_INDEX;
	
}

void
AudioMixer::DisposeInputCookie(int32 cookie)
{

	// nothing yet

}

void
AudioMixer::BufferReceived(BBuffer *buffer)
{

	if (buffer)
	{
	if (buffer->Header()->type == B_MEDIA_PARAMETERS) 
	{
		ApplyParameterData(buffer->Data(), buffer->SizeUsed());
		buffer->Recycle();
	}
	else 
	{
	
		media_header *hdr = buffer->Header();
				
		// check input
		
		input_channel *channel;
		
		int inputcount = fInputChannels.CountItems();
		
		for (int i = 0; i < inputcount; i++)
		{
			channel = (input_channel *)fInputChannels.ItemAt(i);
			if (channel->fInput.destination.id == hdr->destination)
			{
			
			i = inputcount;
						
			bigtime_t now = TimeSource()->Now();
			bigtime_t perf_time = hdr->start_time;				
			bigtime_t how_early = perf_time - fLatency - now;

			if ((RunMode() != B_OFFLINE) && 
			(RunMode() != B_RECORDING) &&
			(how_early < 0))
			{
				
				NotifyLateProducer(channel->fInput.source, -how_early, perf_time);
		
			}
			else
			{

				size_t sample_size;
			
				switch (channel->fInput.format.u.raw_audio.format)
				{
			
					case (media_raw_audio_format::B_AUDIO_FLOAT):
					case (media_raw_audio_format::B_AUDIO_INT):
					
						sample_size = 4;
						break;
				
					case (media_raw_audio_format::B_AUDIO_SHORT):
					
						sample_size = 2;
						break;
						
					case (media_raw_audio_format::B_AUDIO_CHAR):
					case (media_raw_audio_format::B_AUDIO_UCHAR):
					
						sample_size = 1;
						break;
											
				}
	
				// calculate total byte offset for writing to the ringbuffer
				// this takes account of the Event offset, as well as the buffer's start time			

				size_t total_offset = int(channel->fEventOffset + ((((perf_time - fNextEventTime) / 1000000) * 
					channel->fInput.format.u.raw_audio.frame_rate) * 
					sample_size * channel->fInput.format.u.raw_audio.channel_count)) % int(channel->fDataSize);

				char *indata = (char *)buffer->Data();
															
				if (buffer->SizeUsed() > (channel->fDataSize - total_offset))
				{
					memcpy(channel->fData + total_offset, indata, channel->fDataSize - total_offset);
					memcpy(channel->fData, indata + (channel->fDataSize - total_offset), buffer->SizeUsed() - (channel->fDataSize - total_offset));	
				} 
				else
					memcpy(channel->fData + total_offset, indata, buffer->SizeUsed());
						
			}
		
		}
		
		buffer->Recycle();
		
		if ((B_OFFLINE == RunMode()) && (B_DATA_AVAILABLE == channel->fProducerDataStatus))
		{
			RequestAdditionalBuffer(channel->fInput.source, buffer);
		}
		
		}
				
	}
	
	}
}

void
AudioMixer::ProducerDataStatus( const media_destination &for_whom, 
								int32 status, bigtime_t at_performance_time)
{
	
	if (IsValidDest(for_whom))
	{
		media_timed_event event(at_performance_time, BTimedEventQueue::B_DATA_STATUS,
			(void *)&for_whom, BTimedEventQueue::B_NO_CLEANUP, status, 0, NULL);
		EventQueue()->AddEvent(event);
	}

}

status_t
AudioMixer::GetLatencyFor( const media_destination &for_whom, bigtime_t *out_latency, 
							media_node_id *out_timesource)
{

	if (! IsValidDest(for_whom))
		return B_MEDIA_BAD_DESTINATION;
	
	*out_latency = EventLatency() + SchedulingLatency();
	*out_timesource = TimeSource()->ID();

	return B_OK;

}

status_t
AudioMixer::Connected( const media_source &producer, const media_destination &where,
						const media_format &with_format, media_input *out_input)
{
	if (! IsValidDest(where))
		return B_MEDIA_BAD_DESTINATION;
	
	input_channel *channel = new input_channel;

	media_input freeinput;
	
	freeinput.format = with_format;
	freeinput.source = producer;
	freeinput.destination.port = ControlPort();
	freeinput.destination.id = fInputChannels.CountItems();
	
	strcpy(freeinput.name, "Used Input"); // should be the name of the upstream producer

	channel->fInput = freeinput;
	
	fInputChannels.AddItem(channel);

	// update connection info

	*out_input = channel->fInput;

	channel->fData = (char *)malloc(131072); // CHANGE_THIS - we only need to allocate space for as many
	channel->fDataSize = 131072;			// input buffers as would fill an output - plus one
	
	memset(channel->fData, 0, channel->fDataSize);
	
	channel->DataAvailable = true;

	return B_OK;

}

void
AudioMixer::Disconnected( const media_source &producer, const media_destination &where)
{
	
	input_channel *channel;
	
	// disconnected - remove the input from our list

	int inputcount = fInputChannels.CountItems();
		
	for (int i = 0; i < inputcount; i++)
	{
		channel = (input_channel *)fInputChannels.ItemAt(i);
		if (channel->fInput.destination == where)
		{
			
			i = inputcount;
			
	//		channel->DataAvailable = false;
	//		memset(channel->fData, 0, channel->fDataSize);
		
			delete channel->fData;
		
			delete channel;
		
			fInputChannels.RemoveItem(channel);
		
		}
		
	}
	
}

status_t
AudioMixer::FormatChanged( const media_source &producer, const media_destination &consumer, 
							int32 change_tag, const media_format &format)
{

	// Format changed, we need to update our local info
	// and change our ringbuffer size if needs be
	// Not supported (yet)

	return B_ERROR;
	
}

//
// BBufferProducer methods
//

status_t
AudioMixer::FormatSuggestionRequested( media_type type,	int32 quality, media_format *format)
{

	// downstream consumers are asking for a preferred format
	// if suitable, return our previously constructed format def

	if (!format)
		return B_BAD_VALUE;

	*format = fPrefOutputFormat;
	
	if (type == B_MEDIA_UNKNOWN_TYPE)
		type = B_MEDIA_RAW_AUDIO;
	
	if (type != B_MEDIA_RAW_AUDIO) 
		return B_MEDIA_BAD_FORMAT;
	else return B_OK;

}

status_t
AudioMixer::FormatProposal( const media_source &output,	media_format *format)
{

	// check to see if format is valid for the specified output
	
	if (! IsValidSource(output))
		return B_MEDIA_BAD_SOURCE;
	
	if ((format->type != B_MEDIA_UNKNOWN_TYPE) && (format->type != B_MEDIA_RAW_AUDIO))
		return B_MEDIA_BAD_FORMAT;
	else return B_OK;

}

status_t
AudioMixer::FormatChangeRequested( const media_source &source, const media_destination &destination, 
									media_format *io_format, int32 *_deprecated_)
{
	
	// try to change the format of the specified source to io_format
	// not supported (yet)
	
	return B_ERROR;
	
}

status_t 
AudioMixer::GetNextOutput( int32 *cookie, media_output *out_output)
{

	// iterate through available outputs
	
	if (*cookie == 0)
	{
		*out_output = fOutput;
		*cookie = 1;
		return B_OK;
	}
	else return B_BAD_INDEX;
	
}

status_t 
AudioMixer::DisposeOutputCookie( int32 cookie)
{

	// we don't need to do anything here, since we haven't 
	// used the cookie for anything special... 
	
	return B_OK;

}

status_t 
AudioMixer::SetBufferGroup(const media_source &for_source, BBufferGroup *newGroup)
{

	if (! IsValidSource(for_source))
		return B_MEDIA_BAD_SOURCE;

	if (newGroup == fBufferGroup) // we're already using this buffergroup
		return B_OK;

	// Ahh, someone wants us to use a different buffer group.  At this point we delete
	// the one we are using and use the specified one instead.  If the specified group is
	// NULL, we need to recreate one ourselves, and use *that*.  Note that if we're
	// caching a BBuffer that we requested earlier, we have to Recycle() that buffer
	// *before* deleting the buffer group, otherwise we'll deadlock waiting for that
	// buffer to be recycled!
	delete fBufferGroup;		// waits for all buffers to recycle
	if (newGroup != NULL)
	{
		// we were given a valid group; just use that one from now on
		fBufferGroup = newGroup;
	}
	else
	{
		// we were passed a NULL group pointer; that means we construct
		// our own buffer group to use from now on
		AllocateBuffers();
	}


	return B_OK;
}

status_t
AudioMixer::GetLatency(bigtime_t *out_latency)
{

	// report our *total* latency:  internal plus downstream plus scheduling
	*out_latency = EventLatency() + SchedulingLatency();
	return B_OK;
	
}

status_t
AudioMixer::PrepareToConnect(const media_source &what, const media_destination &where, 
								media_format *format, media_source *out_source, char *out_name)
{
	// PrepareToConnect() is the second stage of format negotiations that happens
	// inside BMediaRoster::Connect().  At this point, the consumer's AcceptFormat()
	// method has been called, and that node has potentially changed the proposed
	// format.  It may also have left wildcards in the format.  PrepareToConnect()
	// *must* fully specialize the format before returning!

	// trying to connect something that isn't our source?
	if (! IsValidSource(what))
		return B_MEDIA_BAD_SOURCE;

	// are we already connected?
	if (fOutput.destination != media_destination::null)
		return B_MEDIA_ALREADY_CONNECTED;

	// the format may not yet be fully specialized (the consumer might have
	// passed back some wildcards).  Finish specializing it now, and return an
	// error if we don't support the requested format.
	if (format->type != B_MEDIA_RAW_AUDIO)
		return B_MEDIA_BAD_FORMAT;
		
	else if ((format->u.raw_audio.format != media_raw_audio_format::B_AUDIO_FLOAT) &&  // currently support floating point
		(format->u.raw_audio.format != media_raw_audio_format::B_AUDIO_SHORT))		// and 16bit int audio
		return B_MEDIA_BAD_FORMAT;

	// all fields MUST be validated here
	// or else the consumer is prone to divide-by-zero errors

	if(format->u.raw_audio.frame_rate == media_raw_audio_format::wildcard.frame_rate)
		format->u.raw_audio.frame_rate = fPrefOutputFormat.u.raw_audio.frame_rate;

	if(format->u.raw_audio.channel_count == media_raw_audio_format::wildcard.channel_count)
		format->u.raw_audio.channel_count = fPrefOutputFormat.u.raw_audio.channel_count;
//		format->u.raw_audio.channel_count = 2;

	if(format->u.raw_audio.byte_order == media_raw_audio_format::wildcard.byte_order)
		format->u.raw_audio.byte_order = fPrefOutputFormat.u.raw_audio.byte_order;

	if (format->u.raw_audio.buffer_size == media_raw_audio_format::wildcard.buffer_size)
		format->u.raw_audio.buffer_size = 1024;		// pick something comfortable to suggest

	
	// Now reserve the connection, and return information about it
	fOutput.destination = where;
	fOutput.format = *format;
	*out_source = fOutput.source;
	strncpy(out_name, fOutput.name, B_MEDIA_NAME_LENGTH); // strncpy?

	return B_OK;
}

void 
AudioMixer::Connect( status_t error, const media_source &source, const media_destination &dest, 
						const media_format &format, char *io_name)
{
	//FPRINTF(stderr, "AudioMixer::Connect\n");

	// If something earlier failed, Connect() might still be called, but with a non-zero
	// error code.  When that happens we simply unreserve the connection and do
	// nothing else.
	
	// we need to check which output dest refers to - we only have one for now
	
	if (error)
	{
		fOutput.destination = media_destination::null;
		fOutput.format = fPrefOutputFormat;
		return;
	}

	// connection is confirmed, record information and send output name
	
	fOutput.destination = dest;
	fOutput.format = format;
	strncpy(io_name, fOutput.name, B_MEDIA_NAME_LENGTH);

	// Now that we're connected, we can determine our downstream latency.
	// Do so, then make sure we get our events early enough.
	media_node_id id;
	FindLatencyFor(fOutput.destination, &fLatency, &id);

	// we need at least the length of a full output buffer's latency (I think?)

	size_t sample_size;
			
	switch (fOutput.format.u.raw_audio.format)
	{
			
		case (media_raw_audio_format::B_AUDIO_FLOAT):
		case (media_raw_audio_format::B_AUDIO_INT):
					
			sample_size = 4;
			break;
				
		case (media_raw_audio_format::B_AUDIO_SHORT):
			
			sample_size = 2;
			break;
						
		case (media_raw_audio_format::B_AUDIO_CHAR):
		case (media_raw_audio_format::B_AUDIO_UCHAR):
			
			sample_size = 1;
			break;
											
	}

	size_t framesPerBuffer = (fOutput.format.u.raw_audio.buffer_size / sample_size) / fOutput.format.u.raw_audio.channel_count;
	
	fInternalLatency = (framesPerBuffer / fOutput.format.u.raw_audio.frame_rate) * 1000000.0;

	// might need to tweak the latency

	SetEventLatency(fLatency + fInternalLatency);

	// reset our buffer duration, etc. to avoid later calculations
	// crashes w/ divide-by-zero when connecting to a variety of nodes... 
	if (fOutput.format.u.raw_audio.frame_rate == media_raw_audio_format::wildcard.frame_rate)
	{
		fOutput.format.u.raw_audio.frame_rate = 44100;
	}
	
	bigtime_t duration = bigtime_t(1000000) * framesPerBuffer / bigtime_t(fOutput.format.u.raw_audio.frame_rate);
	SetBufferDuration(duration);

	// Set up the buffer group for our connection, as long as nobody handed us a
	// buffer group (via SetBufferGroup()) prior to this.  That can happen, for example,
	// if the consumer calls SetOutputBuffersFor() on us from within its Connected()
	// method.
	if (!fBufferGroup) AllocateBuffers();
}

void 
AudioMixer::Disconnect(const media_source &what, const media_destination &where)
{

	// Make sure that our connection is the one being disconnected
	if ((where == fOutput.destination) && (what == fOutput.source)) // change later for multisource outputs
	{
		fOutput.destination = media_destination::null;
		fOutput.format = fPrefOutputFormat;
		delete fBufferGroup;
		fBufferGroup = NULL;
	}

}

void 
AudioMixer::LateNoticeReceived(const media_source &what, bigtime_t how_much, bigtime_t performance_time)
{

	// If we're late, we need to catch up.  Respond in a manner appropriate to our
	// current run mode.
	if (what == fOutput.source)
	{
		if (RunMode() == B_RECORDING)
		{
			// A hardware capture node can't adjust; it simply emits buffers at
			// appropriate points.  We (partially) simulate this by not adjusting
			// our behavior upon receiving late notices -- after all, the hardware
			// can't choose to capture "sooner"....
		}
		else if (RunMode() == B_INCREASE_LATENCY)
		{
			// We're late, and our run mode dictates that we try to produce buffers
			// earlier in order to catch up.  This argues that the downstream nodes are
			// not properly reporting their latency, but there's not much we can do about
			// that at the moment, so we try to start producing buffers earlier to
			// compensate.
			fInternalLatency += how_much;
			SetEventLatency(fLatency + fInternalLatency);

		}
		else
		{
			// The other run modes dictate various strategies for sacrificing data quality
			// in the interests of timely data delivery.  The way *we* do this is to skip
			// a buffer, which catches us up in time by one buffer duration.
			// this should be the "drop data" mode - decrease precision could repeat the buffer ?
			
			//size_t nSamples = fOutput.format.u.raw_audio.buffer_size / sizeof(float);
			//fFramesSent += (nSamples / 2);

		}
	}
}

void 
AudioMixer::EnableOutput(const media_source &what, bool enabled, int32 *_deprecated_)
{

	// If I had more than one output, I'd have to walk my list of output records to see
	// which one matched the given source, and then enable/disable that one.  But this
	// node only has one output, so I just make sure the given source matches, then set
	// the enable state accordingly.
	if (what == fOutput.source)
	{
		fOutputEnabled = enabled;
	}
}


//
// BMediaEventLooper methods
//

void
AudioMixer::NodeRegistered()
{

		Run();
		
		fOutput.node = Node();
		fInput.node = Node();
		
		SetPriority(120);
		
		SetParameterWeb(fWeb);

}

void 
AudioMixer::HandleEvent( const media_timed_event *event, bigtime_t lateness, 
								bool realTimeEvent = false)
{

	switch (event->type)
	{
	
		case BTimedEventQueue::B_HANDLE_BUFFER:
		{

			// check output
			if (fOutput.destination != media_destination::null && fOutputEnabled) // this is in the wrong order too
			{
					
				BBuffer *outbuffer = fBufferGroup->RequestBuffer(fOutput.format.u.raw_audio.buffer_size, BufferDuration());
				
				if (outbuffer)
				{
					
					int32 outChannels = fOutput.format.u.raw_audio.channel_count;
					
					// we have an output buffer - now it needs to be filled!

					switch (fOutput.format.u.raw_audio.format)
					{
						case media_raw_audio_format::B_AUDIO_FLOAT:
						{
							float *outdata = (float*)outbuffer->Data();
							
							memset(outdata, 0, sizeof(outdata));
							
							int numsamples = int(fOutput.format.u.raw_audio.buffer_size / sizeof(float));
							
							for (int c = 0; c < fInputChannels.CountItems(); c++)
							{
								input_channel *channel = (input_channel *)fInputChannels.ItemAt(c);
															
								if (channel->DataAvailable == true) // formerly DataAvailable
								{
									int32 inChannels = channel->fInput.format.u.raw_audio.channel_count;
									bool split = outChannels > inChannels;
									bool mix = outChannels < inChannels;
									
									
									if (fOutput.format.u.raw_audio.frame_rate == channel->fInput.format.u.raw_audio.frame_rate)
									{
										switch(channel->fInput.format.u.raw_audio.format) 
										{
											
											case media_raw_audio_format::B_AUDIO_FLOAT:
	
												if (split)
												{		}
												else if (mix)
												{
													for (int s = 0; s < numsamples; s++)
													{
													//	outdata[s] = (float *)(channel->fData)[s]
													}
												}
												else
												{
													for (int s = 0; s < numsamples; s++)
													{
														float *indata = (float *)channel->fData;
														int os = ((channel->fEventOffset / 4) + s) % int(channel->fDataSize / 4);
														//outdata[s] = indata[ int(((channel->fEventOffset) * 0.25) + s) % int(channel->fDataSize / 4)];
														outdata[s] = float(indata[os]);			
													//	fHost->PostMessage(int32(32767 * indata[os]));											
														//*(float *)channel->fData[channel->fEventOffset/4) + s) % channel->fDataSize];
													}
																				
													channel->fEventOffset = (channel->fEventOffset + fOutput.format.u.raw_audio.buffer_size) % 
														channel->fDataSize;

												}
																								
												break;	
												
											case media_raw_audio_format::B_AUDIO_SHORT:
											
												if (split)
												{		}
												else if (mix)
												{
				
												}
												else
												{
													for (int s = 0; s < numsamples; s++)
													{
														int16 *indata = (int16 *)channel->fData;
														int os = ((channel->fEventOffset / 2) + s) % int(channel->fDataSize / 2);
														//outdata[s] = indata[ int(((channel->fEventOffset) * 0.25) + s) % int(channel->fDataSize / 4)];
														outdata[s] = float(indata[os] / 32767.0);		
														//*(float *)channel->fData[channel->fEventOffset/4) + s) % channel->fDataSize];
													}
													
													channel->fEventOffset = (channel->fEventOffset + fOutput.format.u.raw_audio.buffer_size / 2) % 
														channel->fDataSize;
			
												}
												
												break;
																
											} // end formatswitch
											
										} // end same-samplerate condition
									
									} // data
							
								} // chan-loop
							} // cur-case
						
						case media_raw_audio_format::B_AUDIO_SHORT:
						
							int16 *outdata = (int16*)outbuffer->Data();
							int numsamples = int(fOutput.format.u.raw_audio.buffer_size / sizeof(int16));
							
							memset(outdata, 0, outbuffer->SizeAvailable());
											
							for (int c = 0; c < fInputChannels.CountItems(); c++)
							{
								input_channel *channel = (input_channel *)fInputChannels.ItemAt(c);
															
								if (channel->DataAvailable == true) // only use if there are buffers waiting 
								{									// CHANGE_THIS - we still get looped audio when there are no buffers
									int32 inChannels = channel->fInput.format.u.raw_audio.channel_count;
									bool split = outChannels > inChannels;
									bool mix = outChannels < inChannels;
								
									if (fOutput.format.u.raw_audio.frame_rate == channel->fInput.format.u.raw_audio.frame_rate)
									{
										switch(channel->fInput.format.u.raw_audio.format) 
										{
											
											case media_raw_audio_format::B_AUDIO_FLOAT:
	
												if (split)
												{		}
												else if (mix)
												{
													for (int s = 0; s < numsamples; s++)
													{
													//	outdata[s] = (float *)(channel->fData)[s]
													}
												}
												else
												{
													for (int s = 0; s < numsamples; s++)
													{
														float *indata = (float *)channel->fData;
														int os = ((channel->fEventOffset / 4) + s) % int(channel->fDataSize / 4);
														outdata[s] = outdata[s] + int16(32767 * indata[os]);			
													}
																				
													channel->fEventOffset = (channel->fEventOffset + fOutput.format.u.raw_audio.buffer_size * 2) % 
														channel->fDataSize;

												}
																								
												break;	
												
											case media_raw_audio_format::B_AUDIO_SHORT:
											
												if (split)
												{		}
												else if (mix)
												{
				
												}
												else
												{
													for (int s = 0; s < numsamples; s++)
													{
														int16 *indata = (int16 *)channel->fData;
														int os = ((channel->fEventOffset / 2) + s) % int(channel->fDataSize / 2);
														outdata[s] = outdata[s] + indata[os];
													}
													
													channel->fEventOffset = (channel->fEventOffset + (fOutput.format.u.raw_audio.buffer_size / 2)) % 
														channel->fDataSize;

												}
												
												break;
																
											}
										}
									
									}
							
								}
							}
					// for each input, check the format, then mix into output buffer

					
					media_header *outheader = outbuffer->Header();
					outheader->type = B_MEDIA_RAW_AUDIO;
					outheader->size_used = fOutput.format.u.raw_audio.buffer_size;
					outheader->time_source = TimeSource()->ID();
					
					// if this is the first buffer, mark with the start time
					// we need this to calculate the other buffer times
					
					if (fStartTime == 0) {
						fStartTime = event->event_time;
						fNextEventTime = fStartTime;
					}

					bigtime_t stamp;
					if (RunMode() == B_RECORDING)
						stamp = event->event_time; // this is actually the same as the other modes, since we're using 
					else							// a timedevent queue and adding events at 
					{
						
						// we're in a live performance mode
						// use the start time we calculate at the end of the the mix loop
						// this time is based on the offset of all media produced so far, 
						// plus fStartTime, which is the recorded time of our first event

						stamp = fNextEventTime;

					}
								
					outheader->start_time = stamp;

					status_t err = SendBuffer(outbuffer, fOutput.destination);
												
					if(err != B_OK)
						outbuffer->Recycle();
						
					}
					
				// even if we didn't get a buffer allocated, we still need to send the next event
				
				size_t sample_size;
			
				switch (fOutput.format.u.raw_audio.format)
				{
			
					case (media_raw_audio_format::B_AUDIO_FLOAT):
					case (media_raw_audio_format::B_AUDIO_INT):
					
						sample_size = 4;
						break;
				
					case (media_raw_audio_format::B_AUDIO_SHORT):
					
						sample_size = 2;
						break;
						
					case (media_raw_audio_format::B_AUDIO_CHAR):
					case (media_raw_audio_format::B_AUDIO_UCHAR):
					
						sample_size = 1;
						break;
											
				}
	
				int framesperbuffer = (fOutput.format.u.raw_audio.buffer_size / (sample_size * fOutput.format.u.raw_audio.channel_count));
					
				fFramesSent += (framesperbuffer);
					
				// calculate the start time for the next event
					
				fNextEventTime = fStartTime + (double(fFramesSent / fOutput.format.u.raw_audio.frame_rate) * 1000000.0);
					
				media_timed_event nextBufferEvent(fNextEventTime, BTimedEventQueue::B_HANDLE_BUFFER);
				EventQueue()->AddEvent(nextBufferEvent);
				
		
							
			}

		
		break;
			
		}	
		
		case BTimedEventQueue::B_START:

			if (RunState() != B_STARTED)
			{
				// We want to start sending buffers now, so we set up the buffer-sending bookkeeping
				// and fire off the first "produce a buffer" event.
				fStartTime = 0;
				fFramesSent = 0;
				
				//fThread = spawn_thread(_mix_thread_, "audio mixer thread", B_REAL_TIME_PRIORITY, this);
				
				media_timed_event firstBufferEvent(event->event_time, BTimedEventQueue::B_HANDLE_BUFFER);

				// Alternatively, we could call HandleEvent() directly with this event, to avoid a trip through
				// the event queue, like this:
				//
				//		this->HandleEvent(&firstBufferEvent, 0, false);
				//
				EventQueue()->AddEvent(firstBufferEvent);
				
				//	fStartTime = event->event_time;
	
			}
		
			break;
			
		case BTimedEventQueue::B_STOP:
		{
			// stopped - don't process any more buffers, flush all buffers from eventqueue

			EventQueue()->FlushEvents(0, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HANDLE_BUFFER);
			break;
		}

		case BTimedEventQueue::B_DATA_STATUS:
		{
			
			input_channel *channel;
			
			int inputcount = fInputChannels.CountItems();
		
			for (int i = 0; i < inputcount; i++)
			{
				channel = (input_channel *)fInputChannels.ItemAt(i);
				if (channel->fInput.destination == (media_destination &)event->pointer)
				{
					i = inputcount;
					channel->fProducerDataStatus = event->data;
					if (channel->fProducerDataStatus == B_DATA_AVAILABLE)
						channel->DataAvailable = true;
					else
						channel->DataAvailable = false;
				}
			}
			break;
		}
		
		default:
		
			break;	
	
	}

}
								
//
// AudioMixer methods
//		
		
void 
AudioMixer::AllocateBuffers()
{

	// allocate enough buffers to span our downstream latency, plus one
	size_t size = fOutput.format.u.raw_audio.buffer_size;
	int32 count = int32(fLatency / BufferDuration() + 1 + 1);

	fBufferGroup = new BBufferGroup(size, count);
	
}

// use this later for separate threads

int32
AudioMixer::_mix_thread_(void *data)
{
	return ((AudioMixer *)data)->MixThread();
}

int32 
AudioMixer::MixThread()
{
	while (1)
	{
		snooze(500000);
	}
	
}
