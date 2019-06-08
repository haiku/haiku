// Key.h
//
// Copyright (c) 2003, Ingo Weinhold (bonefish@cs.tu-berlin.de)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can alternatively use *this file* under the terms of the the MIT
// license included in this package.

#ifndef KEY_H
#define KEY_H

#include <new>

#include <stdio.h>

#include <SupportDefs.h>

#include "Debug.h"
#include "endianess.h"

using std::nothrow;

// Key
/*!
	\class Key
	\brief Represents the on-disk structure for a key.

	Unfortunately there exist two different key formats and one can not
	always guess the right format from the data. That makes the implementation
	of the class a bit messy. This and the endianess awareness code, that is
	particularly ugly for the bitfields (hopefully at least correct).
*/
class Key : private key {
public:
	Key() {}
	Key(const Key &k) : key(k) {}
	~Key() {}

	static Key* CastFrom(key* k)
		{ return static_cast<Key*>(k); }
	static const Key* CastFrom(const key* k)
		{ return static_cast<const Key*>(k); }

	void SetTo(uint32 dirID, uint32 objectID, uint64 offset, uint32 type,
			   uint16 version)
	{
		k_dir_id = h2le(dirID);
		k_objectid = h2le(objectID);
		if (version == KEY_FORMAT_3_5) {
			u.k_offset_v1.k_offset = h2le((uint32)offset);
			u.k_offset_v1.k_uniqueness = h2le(type);
		} else
			_SetOffsetAndType(offset, type);
	}

	uint16 GuessVersion() const
	{
		// assume old format, unless detected otherwise
		switch (_GetType()) {
			case TYPE_DIRECT:
			case TYPE_INDIRECT:
			case TYPE_DIRENTRY:
				return KEY_FORMAT_3_6;
			default:
				return KEY_FORMAT_3_5;
		}
	}
	uint32 GetDirID() const { return le2h(k_dir_id); }
	uint32 GetObjectID() const { return le2h(k_objectid); }
	uint64 GetOffset(uint16 version) const
	{
		return (version == KEY_FORMAT_3_6 ? _GetOffset()
										  : le2h(u.k_offset_v1.k_offset));
	}
//	uint64 GetOffset() const { return GetOffset(GuessVersion()); }
	void SetOffset(uint64 offset, uint16 version)
	{
		if (version == KEY_FORMAT_3_6)
			_SetOffsetAndType(offset, _GetType());
		else
			u.k_offset_v1.k_offset = h2le(offset);
	}
	uint16 GetType(uint16 version) const
	{
		// current version
		if (version == KEY_FORMAT_3_6)
			return _GetType();
		// old version
		switch (le2h(u.k_offset_v1.k_uniqueness)) {
			case V1_SD_UNIQUENESS:
				return TYPE_STAT_DATA;
			case V1_INDIRECT_UNIQUENESS:
				return TYPE_INDIRECT;
			case V1_DIRECT_UNIQUENESS:
				return TYPE_DIRECT;
			case V1_DIRENTRY_UNIQUENESS:
				return TYPE_DIRENTRY;
			case V1_ANY_UNIQUENESS:
			default:
				return TYPE_ANY;
		}
	}
//	uint16 GetType() const { return GetType(GuessVersion()); }

	Key &operator=(const Key &k)
	{
		*static_cast<key*>(this) = k;
		return *this;
	}

private:
	// helpers for accessing k_offset_v2
	uint64 _GetOffset() const
	{
		#if LITTLE_ENDIAN
			return u.k_offset_v2.k_type;
		#else
			offset_v2 temp;
			*(uint64*)&temp = h2le(*(uint64*)&u.k_offset_v2);
			return temp.k_offset;
		#endif
	}
	uint32 _GetType() const
	{
		#if LITTLE_ENDIAN
			return u.k_offset_v2.k_type;
		#else
			offset_v2 temp;
			*(uint64*)&temp = h2le(*(uint64*)&u.k_offset_v2);
			return temp.k_type;
		#endif
	}
	void _SetOffsetAndType(uint64 offset, uint32 type)
	{
		u.k_offset_v2.k_offset = offset;
		u.k_offset_v2.k_type = type;
		#if !LITTLE_ENDIAN
			*(uint64*)&u.k_offset_v2 = h2le(*(uint64*)&u.k_offset_v2);
		#endif
	}
};


