// IParameterSet.cpp

#include "IParameterSet.h"

// --------------------------------------------------------
// *** EXTERNAL INTERFACE
// --------------------------------------------------------

IParameterSet::~IParameterSet() {} //nyi
IParameterSet::IParameterSet() {} //nyi

// set parameter if the operation is stopped, or queue
// parameter-change if it's running.
// B_BAD_INDEX: invalid parameter ID
// B_NO_MEMORY: too little data
status_t IParameterSet::setValue(
	int32										parameterID,
	bigtime_t								performanceTime,
	const void*							data,
	size_t									size) {
	
	// +++++ record performanceTime
	
	return store(parameterID, data, size);
}

// fetch last-changed value for the given parameter	
// B_BAD_INDEX: invalid parameter ID
// B_NO_MEMORY: data buffer too small
status_t IParameterSet::getValue(
	int32										parameterID,
	bigtime_t*							lastChangeTime,
	void*										data,
	size_t*									ioSize) {

	// +++++ fetch lastChangeTime
	
	return retrieve(parameterID, data, ioSize);		
}


// END -- IParameterSet.cpp --
