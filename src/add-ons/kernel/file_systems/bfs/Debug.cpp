/*
 * Copyright 2001-2008, Axel Dörfler, axeld@pinc-software.de.
 * Some code is based on work previously done by Marcus Overhagen.
 *
 * This file may be used under the terms of the MIT License.
 */


#include "Debug.h"
#include "BPlusTree.h"
#include "Inode.h"

#define Print __out


char *
get_tupel(uint32 id)
{
	static unsigned char tupel[5];

	tupel[0] = 0xff & (id >> 24);
	tupel[1] = 0xff & (id >> 16);
	tupel[2] = 0xff & (id >> 8);
	tupel[3] = 0xff & (id);
	tupel[4] = 0;
	for (int16 i = 0;i < 4;i++) {
		if (tupel[i] < ' ' || tupel[i] > 128)
			tupel[i] = '.';
	}

	return (char *)tupel;
}


void
dump_block_run(const char *prefix, const block_run &run)
{
	Print("%s(%d, %d, %d)\n", prefix, (int)run.allocation_group, run.start, run.length);
}


void
dump_super_block(const disk_super_block *superBlock)
{
	Print("disk_super_block:\n");
	Print("  name           = %s\n", superBlock->name);
	Print("  magic1         = %#08x (%s) %s\n", (int)superBlock->magic1, get_tupel(superBlock->magic1), (superBlock->magic1 == SUPER_BLOCK_MAGIC1 ? "valid" : "INVALID"));
	Print("  fs_byte_order  = %#08x (%s)\n", (int)superBlock->fs_byte_order, get_tupel(superBlock->fs_byte_order));
	Print("  block_size     = %u\n", (unsigned)superBlock->block_size);
	Print("  block_shift    = %u\n", (unsigned)superBlock->block_shift);
	Print("  num_blocks     = %Lu\n", superBlock->num_blocks);
	Print("  used_blocks    = %Lu\n", superBlock->used_blocks);
	Print("  inode_size     = %u\n", (unsigned)superBlock->inode_size);
	Print("  magic2         = %#08x (%s) %s\n", (int)superBlock->magic2, get_tupel(superBlock->magic2), (superBlock->magic2 == (int)SUPER_BLOCK_MAGIC2 ? "valid" : "INVALID"));
	Print("  blocks_per_ag  = %u\n", (unsigned)superBlock->blocks_per_ag);
	Print("  ag_shift       = %u (%ld bytes)\n", (unsigned)superBlock->ag_shift, 1L << superBlock->ag_shift);
	Print("  num_ags        = %u\n", (unsigned)superBlock->num_ags);
	Print("  flags          = %#08x (%s)\n", (int)superBlock->flags, get_tupel(superBlock->flags));
	dump_block_run("  log_blocks     = ", superBlock->log_blocks);
	Print("  log_start      = %Lu\n", superBlock->log_start);
	Print("  log_end        = %Lu\n", superBlock->log_end);
	Print("  magic3         = %#08x (%s) %s\n", (int)superBlock->magic3, get_tupel(superBlock->magic3), (superBlock->magic3 == SUPER_BLOCK_MAGIC3 ? "valid" : "INVALID"));
	dump_block_run("  root_dir       = ", superBlock->root_dir);
	dump_block_run("  indices        = ", superBlock->indices);
}


void
dump_data_stream(const data_stream *stream)
{
	Print("data_stream:\n");
	for (int i = 0; i < NUM_DIRECT_BLOCKS; i++) {
		if (!stream->direct[i].IsZero()) {
			Print("  direct[%02d]                = ",i);
			dump_block_run("",stream->direct[i]);
		}
	}
	Print("  max_direct_range          = %Lu\n", stream->max_direct_range);

	if (!stream->indirect.IsZero())
		dump_block_run("  indirect                  = ", stream->indirect);

	Print("  max_indirect_range        = %Lu\n", stream->max_indirect_range);

	if (!stream->double_indirect.IsZero())
		dump_block_run("  double_indirect           = ", stream->double_indirect);

	Print("  max_double_indirect_range = %Lu\n", stream->max_double_indirect_range);
	Print("  size                      = %Lu\n", stream->size);
}


