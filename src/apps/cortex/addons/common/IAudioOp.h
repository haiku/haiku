// IAudioOp.h
// * PURPOSE
//   Abstract audio-operation interface.  Each implementation
//   of IAudioOp represents an algorithm for processing
//   streaming media.
//
//   IAudioOp instances are returned by implementations of
//   IAudioOpFactory, responsible for finding the
//   appropriate algorithm for a given format combination.
//
// * NOTES
//   7sep99:
//   +++++ moving back towards a raw interface approach; the host node
//         can provide the state/parameter/event-queue access.
//         See IAudioOpHost for the operations that the host node needs
//         to provide.
//
// * HISTORY
//   e.moon		26aug99		Begun

#ifndef __IAudioOp_H__
#define __IAudioOp_H__

#include "AudioBuffer.h"

class IAudioOpHost;
class ParameterSet;

class IAudioOp {

public:											// *** HOST (NODE)
	IAudioOpHost* const				host;

public:											// *** ctor/dtor
	IAudioOp(
		IAudioOpHost*						_host) : host(_host) {}

	virtual ~IAudioOp() {}

public:											// *** REQUIRED INTERFACE 

	// Process a given source buffer to produce framesRequired
	// frames of output (this may differ from the number of frames
	// read from input if this is a resampling operation,
	// or if the operation requires some amount of 'lookahead'.)
	// The time at which the first destination frame should reach
	// its destination is given by performanceTime (this should help
	// wrt/ accurate parameter-change response.)
	//
	// Return the number of frames produced (if insufficient source
	// frames were available, this may be less than framesRequired;
	// it must never be greater.)
	// NOTE
	//   If the formats are identical, source and destination
	//   may reference the same buffer.
	//
	// ANOTHER NOTE
	//   This method may well be called multiple times in response to
	//   a single call to the functor method (operator()) if one or
	//   more events occur midway through the buffer.
	//
	virtual uint32 process(
		const AudioBuffer&			source,
		AudioBuffer&						destination,
		double&									sourceFrame,
		uint32&									destinationFrame,
		uint32									framesRequired,
		bigtime_t								performanceTime) =0;
	
	// Replace the given filter operation (responsibility for deleting
	// it is yours, in case you want to keep it around for a while.)
	//
	virtual void replace(
		IAudioOp*								oldOp) =0;
		
public:											// *** OPTIONAL INTERFACE

	// Called when the host node is started, before any calls to
	// process().

	virtual void init() {}

	// Return the number of input frames required before an output
	// frame can be produced.
	
	virtual uint32 bufferLatency() const { return 0; }
	
};

#endif /*__IAudioOp_H__*/