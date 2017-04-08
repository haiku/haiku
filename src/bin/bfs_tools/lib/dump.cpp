/*
 * Copyright 2001-2010, pinc Software. All Rights Reserved.
 */

//!	BFS structure dump and helper functions

#include "BPlusTree.h"
#include "Inode.h"
#include "dump.h"

#include <File.h>
#include <Mime.h>

#include <stdio.h>
#include <string.h>

#define Print printf


char *
get_tupel(uint32 id)
{
	static unsigned char tupel[5];

	tupel[0] = 0xff & (id >> 24);
	tupel[1] = 0xff & (id >> 16);
	tupel[2] = 0xff & (id >> 8);
	tupel[3] = 0xff & (id);
	tupel[4] = 0;
	for (int16 i = 0;i < 4;i++)
		if (tupel[i] < ' ' || tupel[i] > 128)
			tupel[i] = '.';

	return (char *)tupel;
}


void
dump_block_run(const char *prefix, const block_run &run, const char *postfix)
{
	Print("%s(%" B_PRId32 ", %d, %d)%s\n", prefix, run.allocation_group,
		run.start, run.length, postfix);
}


void
dump_super_block(const disk_super_block *superBlock)
{
	Print("disk_super_block:\n");
	Print("  name           = %s\n", superBlock->name);
	Print("  magic1         = %#08" B_PRIx32 " (%s) %s\n", superBlock->magic1,
		get_tupel(superBlock->magic1),
		superBlock->magic1 == SUPER_BLOCK_MAGIC1 ? "valid" : "INVALID");
	Print("  fs_byte_order  = %#08" B_PRIx32 " (%s, %s endian)\n",
		superBlock->fs_byte_order, get_tupel(superBlock->fs_byte_order),
		superBlock->fs_byte_order == SUPER_BLOCK_FS_LENDIAN ? "little" : "big");
	Print("  block_size     = %" B_PRIu32 "\n", superBlock->block_size);
	Print("  block_shift    = %" B_PRIu32 "\n", superBlock->block_shift);
	Print("  num_blocks     = %" B_PRIdOFF "\n", superBlock->num_blocks);
	Print("  used_blocks    = %" B_PRIdOFF "\n", superBlock->used_blocks);
	Print("  inode_size     = %" B_PRId32 "\n", superBlock->inode_size);
	Print("  magic2         = %#08" B_PRIx32 " (%s) %s\n", superBlock->magic2,
		get_tupel(superBlock->magic2),
		superBlock->magic2 == (int)SUPER_BLOCK_MAGIC2 ? "valid" : "INVALID");
	Print("  blocks_per_ag  = %" B_PRId32 "\n", superBlock->blocks_per_ag);
	Print("  ag_shift       = %" B_PRId32 "\n", superBlock->ag_shift);
	Print("  num_ags        = %" B_PRId32 "\n", superBlock->num_ags);
	Print("  flags          = %#08" B_PRIx32 " (%s)\n", superBlock->flags,
		get_tupel(superBlock->flags));
	dump_block_run("  log_blocks     = ", superBlock->log_blocks);
	Print("  log_start      = %" B_PRIdOFF "\n", superBlock->log_start);
	Print("  log_end        = %" B_PRIdOFF "\n", superBlock->log_end);
	Print("  magic3         = %#08" B_PRIx32 " (%s) %s\n", superBlock->magic3,
		get_tupel(superBlock->magic3),
		superBlock->magic3 == SUPER_BLOCK_MAGIC3 ? "valid" : "INVALID");
	dump_block_run("  root_dir       = ", superBlock->root_dir);
	dump_block_run("  indices        = ", superBlock->indices);
}


void
dump_data_stream(const bfs_inode *inode, const data_stream *stream, bool showOffsets)
{
	Print("data_stream:\n");

	off_t offset = 0;

	for (int i = 0; i < NUM_DIRECT_BLOCKS; i++) {
		if (!stream->direct[i].IsZero()) {
			Print("  direct[%02d]                = ", i);

			char buffer[256];
			if (showOffsets)
				snprintf(buffer, sizeof(buffer), " %16" B_PRIdOFF, offset);
			else
				buffer[0] = '\0';

			dump_block_run("", stream->direct[i], buffer);

			offset += stream->direct[i].length * inode->inode_size;
		}
	}
	Print("  max_direct_range          = %" B_PRIdOFF "\n",
		stream->max_direct_range);

	if (!stream->indirect.IsZero())
		dump_block_run("  indirect                  = ", stream->indirect);

	Print("  max_indirect_range        = %" B_PRIdOFF "\n",
		stream->max_indirect_range);

	if (!stream->double_indirect.IsZero()) {
		dump_block_run("  double_indirect           = ",
			stream->double_indirect);
	}

	Print("  max_double_indirect_range = %" B_PRIdOFF "\n",
		stream->max_double_indirect_range);
	Print("  size                      = %" B_PRIdOFF "\n", stream->size);
}


