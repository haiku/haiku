/*
 * Copyright 2010, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */


#include "HTree.h"

#include <new>
#include <string.h>

#include "HTreeEntryIterator.h"
#include "Inode.h"
#include "Volume.h"


//#define TRACE_EXT2
#ifdef TRACE_EXT2
#	define TRACE(x...) dprintf("\33[34mext2:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif


bool
HTreeRoot::IsValid() const
{
	if (reserved != 0)
		return false;
	if (hash_version != HTREE_HASH_LEGACY
		&& hash_version != HTREE_HASH_HALF_MD4
		&& hash_version != HTREE_HASH_TEA)
		return false;
	if (root_info_length != 8)
		return false;
	if (indirection_levels > 1)
		return false;
	
	// TODO: Maybe we should check the false directory entries too?
	
	return true;
}


HTree::HTree(Volume* volume, Inode* directory)
	:
	fDirectory(directory),
	fRootEntry(NULL)
{
	fBlockSize = volume->BlockSize();
	fIndexed = volume->IndexedDirectories()
		&& (directory->Flags() & EXT2_INODE_INDEXED) != 0;

	ext2_super_block superBlock = volume->SuperBlock();
	fHashSeed[0] = superBlock.HashSeed(0);
	fHashSeed[1] = superBlock.HashSeed(1);
	fHashSeed[2] = superBlock.HashSeed(2);
	fHashSeed[3] = superBlock.HashSeed(3);

	TRACE("HTree::HTree() %lx %lx %lx %lx\n", fHashSeed[0],
		fHashSeed[1], fHashSeed[2], fHashSeed[3]);
	
	if (fHashSeed[0] == 0 && fHashSeed[1] == 0 && fHashSeed[2] == 0
		&& fHashSeed[3] == 0) {
		fHashSeed[0] = 0x67452301;
		fHashSeed[1] = 0xefcdab89;
		fHashSeed[2] = 0x98badcfe;
		fHashSeed[3] = 0x10325476;
	}
}


HTree::~HTree()
{
}


status_t
HTree::Lookup(const char* name, DirectoryIterator** iterator)
{
	if (!fIndexed || (name[0] == '.'
		&& (name[1] == '\0' || (name[1] == '.' && name[2] == '0')))) {
		// No HTree support or looking for trivial directories
		// TODO: Does these directories get hashed?
		*iterator = new(std::nothrow) DirectoryIterator(fDirectory);
		
		if (*iterator == NULL)
			return B_NO_MEMORY;
		return B_OK;
	}
	
	HTreeRoot root;
	size_t length = sizeof(root);
	
	status_t status = fDirectory->ReadAt(0, (uint8*)&root, &length);
	if (status < B_OK)
		return status;
	
	if (length != sizeof(root) || !root.IsValid()) {
		// Fallback to linear search
		*iterator = new(std::nothrow) DirectoryIterator(fDirectory);
		if (*iterator == NULL)
			return B_NO_MEMORY;
		
		return B_OK;
	}
	
	uint32 hash = _Hash(name, root.hash_version);
	
	off_t start = (off_t)root.root_info_length
		+ 2 * (sizeof(HTreeFakeDirEntry) + 4);

	fRootEntry = new(std::nothrow) HTreeEntryIterator(start, fDirectory);
	if (fRootEntry == NULL)
		return B_NO_MEMORY;
	
	fRootDeleter.SetTo(fRootEntry);
	status = fRootEntry->Init();
	if (status != B_OK)
		return status;
	
	return fRootEntry->Lookup(hash, (uint32)root.indirection_levels, iterator);
}


uint32
HTree::_Hash(const char* name, uint8 version)
{
	uint32 hash;
	
	switch (version) {
		case HTREE_HASH_LEGACY:
			hash = _HashLegacy(name);
			break;
		case HTREE_HASH_HALF_MD4:
			hash = _HashHalfMD4(name);
			break;
		case HTREE_HASH_TEA:
			hash = _HashTEA(name);
			break;
		default:
			panic("Hash verification succeeded but then failed?");
			hash = 0;
	};

	TRACE("Filename hash: %u\n", hash);
	
	return hash & ~1;
}


uint32
HTree::_HashLegacy(const char* name)
{
	uint32 hash = 0x12a3fe2d;
	uint32 previous = 0x37abe8f9;
	
	for (; *name != '\0'; ++name) {
		uint32 next = previous + (hash ^ (*name * 7152373));
		
		if ((next & 0x80000000) != 0)
			next -= 0x7fffffff;

		previous = hash;
		hash = next;
	}
	
	return hash;
}


/*inline*/ uint32
HTree::_MD4F(uint32 x, uint32 y, uint32 z)
{
	return z ^ (x & (y ^ z));
}


/*inline*/ uint32
HTree::_MD4G(uint32 x, uint32 y, uint32 z)
{
	return (x & y) + ((x ^ y) & z);
}


/*inline*/ uint32
HTree::_MD4H(uint32 x, uint32 y, uint32 z)
{
	return x ^ y ^ z;
}


