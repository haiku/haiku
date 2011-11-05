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
