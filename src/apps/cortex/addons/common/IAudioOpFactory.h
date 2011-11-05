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


// IAudioOpFactory.h
// * PURPOSE
//   An interface to an 'algorithm finder' object.  Implementations
//   of IAudioOpFactory are given format and parameter information,
//   and are required to return the best-matching algorithm (IAudioOp
//   implementation) -- or 0 if no algorithm can handle the given
//   formats.
//
//   One subclass of IAudioOpFactory should exist for each
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

#ifndef __IAudioOpFactory_H__
#define __IAudioOpFactory_H__

#include <MediaDefs.h>

class IAudioOp;
class IAudioOpHost;
class IParameterSet;

class IAudioOpFactory {
public:											// *** INTERFACE
	// The basic operation-creation method.
	// Return 0 if no algorithm could be found for the given format.

	virtual IAudioOp* createOp(
		IAudioOpHost*										host,
		const media_raw_audio_format&		inputFormat,
		const media_raw_audio_format&		outputFormat) =0;

	// Return an IParameter object for the category of
	// operation you create.

	virtual IParameterSet* createParameterSet() =0;

public:											// *** HOOKS
	virtual ~IAudioOpFactory() {}
};

#endif /*__IAudioOpFactory_H__*/