/*inline*/ void
HTree::_MD4RotateVars(uint32& a, uint32& b, uint32& c, uint32& d)
{
	uint32 oldD = d;
	d = c;
	c = b;
	b = a;
	a = oldD;
}


void
HTree::_HalfMD4Transform(uint32 buffer[4], uint32 blocks[8])
{
	uint32 a, b, c, d;
	
	a = buffer[0];
	b = buffer[1];
	c = buffer[2];
	d = buffer[3];
	
	// Round 1
	uint32 shifts[4] = { 3, 7, 11, 19 };
	
	for (int i = 0; i < 8; ++i) {
		a += _MD4F(b, c, d) + blocks[i];
		uint32 shift = shifts[i % 4];
		a = (a << shift) | (a >> (32 - shift));
		
		_MD4RotateVars(a, b, c, d);
	}
	
	// Round 2
	shifts[1] = 5;
	shifts[2] = 9;
	shifts[3] = 13;

	for (int j = 0; j < 2; ++j) {
		for (int i = j; i < 4; i += 2) {
			a += _MD4G(b, c, d) + blocks[i] + 013240474631UL;
			uint32 shift = shifts[i / 2];
			a = (a << shift) | (a >> (32 - shift));
			
			_MD4RotateVars(a, b, c, d);
		}
	}
	
	// Round 3
	shifts[1] = 9;
	shifts[2] = 11;
	shifts[3] = 15;

	for (int i = 0; i < 4; ++i) {
		a += _MD4H(b, c, d) + blocks[3 - i] + 015666365641UL;
		uint32 shift = shifts[i*2];
		a = (a << shift) | (a >> (32 - shift));
		
		_MD4RotateVars(a, b, c, d);
		
		a += _MD4H(b, c, d) + blocks[7 - i] + 015666365641UL;
		shift = shifts[i*2 + 1];
		a = (a << shift) | (a >> (32 - shift));
		
		_MD4RotateVars(a, b, c, d);
	}
	
	buffer[0] += a;
	buffer[1] += b;
	buffer[2] += c;
	buffer[3] += d;
}


uint32
HTree::_HashHalfMD4(const char* name)
{
	uint32 buffer[4];

	buffer[0] = fHashSeed[0];
	buffer[1] = fHashSeed[1];
	buffer[2] = fHashSeed[2];
	buffer[3] = fHashSeed[3];
	
	for (int length = strlen(name); length > 0; length -= 32) {
		uint32 blocks[8];
		
		_PrepareBlocksForHash(name, length, blocks, 8);	
		_HalfMD4Transform(buffer, blocks);
		
		name += 32;
	}

	return buffer[1];
}


void
HTree::_TEATransform(uint32 buffer[4], uint32 blocks[4])
{
	uint32 x, y;
	x = buffer[0];
	y = buffer[1];
	
	uint32 a, b, c, d;
	a = blocks[0];
	b = blocks[1];
	c = blocks[2];
	d = blocks[3];
	
	uint32 sum = 0;
	
	for (int i = 16; i > 0; --i) {
		sum += 0x9E3779B9;
		x += ((y << 4) + a) ^ (y + sum) ^ ((y >> 5) + b);
		y += ((x << 4) + c) ^ (x + sum) ^ ((x >> 5) + d);
	}
	
	buffer[0] += x;
	buffer[1] += y;
}


uint32
HTree::_HashTEA(const char* name)
{
	uint32 buffer[4];

	buffer[0] = fHashSeed[0];
	buffer[1] = fHashSeed[1];
	buffer[2] = fHashSeed[2];
	buffer[3] = fHashSeed[3];
	
	for (int length = strlen(name); length > 0; length -= 16) {
		uint32 blocks[4];
		
		_PrepareBlocksForHash(name, length, blocks, 4);
		TRACE("_HashTEA %lx %lx %lx\n", blocks[0], blocks[1], blocks[2]);
		_TEATransform(buffer, blocks);
		
		name += 16;
	}

	return buffer[0];
}


void
HTree::_PrepareBlocksForHash(const char* string, int length, uint32* blocks,
	int numBlocks)
{
	uint32 padding = (uint32)length;
	padding = (padding << 8) | padding;
	padding = (padding << 16) | padding;
	
	int numBytes = numBlocks * 4;
	if (length > numBytes)
		length = numBytes;
	
	int completeIterations = length / 4;
	
	for (int i = 0; i < completeIterations; ++i) {
		uint32 value = 0 | *(string++);
		value = (value << 8) | *(string++);
		value = (value << 8) | *(string++);
		value = (value << 8) | *(string++);
		blocks[i] = value;
	}
	
	if (completeIterations < numBlocks) {
		int remainingBytes = length % 4;
		
		uint32 value = padding;
		for (int i = 0; i < remainingBytes; ++i)
			value = (value << 8) + *(string++);
		
		blocks[completeIterations] = value;
		
		for (int i = completeIterations + 1; i < numBlocks; ++i)
			blocks[i] = padding;
	}
}
