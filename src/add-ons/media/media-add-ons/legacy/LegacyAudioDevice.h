/*****************************************************************************/
// LegacyAudioDevice class
//
// [Class Description]
//
//
// Copyright (c) 2002 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#ifndef _LEGACY_AUDIO_DEVICE_H
#define _LEGACY_AUDIO_DEVICE_H

#include <media/MediaAddOn.h>
#include <media/MediaFormats.h>

class LegacyAudioDevice
{
	public:
						LegacyAudioDevice( const char *name, int32 id );
						~LegacyAudioDevice();

		bool			CheckInit()
						{ return fInitOK; }

		flavor_info		*GetInputFlavor()
						{ return &input_flavor; }

		flavor_info		*GetOutputFlavor()
						{ return &output_flavor; }

		media_format	*GetInputFormat()
						{ return &input_format; }

		media_format	*GetOutputFormat()
						{ return &output_format; }


	protected:
		int32			Input_Thread();
		int32			Output_Thread();

	private:
		bool			fInitOK;

		int				fd;				//file-descriptor of hw device

		flavor_info 	input_flavor;
		media_format	input_format;
		sem_id			in_sem; 		//input completion semaphore
		thread_id		input_thread; 	//input thread

		flavor_info 	output_flavor;
		media_format	output_format;
		sem_id			out_sem; 		//output completion semaphore
		thread_id		output_thread;	//output thread
};

#endif