void
dump_inode(const Inode *nameNode, const bfs_inode *inode, bool showOffsets)
{
	if (nameNode != NULL)
		Print("inode \"%s\":\n", nameNode->Name());
	else
		Print("inode:\n");

	Print("  magic1             = %08" B_PRIx32 " (%s) %s\n",inode->magic1,
		get_tupel(inode->magic1),
		(inode->magic1 == INODE_MAGIC1 ? "valid" : "INVALID"));
	dump_block_run(	"  inode_num          = ",inode->inode_num);
	Print("  uid                = %" B_PRIu32 "\n",inode->uid);
	Print("  gid                = %" B_PRIu32 "\n",inode->gid);
	Print("  mode               = %10" B_PRIo32 " (octal)\n",inode->mode);
	Print("  flags              = %08" B_PRIx32 "\n",inode->flags);

	time_t time;
	time = (time_t)(inode->create_time >> 16);
	Print("  create_time        = %s",ctime(&time));
	time = (time_t)(inode->last_modified_time >> 16);
	Print("  last_modified_time = %s",ctime(&time));

	dump_block_run(	"  parent             = ",inode->parent);
	dump_block_run(	"  attributes         = ",inode->attributes);
	Print("  type               = %" B_PRIu32 "\n",inode->type);
	Print("  inode_size         = %" B_PRId32 "\n",inode->inode_size);
	Print("  etc                = %#08" B_PRIx32 "\n",inode->etc);
	Print("  short_symlink      = %s\n",
		S_ISLNK(inode->mode) && (inode->flags & INODE_LONG_SYMLINK) == 0
			? inode->short_symlink : "-");

	dump_data_stream(inode, &inode->data, showOffsets);
	Print("  --\n");
#if 0
	Print("  --\n  pad[0]             = %08lx\n", inode->pad[0]);
	Print("  pad[1]             = %08lx\n", inode->pad[1]);
	Print("  pad[2]             = %08lx\n", inode->pad[2]);
	Print("  pad[3]             = %08lx\n", inode->pad[3]);
#endif
}


void
dump_small_data(Inode *inode)
{
	if (inode == NULL || inode->InodeBuffer() == NULL)
		return;

	small_data *item = NULL;

	printf("small data section (max. %ld bytes):\n",
		inode->InodeBuffer()->inode_size - sizeof(struct bfs_inode));

	while (inode->GetNextSmallData(&item) == B_OK) {
		printf("%#08" B_PRIx32 " (%s), name = \"%s\", ", item->type,
			get_tupel(item->type), item->Name());
		if (item->type == FILE_NAME_TYPE
			|| item->type == B_STRING_TYPE
			|| item->type == B_MIME_STRING_TYPE)
			printf("data = \"%s\", ", item->Data());

		printf("%u bytes\n", item->data_size);
	}
}


void
dump_bplustree_header(const bplustree_header* header)
{
	printf("bplustree_header:\n");
	printf("  magic                = %#08" B_PRIx32 " (%s) %s\n",
		header->magic, get_tupel(header->magic),
		header->magic == BPLUSTREE_MAGIC ? "valid" : "INVALID");
	printf("  node_size            = %" B_PRIu32 "\n", header->node_size);
	printf("  max_number_of_levels = %" B_PRIu32 "\n",
		header->max_number_of_levels);
	printf("  data_type            = %" B_PRIu32 "\n", header->data_type);
	printf("  root_node_pointer    = %" B_PRIdOFF "\n",
		header->root_node_pointer);
	printf("  free_node_pointer    = %" B_PRIdOFF "\n",
		header->free_node_pointer);
	printf("  maximum_size         = %" B_PRIdOFF "\n",
		header->maximum_size);
}


