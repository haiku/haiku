#ifndef _MOV_ATOM_H
#define _MOV_ATOM_H

#include <File.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <SupportDefs.h>

// Std Headers
#include <map.h>

#include "QTStructs.h"

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

protected:
	BPositionIO	*theStream;
	void	Indent(uint32 pindent);
		
public:
			AtomBase(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~AtomBase();
	
	virtual bool IsContainer() {return false;};
	bool IsExtended() {return false;};
	bool IsEndOfAtom() {return (theStream->Position() >= off_t(streamOffset + atomSize));};
	
	// Is this a known atom type
	bool IsKnown();
	
	virtual void	DisplayAtoms(uint32 pindent);
	
	uint64	getAtomSize() {return atomSize;};
	uint32	getAtomType() {return atomType;};
	
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

};

extern AtomBase *getAtom(BPositionIO *pStream);

#endif // _MOV_ATOM_H
