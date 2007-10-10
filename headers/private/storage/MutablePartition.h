/*
 * Copyright 2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MUTABLE_PARTITION_H
#define _MUTABLE_PARTITION_H

#include <List.h>
#include <Partition.h>


struct user_partition_data;


class BMutablePartition {
public:
			off_t				Offset() const;
			void				SetOffset(off_t offset);

			off_t				Size() const;
			void				SetSize(off_t size);

			off_t				ContentSize() const;
			void				SetContentSize(off_t size);

			off_t				BlockSize() const;
			void				SetBlockSize(off_t blockSize);

			uint32				Status() const;

			uint32				Flags() const;
			void				SetFlags(uint32 flags);

			dev_t				Volume() const;

			int32				Index() const;

			const char*			Name() const;
			status_t			SetName(const char* name);

			const char*			ContentName() const;
			status_t			SetContentName(const char* name);

			const char*			Type() const;
			status_t			SetType(const char* type) const;

			const char*			ContentType() const;
			status_t			SetContentType(const char* type) const;

			const char*			Parameters() const;
			status_t			SetParameters(const char* parameters);

			const char*			ContentParameters() const;
			status_t			SetContentParameters(const char* parameters);

			status_t			CreateChild(int32 index,
									BMutablePartition** child);
			status_t			DeleteChild(int32 index);

			BMutablePartition*	ChildAt(int32 index) const;
			int32				CountChildren() const;

			// for the partitioning system managing the parent
			void*				ChildCookie() const;
			void				SetChildCookie(void* cookie);

private:
								BMutablePartition(
									BPartition::MutableDelegate* delegate);
								~BMutablePartition();

			status_t			Init(const user_partition_data* partitionData);

			const user_partition_data* PartitionData() const;

private:
			friend class BPartition::MutableDelegate;

			BPartition::MutableDelegate* GetDelegate() const;

			BPartition::MutableDelegate* fDelegate;
			user_partition_data* fData;
			BList				fChildren;
			void*				fChildCookie;
};


#endif	// _MUTABLE_PARTITION_H
