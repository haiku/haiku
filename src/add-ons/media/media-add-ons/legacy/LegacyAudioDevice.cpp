#include <media/MediaDefs.h>
#include <unistd.h>
#include <fcntl.h>

#include "driver_io.h"

#include "LegacyAudioDevice.h"


LegacyAudioDevice::LegacyAudioDevice( const char *name, int32 id )
{
	size_t buffer_size;

	fInitOK = false;

	//open the device driver
	fd = open( name, O_RDWR );

	if ( fd == 0 ) {
		goto init_err1;
	}

	input_flavor.name              = const_cast<char *>( name );
	input_flavor.info              = const_cast<char *>( "legacy audio input" ); //XXX might get delete[]ed later
	input_flavor.kinds             = B_BUFFER_PRODUCER|B_CONTROLLABLE|B_PHYSICAL_INPUT;
	input_flavor.flavor_flags      = B_FLAVOR_IS_GLOBAL;
	input_flavor.internal_id       = 2 * id;
	input_flavor.possible_count    = 1;
	input_flavor.in_format_count   = 0;
	input_flavor.in_format_flags   = 0;
	input_flavor.in_formats        = NULL;
	input_flavor.out_format_count  = 1;
	input_flavor.out_format_flags  = 0;
	input_flavor.out_formats       = &input_format;

	//get driver capture buffer size
	//ioctl( fd, SOUND_GET_CAPTURE_PREFERRED_BUF_SIZE, &buffer_size, 0 );
	//ioctl( fd, SOUND_SET_CAPTURE_PREFERRED_BUF_SIZE, buffer_size, 0 );
	buffer_size = 4096;

	input_format.type                      = B_MEDIA_RAW_AUDIO;
	input_format.u.raw_audio.frame_rate    = 44100.0;
	input_format.u.raw_audio.format        = media_raw_audio_format::B_AUDIO_SHORT;
	input_format.u.raw_audio.channel_count = 2;
	input_format.u.raw_audio.byte_order    = B_MEDIA_HOST_ENDIAN;
	input_format.u.raw_audio.buffer_size   = buffer_size;

	//allocate input completion semaphore
	in_sem = create_sem( 0, "legacy audio input completion" );

	if ( in_sem < B_OK ) {
		goto init_err2;
	}

	//tell the driver about the capture completion semaphore
	ioctl( fd, SOUND_SET_CAPTURE_COMPLETION_SEM, &in_sem, 0 );

	output_flavor.name              = const_cast<char *>( name );
	output_flavor.info              = const_cast<char *>( "legacy audio output" ); //XXX might get delete[]ed later
	output_flavor.kinds             = B_BUFFER_CONSUMER|B_TIME_SOURCE|B_CONTROLLABLE|B_PHYSICAL_OUTPUT;
	output_flavor.flavor_flags      = B_FLAVOR_IS_GLOBAL;
	output_flavor.internal_id       = 2 * id + 1;
	output_flavor.possible_count    = 1;
	output_flavor.in_format_count   = 1;
	output_flavor.in_format_flags   = 0;
	output_flavor.in_formats        = &output_format;
	output_flavor.out_format_count  = 0;
	output_flavor.out_format_flags  = 0;
	output_flavor.out_formats       = NULL;

	//get driver playback buffer size
	//ioctl( fd, SOUND_GET_PLAYBACK_PREFERRED_BUF_SIZE, &buffer_size, 0 );
	//ioctl( fd, SOUND_SET_PLAYBACK_PREFERRED_BUF_SIZE, buffer_size, 0 );
	buffer_size = 4096; 

	output_format.type                      = B_MEDIA_RAW_AUDIO;
	output_format.u.raw_audio.frame_rate    = 44100.0;
	output_format.u.raw_audio.format        = media_raw_audio_format::B_AUDIO_SHORT;
	output_format.u.raw_audio.channel_count = 2;
	output_format.u.raw_audio.byte_order    = B_MEDIA_HOST_ENDIAN;
	output_format.u.raw_audio.buffer_size   = buffer_size;

	//allocate output completion semaphore
	out_sem = create_sem( 0, "legacy audio output completion" );

	if ( out_sem < B_OK ) {
		goto init_err3;
	}

	//tell the driver about the playback completion semaphore
	ioctl( fd, SOUND_SET_PLAYBACK_COMPLETION_SEM, &out_sem, 0 );

	fInitOK = true;
	return;

init_err3:
	delete_sem( in_sem );

init_err2:
	close( fd );

init_err1:
	return;
}


LegacyAudioDevice::~LegacyAudioDevice()
{
	if ( fInitOK == false ) {
		return;
	}

	//cleaning up...

	if ( out_sem > B_OK ) {
		delete_sem( out_sem );
	}

	if ( in_sem > B_OK ) {
		delete_sem( in_sem );
	}

	if ( fd != 0 ) {
		close( fd );
	}

	//...done
}


int32
LegacyAudioDevice::Input_Thread()
{
	while ( 1 ) {
		/* send IO request */
		// XXX this is wrong, the buffer needs a header
		//ioctl( fd, SOUND_READ_BUFFER, buffer, sizeof( buffer ) );
		

		/* Wait for IO completion:
		   The only acceptable response is B_OK. Everything else means
		   the thread should quit. */
		if ( acquire_sem( in_sem ) != B_OK ) {
			break;
		}

#if 0
		/* Send buffers only if the node is running and the output has been
		 * enabled */
		if ( !fRunning || !fEnabled ) {
			continue;
		}

		BAutolock _(fLock);

		/* Fetch a buffer from the buffer group */
		BBuffer *buffer = fBufferGroup->RequestBuffer(
						4 * fConnectedFormat.display.line_width *
						fConnectedFormat.display.line_count, 0LL);
		if (!buffer)
			continue;

		/* Fill out the details about this buffer. */
		media_header *h = buffer->Header();
		h->type = B_MEDIA_RAW_VIDEO;
		h->time_source = TimeSource()->ID();
		h->size_used = 4 * fConnectedFormat.display.line_width *
						fConnectedFormat.display.line_count;
		/* For a buffer originating from a device, you might want to calculate
		 * this based on the PerformanceTimeFor the time your buffer arrived at
		 * the hardware (plus any applicable adjustments). */
		h->start_time = fPerformanceTimeBase +
						(bigtime_t)
							((fFrame - fFrameBase) *
							(1000000 / fConnectedFormat.field_rate));
		h->file_pos = 0;
		h->orig_size = 0;
		h->data_offset = 0;
		h->u.raw_video.field_gamma = 1.0;
		h->u.raw_video.field_sequence = fFrame;
		h->u.raw_video.field_number = 0;
		h->u.raw_video.pulldown_number = 0;
		h->u.raw_video.first_active_line = 1;
		h->u.raw_video.line_count = fConnectedFormat.display.line_count;

		/* Fill in a pattern */
		uint32 *p = (uint32 *)buffer->Data();
		for (int y=0;y<fConnectedFormat.display.line_count;y++)
			for (int x=0;x<fConnectedFormat.display.line_width;x++)
				*(p++) = ((((x+y)^0^x)+fFrame) & 0xff) * (0x01010101 & fColor);

		/* Send the buffer on down to the consumer */
		if (SendBuffer(buffer, fOutput.destination) < B_OK) {
			PRINTF(-1, ("FrameGenerator: Error sending buffer\n"));
			/* If there is a problem sending the buffer, return it to its
			 * buffer group. */
			buffer->Recycle();
		}
#endif
	}

	return B_OK;
}


int32
LegacyAudioDevice::Output_Thread()
{
	while (1) {
	}

	return B_OK;
}
