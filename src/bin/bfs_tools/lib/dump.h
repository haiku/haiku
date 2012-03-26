/*
 * Copyright 2001-2010 pinc Software. All Rights Reserved.
 */
#ifndef DUMP_H
#define DUMP_H


#include "bfs.h"


struct bplustree_node;
struct bplustree_header;
class Inode;


extern void dump_block_run(const char* prefix, const block_run& run,
	const char *postfix = NULL);
extern void dump_super_block(const disk_super_block* superBlock);
extern void dump_data_stream(const bfs_inode* inode, const data_stream* stream,
	bool showOffsets = false);
extern void dump_small_data(Inode* inode);
extern void dump_inode(const Inode* node, const bfs_inode* inode,
	bool showOffsets = false);
extern char* get_tupel(uint32 id);

extern void dump_bplustree_header(const bplustree_header* header);
extern void dump_bplustree_node(const bplustree_node* node,
	const bplustree_header* header, Disk* disk = NULL);

extern void dump_block(const char *buffer, uint32 size, int8 valueSize = 0);

#endif	/* DUMP_H */
