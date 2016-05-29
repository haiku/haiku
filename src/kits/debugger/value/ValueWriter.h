/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VALUE_WRITER_H
#define VALUE_WRITER_H


#include <OS.h>
#include <String.h>

#include <Variant.h>


class Architecture;
class CpuState;
class DebuggerInterface;
class ValueLocation;


class ValueWriter {
public:
								ValueWriter(Architecture* architecture,
									DebuggerInterface* interface,
									CpuState* cpuState,
									thread_id targetThread);
									// cpuState can be NULL
								~ValueWriter();

			Architecture*		GetArchitecture() const
									{ return fArchitecture; }

			status_t			WriteValue(ValueLocation* location,
									BVariant& value);

private:
			Architecture*		fArchitecture;
			DebuggerInterface*	fDebuggerInterface;
			CpuState*			fCpuState;
			thread_id			fTargetThread;
};


#endif	// VALUE_WRITER_H