void
dump_inode(const bfs_inode *inode)
{
	Print("inode:\n");
	Print("  magic1             = %08x (%s) %s\n", (int)inode->magic1,
		get_tupel(inode->magic1), (inode->magic1 == INODE_MAGIC1 ? "valid" : "INVALID"));
	dump_block_run(	"  inode_num          = ", inode->inode_num);
	Print("  uid                = %u\n", (unsigned)inode->uid);
	Print("  gid                = %u\n", (unsigned)inode->gid);
	Print("  mode               = %08x\n", (int)inode->mode);
	Print("  flags              = %08x\n", (int)inode->flags);
	Print("  create_time        = %Ld (%Ld)\n", inode->create_time,
		inode->create_time >> INODE_TIME_SHIFT);
	Print("  last_modified_time = %Ld (%Ld)\n", inode->last_modified_time,
		inode->last_modified_time >> INODE_TIME_SHIFT);
	dump_block_run(	"  parent             = ", inode->parent);
	dump_block_run(	"  attributes         = ", inode->attributes);
	Print("  type               = %u\n", (unsigned)inode->type);
	Print("  inode_size         = %u\n", (unsigned)inode->inode_size);
	Print("  etc                = %#08x\n", (int)inode->etc);
	Print("  short_symlink      = %s\n",
		S_ISLNK(inode->mode) && (inode->flags & INODE_LONG_SYMLINK) == 0 ?
			inode->short_symlink : "-");
	dump_data_stream(&(inode->data));
	Print("  --\n  pad[0]             = %08x\n", (int)inode->pad[0]);
	Print("  pad[1]             = %08x\n", (int)inode->pad[1]);
	Print("  pad[2]             = %08x\n", (int)inode->pad[2]);
	Print("  pad[3]             = %08x\n", (int)inode->pad[3]);
}


void
dump_bplustree_header(const bplustree_header *header)
{
	Print("bplustree_header:\n");
	Print("  magic                = %#08x (%s) %s\n", (int)header->magic,
		get_tupel(header->magic), (header->magic == BPLUSTREE_MAGIC ? "valid" : "INVALID"));
	Print("  node_size            = %u\n", (unsigned)header->node_size);
	Print("  max_number_of_levels = %u\n", (unsigned)header->max_number_of_levels);
	Print("  data_type            = %u\n", (unsigned)header->data_type);
	Print("  root_node_pointer    = %Ld\n", header->root_node_pointer);
	Print("  free_node_pointer    = %Ld\n", header->free_node_pointer);
	Print("  maximum_size         = %Lu\n", header->maximum_size);
}


#define DUMPED_BLOCK_SIZE 16

void
dump_block(const char *buffer,int size)
{
	for(int i = 0;i < size;) {
		int start = i;

		for(;i < start+DUMPED_BLOCK_SIZE;i++) {
			if (!(i % 4))
				Print(" ");

			if (i >= size)
				Print("  ");
			else
				Print("%02x",*(unsigned char *)(buffer+i));
		}
		Print("  ");

		for(i = start;i < start + DUMPED_BLOCK_SIZE;i++) {
			if (i < size) {
				char c = *(buffer+i);

				if (c < 30)
					Print(".");
				else
					Print("%c",c);
			} else
				break;
		}
		Print("\n");
	}
}


