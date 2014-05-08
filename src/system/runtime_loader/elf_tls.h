/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */
#ifndef ELF_TLS_H
#define ELF_TLS_H


#include "runtime_loader_private.h"
#include "utility.h"


class TLSBlock;

class TLSBlockTemplate {
public:
						TLSBlockTemplate() { }
	inline				TLSBlockTemplate(void* address, size_t fileSize,
							size_t memorySize);

			void		SetGeneration(unsigned generation)
							{ fGeneration = generation; }
			unsigned	Generation() const	{ return fGeneration; }

			void		SetBaseAddress(addr_t baseAddress);

			TLSBlock	CreateBlock();

private:
			unsigned	fGeneration;

			void*		fAddress;
			size_t		fFileSize;
			size_t		fMemorySize;
};

class TLSBlockTemplates {
public:
	static	TLSBlockTemplates&	Get();

			unsigned	Register(const TLSBlockTemplate& block);
			void		Unregister(unsigned dso);

			void		SetBaseAddress(unsigned dso, addr_t baseAddress);

			unsigned	GetGeneration(unsigned dso) const;

			TLSBlock	CreateBlock(unsigned dso);

private:
	inline				TLSBlockTemplates();

	static	TLSBlockTemplates*	fInstance;

			unsigned	fGeneration;

			utility::vector<TLSBlockTemplate>	fTemplates;
			utility::vector<unsigned>	fFreeDSOs;
};


TLSBlockTemplate::TLSBlockTemplate(void* address, size_t fileSize,
	size_t memorySize)
	:
	fAddress(address),
	fFileSize(fileSize),
	fMemorySize(memorySize)
{
}


void*	get_tls_address(unsigned dso, addr_t offset);
void	destroy_thread_tls();


#endif	// ELF_TLS_H
