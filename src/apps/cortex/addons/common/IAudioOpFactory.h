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