void
dump_bplustree_node(const bplustree_node *node, const bplustree_header *header,
	Volume *volume)
{
	Print("bplustree_node:\n");
	Print("  left_link      = %Ld\n", node->left_link);
	Print("  right_link     = %Ld\n", node->right_link);
	Print("  overflow_link  = %Ld\n", node->overflow_link);
	Print("  all_key_count  = %u\n", node->all_key_count);
	Print("  all_key_length = %u\n", node->all_key_length);

	if (header == NULL)
		return;

	if (node->all_key_count > node->all_key_length
		|| uint32(node->all_key_count * 10) > (uint32)header->node_size
		|| node->all_key_count == 0) {
		Print("\n");
		dump_block((char *)node, header->node_size/*, sizeof(off_t)*/);
		return;
	}

	Print("\n");
	for (int32 i = 0;i < node->all_key_count;i++) {
		uint16 length;
		char buffer[256], *key = (char *)node->KeyAt(i, &length);
		if (length > 255 || length == 0) {
			Print("  %2d. Invalid length (%u)!!\n", (int)i, length);
			dump_block((char *)node, header->node_size/*, sizeof(off_t)*/);
			break;
		}
		memcpy(buffer, key, length);
		buffer[length] = '\0';

		off_t *value = node->Values() + i;
		if ((addr_t)value < (addr_t)node || (addr_t)value > (addr_t)node + header->node_size)
			Print("  %2d. Invalid Offset!!\n", (int)i);
		else {
			Print("  %2d. ", (int)i);
			if (header->data_type == BPLUSTREE_STRING_TYPE)
				Print("\"%s\"", buffer);
			else if (header->data_type == BPLUSTREE_INT32_TYPE)
				Print("int32 = %d (0x%x)", (int)*(int32 *)&buffer, (int)*(int32 *)&buffer);
			else if (header->data_type == BPLUSTREE_UINT32_TYPE)
				Print("uint32 = %u (0x%x)", (unsigned)*(uint32 *)&buffer, (unsigned)*(uint32 *)&buffer);
			else if (header->data_type == BPLUSTREE_INT64_TYPE)
				Print("int64 = %Ld (0x%Lx)", *(int64 *)&buffer, *(int64 *)&buffer);
			else
				Print("???");

			off_t offset = *value & 0x3fffffffffffffffLL;
			Print(" (%d bytes) -> %Ld", length, offset);
			if (volume != NULL) {
				block_run run = volume->ToBlockRun(offset);
				Print(" (%d, %d)", (int)run.allocation_group, run.start);
			}
			if (bplustree_node::LinkType(*value) == BPLUSTREE_DUPLICATE_FRAGMENT)
				Print(" (duplicate fragment %Ld)\n", *value & 0x3ff);
			else if (bplustree_node::LinkType(*value) == BPLUSTREE_DUPLICATE_NODE)
				Print(" (duplicate node)\n");
			else
				Print("\n");
		}
	}
}


//	#pragma mark - debugger commands


#ifdef BFS_DEBUGGER_COMMANDS


static int
dump_inode(int argc, char **argv)
{
	if (argc != 2) {
		kprintf("usage: bfsinode <ptr-to-inode>\n");
		return 0;
	}

	Inode *inode = (Inode *)parse_expression(argv[1]);
	dump_inode(&inode->Node());

	return B_OK;
}


static int
dump_volume(int argc, char **argv)
{
	if (argc != 2) {
		kprintf("usage: bfs <ptr-to-volume>\n");
		return 0;
	}

	Volume *volume = (Volume *)parse_expression(argv[1]);

	kprintf("root node: %p\n", volume->RootNode());
	kprintf("indices node: %p\n", volume->IndicesNode());

	dump_super_block(&volume->SuperBlock());

	return B_OK;
}


void
remove_debugger_commands()
{
	remove_debugger_command("bfsinode", dump_inode);
	remove_debugger_command("bfs", dump_volume);
}


void
add_debugger_commands()
{
	add_debugger_command("bfsinode", dump_inode, "dump an Inode object");
	add_debugger_command("bfs", dump_volume, "dump a BFS volume");
}


#endif	// BFS_DEBUGGER_COMMANDS

