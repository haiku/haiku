// IAudioOpHost.h
// * PURPOSE
//   This interface is used by audio-operation hosts (generally
//   media nodes) to expose the components that the operation needs
//   access to.

#ifndef __IAudioOpHost_H__
#define __IAudioOpHost_H__

class IParameterSet;

class IAudioOpHost {
public:											// *** REQUIRED INTERFACE
	virtual IParameterSet* parameterSet() const =0;
};

#endif /*__IAudioOpHost_H__*/