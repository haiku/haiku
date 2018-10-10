/*
 * Copyright 2010, Haiku Inc. All rights reserved.
 * Copyright 2001-2010, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */
#ifndef JOURNAL_H
#define JOURNAL_H


#define JOURNAL_MAGIC								0xc03b3998U

#define JOURNAL_DESCRIPTOR_BLOCK					1
#define JOURNAL_COMMIT_BLOCK						2
#define JOURNAL_SUPERBLOCK_V1						3
#define JOURNAL_SUPERBLOCK_V2						4
#define JOURNAL_REVOKE_BLOCK						5

#define JOURNAL_FLAG_ESCAPED						1
#define JOURNAL_FLAG_SAME_UUID						2
#define JOURNAL_FLAG_DELETED						4
#define JOURNAL_FLAG_LAST_TAG						8

#define JOURNAL_FEATURE_COMPATIBLE_CHECKSUM			0x1

#define JOURNAL_FEATURE_INCOMPATIBLE_REVOKE			0x1
#define JOURNAL_FEATURE_INCOMPATIBLE_64BIT			0x2
#define JOURNAL_FEATURE_INCOMPATIBLE_ASYNC_COMMIT	0x4
#define JOURNAL_FEATURE_INCOMPATIBLE_CSUM_V2		0x8
#define JOURNAL_FEATURE_INCOMPATIBLE_CSUM_V3		0x10

#define JOURNAL_KNOWN_READ_ONLY_COMPATIBLE_FEATURES	0
#define JOURNAL_KNOWN_INCOMPATIBLE_FEATURES			\
	(JOURNAL_FEATURE_INCOMPATIBLE_REVOKE | JOURNAL_FEATURE_INCOMPATIBLE_64BIT \
		| JOURNAL_FEATURE_INCOMPATIBLE_CSUM_V3)

#define JOURNAL_CHECKSUM_TYPE_CRC32					0x1
#define JOURNAL_CHECKSUM_TYPE_MD5					0x2
#define JOURNAL_CHECKSUM_TYPE_SHA1					0x3
#define JOURNAL_CHECKSUM_TYPE_CRC32C				0x4

#include "Volume.h"

#include <AutoDeleter.h>
#include <util/DoublyLinkedList.h>

#include "Transaction.h"


class RevokeManager;


struct JournalHeader {
	uint32			magic;
	uint32			block_type;
	uint32			sequence;
	char			data[0];

	uint32			Magic()			const	
		{ return B_BENDIAN_TO_HOST_INT32(magic); }
	uint32			BlockType()		const	
		{ return B_BENDIAN_TO_HOST_INT32(block_type); }
	uint32			Sequence()		const	
		{ return B_BENDIAN_TO_HOST_INT32(sequence); }

	bool			CheckMagic()	const
		{ return Magic() == JOURNAL_MAGIC; }

	void			IncrementSequence()
		{ sequence = B_HOST_TO_BENDIAN_INT32(Sequence() + 1); }
	void			DecrementSequence()
		{ sequence = B_HOST_TO_BENDIAN_INT32(Sequence() - 1); }
	void			MakeDescriptor(uint32 sequence);
	void			MakeCommit(uint32 sequence);
} _PACKED;


struct JournalBlockTag {
	uint32			block_number;
	uint16			checksum;
	uint16			flags;

	uint32			BlockNumber()	const
		{ return B_BENDIAN_TO_HOST_INT32(block_number); }
	uint16			Flags()			const
		{ return B_BENDIAN_TO_HOST_INT16(flags); }

	void			SetBlockNumber(uint32 block)
		{ block_number = B_HOST_TO_BENDIAN_INT32(block); }
	void			SetFlags(uint16 new_flags)
		{ flags = B_HOST_TO_BENDIAN_INT16(new_flags); }
	void			SetLastTagFlag()
		{ flags |= B_HOST_TO_BENDIAN_INT16(JOURNAL_FLAG_LAST_TAG); }
	void			SetEscapedFlag()
		{ flags |= B_HOST_TO_BENDIAN_INT16(JOURNAL_FLAG_ESCAPED); }
} _PACKED;


