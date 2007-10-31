/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MUTABLE_PARTITION_H
#define _MUTABLE_PARTITION_H

#include <List.h>
#include <Partition.h>


struct user_partition_data;


class BMutablePartition {
public:
			void				UninitializeContents();

			off_t				Offset() const;
			void				SetOffset(off_t offset);

			off_t				Size() const;
			void				SetSize(off_t size);

			off_t				ContentSize() const;
			void				SetContentSize(off_t size);

			off_t				BlockSize() const;
			void				SetBlockSize(off_t blockSize);

			uint32				Status() const;
			void				SetStatus(uint32 status);

			uint32				Flags() const;
			void				SetFlags(uint32 flags);
			void				ClearFlags(uint32 flags);

			dev_t				VolumeID() const;
			void				SetVolumeID(dev_t volumeID);

			int32				Index() const;

			const char*			Name() const;
			status_t			SetName(const char* name);

			const char*			ContentName() const;
			status_t			SetContentName(const char* name);

			const char*			Type() const;
			status_t			SetType(const char* type);

			const char*			ContentType() const;
			status_t			SetContentType(const char* type);

			const char*			Parameters() const;
			status_t			SetParameters(const char* parameters);

			const char*			ContentParameters() const;
			status_t			SetContentParameters(const char* parameters);

			status_t			CreateChild(int32 index,
									BMutablePartition** child);
			status_t			CreateChild(int32 index, const char* type,
									const char* name, const char* parameters,
									BMutablePartition** child);
			status_t			DeleteChild(int32 index);
			status_t			DeleteChild(BMutablePartition* child);
			void				DeleteAllChildren();

			BMutablePartition*	Parent() const;
			BMutablePartition*	ChildAt(int32 index) const;
			int32				CountChildren() const;
			int32				IndexOfChild(BMutablePartition* child) const;

			void				SetChangeFlags(uint32 flags);
			uint32				ChangeFlags() const;
			void				Changed(uint32 flags, uint32 clearFlags = 0);

			// for the partitioning system managing the parent
			void*				ChildCookie() const;
			void				SetChildCookie(void* cookie);

private:
								BMutablePartition(
									BPartition::Delegate* delegate);
								~BMutablePartition();

			status_t			Init(const user_partition_data* partitionData,
									BMutablePartition* parent);

			const user_partition_data* PartitionData() const;

private:
			friend class BPartition::Delegate;

			BPartition::Delegate* GetDelegate() const;

			BPartition::Delegate* fDelegate;
			user_partition_data* fData;
			BMutablePartition*	fParent;
			BList				fChildren;
			uint32				fChangeFlags;
			void*				fChildCookie;
};


#endif	// _MUTABLE_PARTITION_H