void
dump_bplustree_node(const bplustree_node* node, const bplustree_header* header,
	Disk* disk)
{
	Print("bplustree_node (%s node):\n",
		node->overflow_link == BPLUSTREE_NULL ? "leaf" : "index");
	Print("  left_link      = %" B_PRIdOFF "\n", node->left_link);
	Print("  right_link     = %" B_PRIdOFF "\n", node->right_link);
	Print("  overflow_link  = %" B_PRIdOFF "\n", node->overflow_link);
	Print("  all_key_count  = %u\n", node->all_key_count);
	Print("  all_key_length = %u\n", node->all_key_length);

	if (header == NULL)
		return;

	if (node->all_key_count > node->all_key_length
		|| uint32(node->all_key_count * 10) > (uint32)header->node_size) {
		Print("\n");
		dump_block((char *)node, header->node_size, sizeof(off_t));
		return;
	}

	Print("\n");
	for (int32 i = 0;i < node->all_key_count;i++) {
		uint16 length;
		char* key = (char *)node->KeyAt(i, &length);
		if (length > BPLUSTREE_MAX_KEY_LENGTH) {
			Print("  %2" B_PRId32 ". Invalid length (%u)!!\n", i, length);
			dump_block((char *)node, header->node_size, sizeof(off_t));
			break;
		}

		char buffer[256];
		memcpy(buffer, key, length);
		buffer[length] = '\0';

		off_t *value = node->Values() + i;
		if ((addr_t)value < (addr_t)node
			|| (addr_t)value > (addr_t)node + header->node_size) {
			Print("  %2" B_PRId32 ". Invalid Offset!!\n", i);
		} else {
			Print("  %2" B_PRId32 ". ",i);
			if (header->data_type == BPLUSTREE_STRING_TYPE)
				Print("\"%s\"",buffer);
			else if (header->data_type == BPLUSTREE_INT32_TYPE) {
				Print("int32 = %" B_PRId32 " (0x%" B_PRIx32 ")",
					*(int32 *)&buffer, *(int32 *)&buffer);
			} else if (header->data_type == BPLUSTREE_UINT32_TYPE) {
				Print("uint32 = %" B_PRIu32 " (0x%" B_PRIx32 ")",
					*(uint32 *)&buffer, *(uint32 *)&buffer);
			} else if (header->data_type == BPLUSTREE_INT64_TYPE) {
				Print("int64 = %" B_PRId64 " (0x%" B_PRIx64 ")",
					*(int64 *)&buffer, *(int64 *)&buffer);
			} else
				Print("???");

			off_t offset = *value & 0x3fffffffffffffffLL;
			Print(" (%d bytes) -> %" B_PRIdOFF,length,offset);
			if (disk != NULL) {
				block_run run = disk->ToBlockRun(offset);
				Print(" (%" B_PRId32 ", %d)", run.allocation_group, run.start);
			}
			if (bplustree_node::LinkType(*value)
					== BPLUSTREE_DUPLICATE_FRAGMENT) {
				Print(" (duplicate fragment %" B_PRIdOFF ")\n",
					*value & 0x3ff);
			} else if (bplustree_node::LinkType(*value)
					== BPLUSTREE_DUPLICATE_NODE) {
				Print(" (duplicate node)\n");
			} else
				Print("\n");
		}
	}
}


void
dump_block(const char *buffer, uint32 size, int8 valueSize)
{
	const uint32 kBlockSize = 16;

	for (uint32 i = 0; i < size;) {
		uint32 start = i;

		for (; i < start + kBlockSize; i++) {
			if (!(i % 4))
				Print(" ");

			if (i >= size)
				Print("  ");
			else
				Print("%02x", *(unsigned char *)(buffer + i));
		}
		Print("  ");

		for (i = start; i < start + kBlockSize; i++) {
			if (i < size) {
				char c = *(buffer + i);

				if (c < 30)
					Print(".");
				else
					Print("%c",c);
			}
			else
				break;
		}

		if (valueSize > 0) {
			Print("  (");
			for (uint32 offset = start; offset < start + kBlockSize;
					offset += valueSize) {
				if (valueSize == sizeof(off_t))
					Print("%s%" B_PRIdOFF, offset == start ? "" : ", ",
						*(off_t *)(buffer + offset));
			}
			Print(")");
		}

		Print("\n");
	}
}