struct JournalBlockTagV3 {
	uint32			block_number;
	uint32			flags;
	uint32			block_number_high;
	uint32			checksum;

	uint64 BlockNumber(bool has64bits) const
	{
		uint64 num = B_BENDIAN_TO_HOST_INT32(block_number);
		if (has64bits)
			num |= ((uint64)B_BENDIAN_TO_HOST_INT32(block_number_high) << 32);
		return num;
	}

	uint32			Flags()			const
		{ return B_BENDIAN_TO_HOST_INT32(flags); }

	void SetBlockNumber(uint64 block, bool has64bits)
	{
		block_number = B_HOST_TO_BENDIAN_INT32(block & 0xffffffff);
		if (has64bits)
			block_number_high = B_HOST_TO_BENDIAN_INT32(block >> 32);
	}

	void			SetFlags(uint32 new_flags)
		{ flags = B_HOST_TO_BENDIAN_INT32(new_flags); }
	void			SetLastTagFlag()
		{ flags |= B_HOST_TO_BENDIAN_INT32(JOURNAL_FLAG_LAST_TAG); }
	void			SetEscapedFlag()
		{ flags |= B_HOST_TO_BENDIAN_INT32(JOURNAL_FLAG_ESCAPED); }
} _PACKED;


struct JournalBlockTail {
	uint32			checksum;

	uint32			Checksum()	const
		{ return B_BENDIAN_TO_HOST_INT32(checksum); }

	void			SetChecksum(uint32 new_checksum)
		{ checksum = B_HOST_TO_BENDIAN_INT32(new_checksum); }
} _PACKED;


struct JournalRevokeHeader {
	JournalHeader	header;
	uint32			num_bytes;

	uint32			revoke_blocks[0];

	uint32			NumBytes()		const	
		{ return B_BENDIAN_TO_HOST_INT32(num_bytes); }
	uint32			RevokeBlock(int offset)	const	
		{ return B_BENDIAN_TO_HOST_INT32(revoke_blocks[offset]); }
} _PACKED;


struct JournalSuperBlock {
	JournalHeader	header;
	
	uint32			block_size;
	uint32			num_blocks;
	uint32			first_log_block;

	uint32			first_commit_id;
	uint32			log_start;

	uint32			error;

	uint32			compatible_features;
	uint32			incompatible_features;
	uint32			read_only_compatible_features;

	uint8			uuid[16];

	uint32			num_users;
	uint32			dynamic_superblock;

	uint32			max_transaction_blocks;
	uint32			max_transaction_data;

	uint8			checksum_type;
	uint8			padding2[3];
	uint32			padding[42];
	uint32			checksum;

	uint8			user_ids[16*48];

	uint32			BlockSize() const
		{ return B_BENDIAN_TO_HOST_INT32(block_size); }
	uint32			NumBlocks() const
		{ return B_BENDIAN_TO_HOST_INT32(num_blocks); }
	uint32			FirstLogBlock() const
		{ return B_BENDIAN_TO_HOST_INT32(first_log_block); }
	uint32			FirstCommitID() const
		{ return B_BENDIAN_TO_HOST_INT32(first_commit_id); }
	uint32			LogStart() const
		{ return B_BENDIAN_TO_HOST_INT32(log_start); }
	uint32			IncompatibleFeatures() const
		{ return B_BENDIAN_TO_HOST_INT32(incompatible_features); }
	uint32			ReadOnlyCompatibleFeatures() const
		{ return B_BENDIAN_TO_HOST_INT32(read_only_compatible_features); }
	uint32			MaxTransactionBlocks() const
		{ return B_BENDIAN_TO_HOST_INT32(max_transaction_blocks); }
	uint32			MaxTransactionData() const
		{ return B_BENDIAN_TO_HOST_INT32(max_transaction_data); }
	uint32			Checksum() const
		{ return B_BENDIAN_TO_HOST_INT32(checksum); }

