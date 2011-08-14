/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// IAudioFilterOpFactory.h
// * PURPOSE
//   An interface to an 'algorithm finder' object.  Implementations
//   of IFAudioilterOpFactory are given format and parameter information,
//   and are required to return the best-matching algorithm (IAudioFilterOp
//   implementation) -- or 0 if no algorithm can handle the given
//   formats.
//
//   One subclass of IAudioFilterOpFactory should exist for each
//   algorithm family, since create() provides no way to select a
//   particular algorithm.  The stock create() should produce an
//   operation initialized to acceptible defaults, but you can
//   always overload create() to provide more options.
//
// * UP IN THE AIR +++++
//   - query mechanism -- determine if a format/parameter change
//     will require an algorithm switch?
//   - explicit parameter support?  seems a better approach than
//     overloading the create() method with parameters for each
//     type of filter operation... ?
//
// * HISTORY
//   e.moon		26aug99		Begun.

#ifndef __IAudioFilterOpFactory_H__
#define __IAudioFilterOpFactory_H__

#include <MediaDefs.h>

class IAudioFilterOp;

class IAudioFilterOpFactory {
public:											// *** INTERFACE
	// The basic create method.  subclasses devoted to producing
	// more specialized audio-filter operations may want to override
	// this with further arguments specific to the task at hand.
	// Return 0 if no algorithm could be found for the given format.

	virtual IAudioFilterOp* create(
		const media_raw_audio_format&		sourceFormat,
		const media_raw_audio_format&		destinationFormat) =0;

public:											// *** HOOKS
	virtual ~IAudioFilterOpFactory() {}

};

#endif /*__IAudioFilterOpFactory_H__*/
