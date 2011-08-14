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

#include <ParameterWeb.h>
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