// VKey	-- a versioned key
/*!
	\class VKey
	\brief Wraps a Key and adds format version information.

	This class is much more useful than Key. It knows its format version and
	adds comparison operators. Note, that the operators do NOT compare the
	type fields of the key. If that is needed, the Compare() method has
	a flag for it.
*/
class VKey {
private:
	void _Unset() { if (fVersion & ALLOCATED) delete fKey; fKey = NULL; }

public:
	VKey() : fKey(NULL), fVersion(KEY_FORMAT_3_5) {}
	VKey(const Key *k, uint32 version)
		: fKey(const_cast<Key*>(k)), fVersion(version) {}
	VKey(const Key *k) : fKey(NULL), fVersion(KEY_FORMAT_3_5) { SetTo(k); }
	VKey(uint32 dirID, uint32 objectID, uint64 offset, uint32 type,
		uint16 version)
		: fKey(NULL), fVersion(KEY_FORMAT_3_5)
	{
		SetTo(dirID, objectID, offset, type, version);
	}
	VKey(const VKey &k)
		: fKey(new(nothrow) Key(*k.fKey)), fVersion(k.fVersion | ALLOCATED) {}
	~VKey() { _Unset(); }

	void SetTo(const Key *k, uint32 version)
	{
		_Unset();
		fKey = const_cast<Key*>(k);
		fVersion = version;
	}
	void SetTo(const Key *k)
	{
		_Unset();
		fKey = const_cast<Key*>(k);
		fVersion = fKey->GuessVersion();
	}
	void SetTo(uint32 dirID, uint32 objectID, uint64 offset, uint32 type,
			   uint16 version)
	{
		_Unset();
		fKey = new(nothrow) Key;
		if (version == KEY_FORMAT_3_5)
			fVersion = KEY_FORMAT_3_5 | ALLOCATED;
		else
			fVersion = KEY_FORMAT_3_6 | ALLOCATED;
		fKey->SetTo(dirID, objectID, offset, type, fVersion);
	}

	uint16 GetVersion() const { return fVersion & VERSION_MASK; }
	uint32 GetDirID() const { return fKey->GetDirID(); }
	uint32 GetObjectID() const { return fKey->GetObjectID(); }
	uint64 GetOffset() const { return fKey->GetOffset(GetVersion()); }
	uint16 GetType() const { return fKey->GetType(GetVersion()); }

	void SetOffset(uint64 offset)
		{ return fKey->SetOffset(offset, GetVersion()); }

	int Compare(const VKey &k, bool compareTypes = false) const
	{
		if (GetDirID() < k.GetDirID())
			return -1;
		if (GetDirID() > k.GetDirID())
			return 1;
		if (GetObjectID() < k.GetObjectID())
			return -1;
		if (GetObjectID() > k.GetObjectID())
			return 1;
		int64 dOffset = (int64)GetOffset() - (int64)k.GetOffset();
		if (dOffset < 0)
			return -1;
		if (dOffset > 0)
			return 1;
		if (compareTypes) {
			int32 dType = (int32)GetType() - (int32)k.GetType();
			if (dType < 0)
				return -1;
			if (dType > 0)
				return 1;
		}
		return 0;
	}
	// Note: The operators don't compare the types! Use Compare(, true), if
	// you want to do that.
	bool operator==(const VKey &k) const { return (Compare(k) == 0); }
	bool operator!=(const VKey &k) const { return (Compare(k) != 0); }
	bool operator<(const VKey &k) const { return (Compare(k) < 0); }
	bool operator>(const VKey &k) const { return (Compare(k) > 0); }
	bool operator<=(const VKey &k) const { return (Compare(k) <= 0); }
	bool operator>=(const VKey &k) const { return (Compare(k) >= 0); }

	VKey &operator=(const VKey &k)
	{
		if (!(fVersion & ALLOCATED))
			fKey = new(nothrow) Key;
		*fKey = *k.fKey;
		fVersion |= ALLOCATED;
		return *this;
	}

	void Dump() const
	{
		TPRINT(("key: {%" B_PRIu32 ", %" B_PRIu32 ", %" B_PRIu64 ", %hu}\n",
			GetDirID(), GetObjectID(), GetOffset(), GetType()));
	}

private:
	enum {
		VERSION_MASK	= KEY_FORMAT_3_5 | KEY_FORMAT_3_6,
		ALLOCATED		= 0x8000
	};

private:
	Key		*fKey;
	uint16	fVersion;
};

#endif	// KEY_H
