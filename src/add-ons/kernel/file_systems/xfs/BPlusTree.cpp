#include "BPlusTree.h"

void bplustree_short_block::SwapEndian()
{
	bb_magic = B_BENDIAN_TO_HOST_INT32(bb_magic);
	bb_level = B_BENDIAN_TO_HOST_INT16(bb_level);
	bb_numrecs = B_BENDIAN_TO_HOST_INT16(bb_numrecs);
	bb_leftsib = B_BENDIAN_TO_HOST_INT32(bb_leftsib);
	bb_rightsib = B_BENDIAN_TO_HOST_INT32(bb_rightsib);
}


void bplustree_long_block::SwapEndian()
{
	bb_magic = B_BENDIAN_TO_HOST_INT32(bb_magic);
	bb_level = B_BENDIAN_TO_HOST_INT16(bb_level);
	bb_numrecs = B_BENDIAN_TO_HOST_INT16(bb_numrecs);
	bb_leftsib = B_BENDIAN_TO_HOST_INT64(bb_leftsib);
	bb_rightsib = B_BENDIAN_TO_HOST_INT64(bb_rightsib);
}


void xfs_alloc_rec::SwapEndian()
{
	ar_startblock = B_BENDIAN_TO_HOST_INT32(ar_startblock);
	ar_blockcount =  B_BENDIAN_TO_HOST_INT32(ar_blockcount);
}


uint32 BPlusTree::BlockSize(){
	return fVolume.SuperBlock().BlockSize();
}


int BPlusTree::RecordSize(){
	if (fRecType == ALLOC_FLAG)
		return XFS_ALLOC_REC_SIZE;
}


int BPlusTree::MaxRecords(bool leaf){
	int blockLen = BlockSize();
	
	if (fPtrType == SHORT_BLOCK_FLAG)
		blockLen - XFS_BTREE_SBLOCK_SIZE;

	if (leaf){
		if (fRecType == ALLOC_FLAG)
			return blockLen/sizeof(xfs_alloc_rec_t);
	}
	else {
		if (fKeyType) == ALLOC_FLAG){
			return blockLen/(sizeof(xfs_alloc_key_t)
							+ sizeof(xfs_alloc_ptr_t));
		}
	}
}


int BPlusTree::KeyLen(){
	if (fKeyType == ALLOC_FLAG) return XFS_ALLOC_REC_SIZE;
}


int BPlusTree::BlockLen(){
	if(fPtrType == LONG_BLOCK_FLAG) return XFS_BTREE_LBLOCK_SIZE;
	else return XFS_BTREE_SBLOCK_SIZE;
}


int BPlusTree::PtrLen(){
	if(fPtrType == LONG_BLOCK_FLAG) return sizeof(uint64);
	else return sizeof(uint32);
}


int BPlusTree::RecordOffset(int pos){
	return BlockLen() + (pos-1)*RecordSize();
}


int BPlusTree::KeyOffset(int pos){
	return BlockLen() + (pos-1)*KeyLen();
}


int BPlusTree::PtrOffset(int pos, int level){
	return BlockLen() + MaxRecords(level>0)*KeyLen() + (pos-1)*PtrLen();
}