	void			SetLogStart(uint32 logStart)
		{ log_start = B_HOST_TO_BENDIAN_INT32(logStart); }
	void			SetFirstCommitID(uint32 firstCommitID)
		{ first_commit_id = B_HOST_TO_BENDIAN_INT32(firstCommitID); }
	void			SetChecksum(uint32 checksum)
		{ log_start = B_HOST_TO_BENDIAN_INT32(checksum); }


} _PACKED;

class LogEntry;
class Transaction;
typedef DoublyLinkedList<LogEntry> LogEntryList;


class Journal {
public:
								Journal(Volume *fsVolume, Volume *jVolume);
	virtual						~Journal();

	virtual	status_t			InitCheck();
	virtual	status_t			Uninit();

	virtual	status_t			Recover();
	virtual	status_t			StartLog();
			status_t			RestartLog();

	virtual	status_t			Lock(Transaction* owner,
									bool separateSubTransactions);
	virtual	status_t			Unlock(Transaction* owner, bool success);

	virtual	status_t			MapBlock(off_t logical, fsblock_t& physical);
	inline	uint32				FreeLogBlocks() const;
			
			status_t			FlushLogAndBlocks();

			int32				TransactionID() const;

			Volume*				GetFilesystemVolume()
				{ return fFilesystemVolume; }
protected:
								Journal();
	
			status_t			_WritePartialTransactionToLog(
									JournalHeader* descriptorBlock,
									bool detached, uint8** escapedBlock,
									uint32& logBlock, off_t& blockNumber,
									long& cookie,
									ArrayDeleter<uint8>& escapedDataDeleter,
									uint32& blockCount, bool& finished);
	virtual status_t			_WriteTransactionToLog();

			status_t			_SaveSuperBlock();
			status_t			_LoadSuperBlock();


			Volume*				fJournalVolume;
			void*				fJournalBlockCache;
			Volume*				fFilesystemVolume;
			void*				fFilesystemBlockCache;

			recursive_lock		fLock;
			Transaction*		fOwner;

			RevokeManager*		fRevokeManager;

			status_t			fInitStatus;
			uint32				fBlockSize;
			uint32				fFirstCommitID;
			uint32				fFirstCacheCommitID;
			uint32				fFirstLogBlock;
			uint32				fLogSize;
			uint32				fVersion;
			
			bool				fIsStarted;
			uint32				fLogStart;
			uint32				fLogEnd;
			uint32				fFreeBlocks;
			uint32				fMaxTransactionSize;

			uint32				fCurrentCommitID;

			LogEntryList		fLogEntries;
			mutex				fLogEntriesLock;
			bool				fHasSubTransaction;
			bool				fSeparateSubTransactions;
			int32				fUnwrittenTransactions;
			int32				fTransactionID;

			bool				fChecksumEnabled;
			bool				fChecksumV3Enabled;
			bool				fFeature64bits;
			uint32				fChecksumSeed;

private:
			status_t			_CheckFeatures(JournalSuperBlock* superblock);

			uint32				_Checksum(JournalSuperBlock* superblock);
			bool				_Checksum(uint8 *block, bool set = false);

			uint32				_CountTags(JournalHeader *descriptorBlock);
			size_t				_TagSize();
			status_t			_RecoverPassScan(uint32& lastCommitID);
			status_t			_RecoverPassRevoke(uint32 lastCommitID);
			status_t			_RecoverPassReplay(uint32 lastCommitID);

			status_t			_FlushLog(bool canWait, bool flushBlocks);

	inline	uint32				_WrapAroundLog(uint32 block);
			
			size_t				_CurrentTransactionSize() const;
			size_t				_FullTransactionSize() const;
			size_t				_MainTransactionSize() const;
			
	virtual	status_t			_TransactionDone(bool success);

	static	void				_TransactionWritten(int32 transactionID,
									int32 event, void* _logEntry);
	static	void				_TransactionIdle(int32 transactionID,
									int32 event, void* _journal);
};

#endif	// JOURNAL_H

