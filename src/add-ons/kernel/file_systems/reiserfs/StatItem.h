// StatItem.h
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

#ifndef STAT_ITEM_H
#define STAT_ITEM_H

#include <sys/stat.h>

#include <SupportDefs.h>

#include "Block.h"
#include "Debug.h"
#include "endianess.h"
#include "Item.h"
#include "reiserfs.h"

// StatData
/*!
	\class StatData
	\brief Represents the on-disk structure for stat data (stat item contents).

	There are two different formats for stat data. This class hides this
	fact and provides convenient access to the fields.
*/
class StatData {
public:
	StatData() : fCurrentData(NULL), fVersion(STAT_DATA_V2) {}
	StatData(const StatData &data)
		: fCurrentData(NULL), fVersion(STAT_DATA_V2) { *this = data; }
	StatData(stat_data_v1 *data, bool clone = false)
		: fCurrentData(NULL), fVersion(STAT_DATA_V2) { SetTo(data, clone); }
	StatData(stat_data *data, bool clone = false)
		: fCurrentData(NULL), fVersion(STAT_DATA_V2) { SetTo(data, clone); }
	~StatData() { Unset(); }

	status_t SetTo(stat_data_v1 *data, bool clone = false)
	{
		Unset();
		status_t error = B_OK;
		fVersion = STAT_DATA_V1;
		if (clone && data) {
			fOldData = new(nothrow) stat_data_v1;
			if (fOldData) {
				*fOldData = *data;
				fVersion |= ALLOCATED;
			} else
				error = B_NO_MEMORY;
		} else
			fOldData = data;
		return error;
	}

	status_t SetTo(stat_data *data, bool clone = false)
	{
		Unset();
		status_t error = B_OK;
		fVersion = STAT_DATA_V2;
		if (clone && data) {
			fCurrentData = new(nothrow) stat_data;
			if (fCurrentData) {
				*fCurrentData = *data;
				fVersion |= ALLOCATED;
			} else
				error = B_NO_MEMORY;
		} else
			fCurrentData = data;
		return error;
	}

	void Unset()
	{
		if (fVersion & ALLOCATED) {
			if (GetVersion() == STAT_DATA_V2) {
				delete fCurrentData;
				fCurrentData = NULL;
			} else {
				delete fOldData;
				fOldData = NULL;
			}
		}
	}

	uint32 GetVersion() const { return (fVersion & VERSION_MASK); }

	uint16 GetMode() const
	{
		return (GetVersion() == STAT_DATA_V2 ? le2h(fCurrentData->sd_mode)
											: le2h(fOldData->sd_mode));
	}

	uint32 GetNLink() const
	{
		return (GetVersion() == STAT_DATA_V2 ? le2h(fCurrentData->sd_nlink)
											: le2h(fOldData->sd_nlink));
	}

	uint32 GetUID() const
	{
		return (GetVersion() == STAT_DATA_V2 ? le2h(fCurrentData->sd_uid)
											: le2h(fOldData->sd_uid));
	}

	uint32 GetGID() const
	{
		return (GetVersion() == STAT_DATA_V2 ? le2h(fCurrentData->sd_gid)
											: le2h(fOldData->sd_gid));
	}

	uint64 GetSize() const
	{
		return (GetVersion() == STAT_DATA_V2 ? le2h(fCurrentData->sd_size)
											: le2h(fOldData->sd_size));
	}

	uint32 GetATime() const
	{
		return (GetVersion() == STAT_DATA_V2 ? le2h(fCurrentData->sd_atime)
											: le2h(fOldData->sd_atime));
	}

	uint32 GetMTime() const
	{
		return (GetVersion() == STAT_DATA_V2 ? le2h(fCurrentData->sd_mtime)
											: le2h(fOldData->sd_mtime));
	}

	uint32 GetCTime() const
	{
		return (GetVersion() == STAT_DATA_V2 ? le2h(fCurrentData->sd_ctime)
											: le2h(fOldData->sd_ctime));
	}

	uint32 GetBlocks() const
	{
		return (GetVersion() == STAT_DATA_V2 ? le2h(fCurrentData->sd_blocks)
											: le2h(fOldData->u.sd_blocks));
	}

	uint32 GetRDev() const
	{
		return (GetVersion() == STAT_DATA_V2 ? le2h(fCurrentData->u.sd_rdev)
											: le2h(fOldData->u.sd_rdev));
	}

	uint32 GetGeneration() const
	{
		return (GetVersion() == STAT_DATA_V2
			? le2h(fCurrentData->u.sd_generation) : 0);
	}

	bool IsDir() const { return S_ISDIR(GetMode()); }
	bool IsFile() const { return S_ISREG(GetMode()); }
	bool IsSymlink() const { return S_ISLNK(GetMode()); }
	bool IsEsoteric() const { return (!IsDir() && !IsFile() && !IsSymlink()); }


	void Dump()
	{
		PRINT(("StatData:\n"));
		PRINT(("  mode:       %hx\n", GetMode()));
		PRINT(("  nlink:      %" B_PRIu32 "\n", GetNLink()));
		PRINT(("  uid:        %" B_PRIx32 "\n", GetUID()));
		PRINT(("  gid:        %" B_PRIx32 "\n", GetGID()));
		PRINT(("  size:       %" B_PRIu64 "\n", GetSize()));
		PRINT(("  atime:      %" B_PRIu32 "\n", GetATime()));
		PRINT(("  mtime:      %" B_PRIu32 "\n", GetMTime()));
		PRINT(("  ctime:      %" B_PRIu32 "\n", GetCTime()));
		PRINT(("  blocks:     %" B_PRIu32 "\n", GetBlocks()));
		PRINT(("  rdev:       %" B_PRIu32 "\n", GetRDev()));
		PRINT(("  generation: %" B_PRIu32 "\n", GetGeneration()));
	}

	StatData &operator=(const StatData &data)
	{
		if (&data != this) {
			if (data.GetVersion() == STAT_DATA_V2)
				SetTo(data.fCurrentData, true);
			else
				SetTo(data.fOldData, true);
		}
		return *this;
	}

private:
	enum {
		VERSION_MASK	= STAT_DATA_V1 | STAT_DATA_V2,
		ALLOCATED		= 0x8000
	};

private:
	union {
		stat_data_v1	*fOldData;
		stat_data		*fCurrentData;
	};
	uint16	fVersion;
};

// StatItem
/*!
	\class StatItem
	\brief Provides access to the on-disk stat item structure.

	A stat item simply consists of StatData. This is only a convenience
	class to get hold of it.
*/
class StatItem : public Item {
public:
	StatItem() : Item() {}
	StatItem(LeafNode *node, ItemHeader *header)
		: Item(node, header) {}

	status_t GetStatData(StatData *statData, bool clone = false) const
	{
		status_t error = B_OK;
		if (GetLen() == sizeof(stat_data)) {
			stat_data *data = (stat_data*)GetData();
			statData->SetTo(data, clone);
		} else if (GetLen() == sizeof(stat_data_v1)) {
			stat_data_v1 *data = (stat_data_v1*)GetData();
			statData->SetTo(data, clone);
		} else {
			FATAL(("WARNING: bad stat item %" B_PRId32 " "
				"on node %" B_PRIu64 ": the item len "
				"(%u) does not match the len of any stat data format!\n",
				GetIndex(), fNode->GetNumber(), GetLen()));
			error = B_BAD_DATA;
		}
		return error;
	}
};

#endif	// STAT_ITEM_H
