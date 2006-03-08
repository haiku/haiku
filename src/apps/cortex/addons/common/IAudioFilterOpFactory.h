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