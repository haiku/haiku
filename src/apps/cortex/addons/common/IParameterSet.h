// IParameterSet.h
// * PURPOSE
//   An abstract parameter-collection object.  Can be shared
//   between a media node and one or more operations to simplify
//   realtime parameter automation.
//
// * HISTORY
//   e.moon		26aug99		Begun

#ifndef __IParameterSet_H__
#define __IParameterSet_H__

#include <SupportDefs.h>

class IParameterSet {

public:											// *** EXTERNAL INTERFACE
	virtual ~IParameterSet(); //nyi
	IParameterSet(); //nyi

	// set parameter if the operation is stopped, or queue
	// parameter-change if it's running.
	// B_BAD_INDEX: invalid parameter ID
	// B_NO_MEMORY: too little data
	status_t setValue(
		int32										parameterID,
		bigtime_t								performanceTime,
		const void*							data,
		size_t									size); //nyi

	// fetch last-changed value for the given parameter	
	// B_BAD_INDEX: invalid parameter ID
	// B_NO_MEMORY: data buffer too small
	status_t getValue(
		int32										parameterID,
		bigtime_t*							lastChangeTime,
		void*										data,
		size_t*									ioSize); //nyi

public:											// *** INTERNAL INTERFACE (HOOKS)

	// write to and from this object (translating parameter ID to
	// member variable(s))

	virtual status_t store(
		int32										parameterID,
		const void*							data,
		size_t									size) =0;

	virtual status_t retrieve(
		int32										parameterID,
		void*										data,
		size_t*									ioSize) =0;
		
	// implement this hook to return a BParameterGroup representing
	// the parameters represented by this set
	
	virtual void populateGroup(
		BParameterGroup* 				group) =0;

private:										// *** IMPLEMENTATION

	// map of parameter ID -> change time +++++
};	

#endif /*__IParameterSet_H__*/