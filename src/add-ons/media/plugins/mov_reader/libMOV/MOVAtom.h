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
#ifndef _MOV_ATOM_H
#define _MOV_ATOM_H

#include "QTStructs.h"

#include <File.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <SupportDefs.h>

#include <map>


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
typedef std::map<uint32, AtomBasePtr, std::less<uint32> > AtomArray;

class AtomBase {

/*

	This is the basic or standard atom.  It contains data describing some aspect of the file/stream

*/

private:
	off_t	streamOffset;
	off_t	atomOffset;
	uint32	atomType;
	uint64	atomSize;
	char	fourcc[5];		// make this an alias to atomType
	AtomBase *parentAtom;

protected:
	BPositionIO	*theStream;
	void	Indent(uint32 pindent);
		
public:
			AtomBase(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~AtomBase();
	
	virtual bool IsContainer() {return false;};

	virtual BPositionIO *OnGetStream();
	BPositionIO *getStream();

	bool IsExtended() {return false;};
	bool IsEndOfAtom() {return (getStream()->Position() >= off_t(streamOffset + atomSize));};
	
	// Is this a known atom type
	bool IsKnown();
	
	virtual void	DisplayAtoms(uint32 pindent);
	
	uint64	getAtomSize() {return atomSize;};
	uint32	getAtomType() {return atomType;};
	off_t	getAtomOffset() {return atomOffset;};
	off_t	getStreamOffset() {return streamOffset;};
	
	uint64	getDataSize() {return atomSize - 8;};

	uint64	getBytesRemaining();
	
	bool	IsType(uint32 patomType) {return patomType == atomType;};
	
	void	setAtomOffset(off_t patomOffset) {atomOffset = patomOffset;};
	void	setStreamOffset(off_t pstreamOffset) {streamOffset = pstreamOffset;};
	
	char 	*getAtomName();
	
	virtual char	*OnGetAtomName();
	
	//	ProcessMetaData() - Reads in the basic Atom Meta Data
	//				- Calls OnProcessMetaData()
	virtual void	ProcessMetaData();
	
	//	OnProcessMetaData() - Subclass override to read/set meta data
	virtual void	OnProcessMetaData();
	
	// Move stream ptr to where atom ends in stream (return false on failure)
	bool	MoveToEnd();

	void	DisplayAtoms();

	// Many atoms use an array header
	void	ReadArrayHeader(array_header *pHeader);
	
	void	setParent(AtomBase *pParent) {parentAtom = pParent;};
	AtomBase *getParent() { return parentAtom;};
	
	void	Read(uint64	*value);
	void	Read(uint32	*value);
	void	Read(uint16	*value);
	void	Read(uint8	*value);
	void	Read(char	*value, uint32 maxread);
	void	Read(uint8	*value, uint32 maxread);
};

class AtomContainer : public AtomBase {

/*

	This is an Atom that contains other atoms.  It has children that may be Containter Atoms or Standard Atoms

*/

private:
	AtomArray atomChildren;
	uint32	TotalChildren;

	virtual void DisplayAtoms(uint32 pindent);
	
public:
			AtomContainer(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~AtomContainer();

	virtual bool IsContainer() {return true;};
	AtomBase *GetChildAtom(uint32 patomType, uint32 offset=0);
	uint32	CountChildAtoms(uint32 patomType);

	//	ProcessMetaData() - Reads in the basic Atom Meta Data
	//				- Calls OnProcessMetaData()
	//				- Calls ProcessMetaData on each child atom
	//				(ensures stream is correct for child via offset)
	virtual void	ProcessMetaData();

	// Add a atom to the children array (return false on failure)
	bool	AddChild(AtomBase *pChildAtom);

	//	OnProcessMetaData() - Subclass override to read/set meta data
	virtual void	OnProcessMetaData();
	virtual void	OnChildProcessingComplete() {};
};

extern AtomBase *getAtom(BPositionIO *pStream);

#endif // _MOV_ATOM_H
