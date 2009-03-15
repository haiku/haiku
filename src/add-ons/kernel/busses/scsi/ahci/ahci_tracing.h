/*
 * Copyright 2008, Bruno G. Albuquerque. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <tracing.h>


#if AHCI_PORT_TRACING

class AHCIController;

namespace AHCIPortTracing {

class AHCIPortTraceEntry : public AbstractTraceEntry {
	protected:
		AHCIPortTraceEntry(AHCIController* controller, int index)
		: fController(controller)
		, fIndex(index)
		{
		}

		void AddDump(TraceOutput& out, const char* name)
		{
			out.Print("ahci port");
			out.Print(" - %s - ", name);
			out.Print("controller: %p", fController);
			out.Print(", index: %d", fIndex);			
		}
	
		AHCIController* fController;
		int fIndex;
};


class AHCIPortPrdTable : public AHCIPortTraceEntry {
	public:
		AHCIPortPrdTable(AHCIController* controller, int index, void* address,
			size_t size)
		: AHCIPortTraceEntry(controller, index)
		, fAddress(address)
		, fSize(size)
		{
			Initialized();
		}

		void AddDump(TraceOutput& out)
		{
			AHCIPortTraceEntry::AddDump(out, "prd table");

			out.Print(", address: %p", fAddress);
			out.Print(", size: %lu", fSize);
		}

		void* fAddress;
		int fSize;
};


}	// namespace AHCIPortTracing

#	define T_PORT(x)	new(std::nothrow) AHCIPortTracing::x

#else
#	define T_PORT(x)
#endif	// AHCI_PORT_TRACING

