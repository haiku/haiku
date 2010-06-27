/*
 * Copyright (c) 2005, David McPaul
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _MP4_ATOM_H
#define _MP4_ATOM_H


#include "MP4Structs.h"

#include <File.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <SupportDefs.h>

#include <vector>


/*
AtomBase
	theStream : Stream;
	streamOffset : FilePtr;
	atomOffset : FilePtr;
	atomSize : int64;
	atomType : int32;  // Can be a number or a 4 char FOURCC
	atomChildren : Array of AtomBase;

public
	ProcessMetaData() - Reads in the basic Atom Meta Data
				- Calls OnProcessMetaData()
				- Calls ProcessMetaData on each child atom
(ensures stream is correct for child via offset)
	OnProcessMetaData() - Subclass override to read/set meta data

	AddChild(AtomBase) - Adds a child atom to the children array

	MoveToEnd() - Moves stream ptr to end of atom

	GetTypeAsString() - returns the type as something the user can read*/


class AtomBase;


typedef AtomBase* AtomBasePtr;
typedef std::vector<AtomBasePtr> AtomArray;


class AtomBase {
/*
	This is the basic or standard atom.  It contains data describing some aspect of the file/stream
*/
private:
			off_t		streamOffset;
			off_t		atomOffset;
			uint32		atomType;
			uint64		atomSize;
			char		fourcc[5];		// make this an alias to atomType
			AtomBase*	parentAtom;

protected:
			BPositionIO	*theStream;
			void		Indent(uint32 pindent);

public:
						AtomBase(BPositionIO *pStream, off_t pstreamOffset,
							uint32 patomType, uint64 patomSize);
	virtual				~AtomBase();
	
	virtual	bool		IsContainer() {return false;};

	virtual	BPositionIO	*OnGetStream();
			BPositionIO	*GetStream();

			bool		IsExtended() {return false;};
			bool		IsEndOfAtom() {return (GetStream()->Position() >=
							off_t(streamOffset + atomSize));};
	
	// Is this a known atom type
			bool		IsKnown();
	
	virtual	void		DisplayAtoms(uint32 pindent);
	
			uint64		GetAtomSize() {return atomSize;};
			uint32		GetAtomType() {return atomType;};
			char*		GetAtomTypeAsFourcc();
			off_t		GetAtomOffset() { return atomOffset; };
			off_t		GetStreamOffset() { return streamOffset; };
	
			uint64		GetDataSize() { return atomSize - 8;};
	
			uint64		GetBytesRemaining();
	
			bool		IsType(uint32 patomType) { return patomType == atomType; };
	
			void		SetAtomOffset(off_t patomOffset) { atomOffset = patomOffset; };
			void		SetStreamOffset(off_t pstreamOffset) { streamOffset = pstreamOffset; };
	
			const char	*GetAtomName();
	
	virtual	const char	*OnGetAtomName();
	
	//	ProcessMetaData() - Reads in the basic Atom Meta Data
	//				- Calls OnProcessMetaData()
	virtual	void		ProcessMetaData();
	
	//	OnProcessMetaData() - Subclass override to read/set meta data
	virtual	void		OnProcessMetaData();
	
	// Move stream ptr to where atom ends in stream (return false on failure)
			bool		MoveToEnd();

			void		DisplayAtoms();

	// Many atoms use an array header
			void		ReadArrayHeader(array_header *pHeader);
	
			void		SetParent(AtomBase *pParent) {parentAtom = pParent;};
			AtomBase*	GetParent() { return parentAtom;};

			void		Read(uint64	*value);
			void		Read(uint32	*value);
			void		Read(int32	*value);
			void		Read(uint16	*value);
			void		Read(uint8	*value);
			void		Read(char	*value, uint32 maxread);
			void		Read(uint8	*value, uint32 maxread);
	
			uint64		GetBits(uint64 buffer, uint8 startBit, uint8 totalBits);
			uint32		GetBits(uint32 buffer, uint8 startBit, uint8 totalBits);
};


class FullAtom : public AtomBase {
public:
						FullAtom(BPositionIO *pStream, off_t pstreamOffset,
							uint32 patomType, uint64 patomSize);
	virtual				~FullAtom();

	virtual	void		OnProcessMetaData();
			uint8		GetVersion() {return Version;};
			uint8		GetFlags1()	{return Flags1;};
			uint8		GetFlags2()	{return Flags2;};
			uint8		GetFlags3()	{return Flags3;};

private:
			uint8		Version;
			uint8		Flags1;
			uint8		Flags2;
			uint8		Flags3;
};


class AtomContainer : public AtomBase {
/*
	This is an Atom that contains other atoms.  It has children that may be Container Atoms or Standard Atoms
*/
private:
			AtomArray	atomChildren;
			uint32		TotalChildren;

	virtual	void		DisplayAtoms(uint32 pindent);
	
public:
						AtomContainer(BPositionIO *pStream, off_t pstreamOffset,
							uint32 patomType, uint64 patomSize);
	virtual				~AtomContainer();

	virtual	bool		IsContainer() {return true;};
			AtomBase	*GetChildAtom(uint32 patomType, uint32 offset=0);
			uint32		CountChildAtoms(uint32 patomType);

	//	ProcessMetaData() - Reads in the basic Atom Meta Data
	//				- Calls OnProcessMetaData()
	//				- Calls ProcessMetaData on each child atom
	//				(ensures stream is correct for child via offset)
	virtual	void		ProcessMetaData();

	// Add a atom to the children array (return false on failure)
			bool		AddChild(AtomBase *pChildAtom);

	//	OnProcessMetaData() - Subclass override to read/set meta data
	virtual	void		OnProcessMetaData();
	virtual	void		OnChildProcessingComplete() {};
};


extern AtomBase *GetAtom(BPositionIO *pStream);


#endif // _MP4_ATOM_H
