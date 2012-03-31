/*
 * Copyright 2001-2009, Axel Dörfler, axeld@pinc-software.de.
 * Some code is based on work previously done by Marcus Overhagen.
 *
 * This file may be used under the terms of the MIT License.
 */


#include "Debug.h"
#include "BlockAllocator.h"
#include "BPlusTree.h"
#include "Inode.h"
#include "Journal.h"


char*
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

	return (char*)tupel;
}


void
dump_block_run(const char* prefix, const block_run& run)
{
	kprintf("%s(%d, %d, %d)\n", prefix, (int)run.allocation_group, run.start,
		run.length);
}


void
dump_super_block(const disk_super_block* superBlock)
{
	kprintf("disk_super_block:\n");
	kprintf("  name           = %s\n", superBlock->name);
	kprintf("  magic1         = %#08x (%s) %s\n", (int)superBlock->Magic1(),
		get_tupel(superBlock->magic1),
		(superBlock->magic1 == SUPER_BLOCK_MAGIC1 ? "valid" : "INVALID"));
	kprintf("  fs_byte_order  = %#08x (%s)\n", (int)superBlock->fs_byte_order,
		get_tupel(superBlock->fs_byte_order));
	kprintf("  block_size     = %u\n", (unsigned)superBlock->BlockSize());
	kprintf("  block_shift    = %u\n", (unsigned)superBlock->BlockShift());
	kprintf("  num_blocks     = %" B_PRIdOFF "\n", superBlock->NumBlocks());
	kprintf("  used_blocks    = %" B_PRIdOFF "\n", superBlock->UsedBlocks());
	kprintf("  inode_size     = %u\n", (unsigned)superBlock->InodeSize());
	kprintf("  magic2         = %#08x (%s) %s\n", (int)superBlock->Magic2(),
		get_tupel(superBlock->magic2),
		(superBlock->magic2 == (int)SUPER_BLOCK_MAGIC2 ? "valid" : "INVALID"));
	kprintf("  blocks_per_ag  = %u\n",
		(unsigned)superBlock->BlocksPerAllocationGroup());
	kprintf("  ag_shift       = %u (%ld bytes)\n",
		(unsigned)superBlock->AllocationGroupShift(),
		1L << superBlock->AllocationGroupShift());
	kprintf("  num_ags        = %u\n", (unsigned)superBlock->AllocationGroups());
	kprintf("  flags          = %#08x (%s)\n", (int)superBlock->Flags(),
		get_tupel(superBlock->Flags()));
	dump_block_run("  log_blocks     = ", superBlock->log_blocks);
	kprintf("  log_start      = %" B_PRIdOFF "\n", superBlock->LogStart());
	kprintf("  log_end        = %" B_PRIdOFF "\n", superBlock->LogEnd());
	kprintf("  magic3         = %#08x (%s) %s\n", (int)superBlock->Magic3(),
		get_tupel(superBlock->magic3),
		(superBlock->magic3 == SUPER_BLOCK_MAGIC3 ? "valid" : "INVALID"));
	dump_block_run("  root_dir       = ", superBlock->root_dir);
	dump_block_run("  indices        = ", superBlock->indices);
}


void
dump_data_stream(const data_stream* stream)
{
	kprintf("data_stream:\n");
	for (int i = 0; i < NUM_DIRECT_BLOCKS; i++) {
		if (!stream->direct[i].IsZero()) {
			kprintf("  direct[%02d]                = ", i);
			dump_block_run("", stream->direct[i]);
		}
	}
	kprintf("  max_direct_range          = %" B_PRIdOFF "\n",
		stream->MaxDirectRange());

	if (!stream->indirect.IsZero())
		dump_block_run("  indirect                  = ", stream->indirect);

	kprintf("  max_indirect_range        = %" B_PRIdOFF "\n",
		stream->MaxIndirectRange());

	if (!stream->double_indirect.IsZero()) {
		dump_block_run("  double_indirect           = ",
			stream->double_indirect);
	}

	kprintf("  max_double_indirect_range = %" B_PRIdOFF "\n",
		stream->MaxDoubleIndirectRange());
	kprintf("  size                      = %" B_PRIdOFF "\n", stream->Size());
}


void
dump_inode(const bfs_inode* inode)
{
	kprintf("inode:\n");
	kprintf("  magic1             = %08x (%s) %s\n", (int)inode->Magic1(),
		get_tupel(inode->magic1),
		(inode->magic1 == INODE_MAGIC1 ? "valid" : "INVALID"));
	dump_block_run(	"  inode_num          = ", inode->inode_num);
	kprintf("  uid                = %u\n", (unsigned)inode->UserID());
	kprintf("  gid                = %u\n", (unsigned)inode->GroupID());
	kprintf("  mode               = %08x\n", (int)inode->Mode());
	kprintf("  flags              = %08x\n", (int)inode->Flags());
	kprintf("  create_time        = %" B_PRIx64 " (%" B_PRIdTIME ".%u)\n",
		inode->CreateTime(), bfs_inode::ToSecs(inode->CreateTime()),
		(unsigned)bfs_inode::ToNsecs(inode->CreateTime()));
	kprintf("  last_modified_time = %" B_PRIx64 " (%" B_PRIdTIME ".%u)\n",
		inode->LastModifiedTime(), bfs_inode::ToSecs(inode->LastModifiedTime()),
		(unsigned)bfs_inode::ToNsecs(inode->LastModifiedTime()));
	kprintf("  status_change_time = %" B_PRIx64 " (%" B_PRIdTIME ".%u)\n",
		inode->StatusChangeTime(), bfs_inode::ToSecs(inode->StatusChangeTime()),
		(unsigned)bfs_inode::ToNsecs(inode->StatusChangeTime()));
	dump_block_run(	"  parent             = ", inode->parent);
	dump_block_run(	"  attributes         = ", inode->attributes);
	kprintf("  type               = %u\n", (unsigned)inode->Type());
	kprintf("  inode_size         = %u\n", (unsigned)inode->InodeSize());
	kprintf("  short_symlink      = %s\n",
		S_ISLNK(inode->Mode()) && (inode->Flags() & INODE_LONG_SYMLINK) == 0
			? inode->short_symlink : "-");
	dump_data_stream(&(inode->data));
	kprintf("  --\n  pad[0]             = %08x\n", (int)inode->pad[0]);
	kprintf("  pad[1]             = %08x\n", (int)inode->pad[1]);
}


void
dump_bplustree_header(const bplustree_header* header)
{
	kprintf("bplustree_header:\n");
	kprintf("  magic                = %#08x (%s) %s\n", (int)header->Magic(),
		get_tupel(header->magic),
		(header->magic == BPLUSTREE_MAGIC ? "valid" : "INVALID"));
	kprintf("  node_size            = %u\n", (unsigned)header->NodeSize());
	kprintf("  max_number_of_levels = %u\n",
		(unsigned)header->MaxNumberOfLevels());
	kprintf("  data_type            = %u\n", (unsigned)header->DataType());
	kprintf("  root_node_pointer    = %" B_PRIdOFF "\n", header->RootNode());
	kprintf("  free_node_pointer    = %" B_PRIdOFF "\n", header->FreeNode());
	kprintf("  maximum_size         = %" B_PRIdOFF "\n", header->MaximumSize());
}


#define DUMPED_BLOCK_SIZE 16

void
dump_block(const char* buffer,int size)
{
	for (int i = 0; i < size;) {
		int start = i;

		for (; i < start + DUMPED_BLOCK_SIZE; i++) {
			if (!(i % 4))
				kprintf(" ");

			if (i >= size)
				kprintf("  ");
			else
				kprintf("%02x", *(unsigned char *)(buffer + i));
		}
		kprintf("  ");

		for (i = start; i < start + DUMPED_BLOCK_SIZE; i++) {
			if (i < size) {
				char c = *(buffer + i);

				if (c < 30)
					kprintf(".");
				else
					kprintf("%c", c);
			} else
				break;
		}
		kprintf("\n");
	}
}


void
dump_bplustree_node(const bplustree_node* node, const bplustree_header* header,
	Volume* volume)
{
	kprintf("bplustree_node:\n");
	kprintf("  left_link      = %" B_PRId64 "\n", node->left_link);
	kprintf("  right_link     = %" B_PRId64 "\n", node->right_link);
	kprintf("  overflow_link  = %" B_PRId64 "\n", node->overflow_link);
	kprintf("  all_key_count  = %u\n", node->all_key_count);
	kprintf("  all_key_length = %u\n", node->all_key_length);

	if (header == NULL)
		return;

	if (node->all_key_count > node->all_key_length
		|| uint32(node->all_key_count * 10) > (uint32)header->node_size
		|| node->all_key_count == 0) {
		kprintf("\n");
		dump_block((char *)node, header->node_size/*, sizeof(off_t)*/);
		return;
	}

	kprintf("\n");
	for (int32 i = 0;i < node->all_key_count;i++) {
		uint16 length;
		char buffer[256], *key = (char *)node->KeyAt(i, &length);
		if (length > 255 || length == 0) {
			kprintf("  %2d. Invalid length (%u)!!\n", (int)i, length);
			dump_block((char *)node, header->node_size/*, sizeof(off_t)*/);
			break;
		}
		memcpy(buffer, key, length);
		buffer[length] = '\0';

		off_t* value = node->Values() + i;
		if ((addr_t)value < (addr_t)node
			|| (addr_t)value > (addr_t)node + header->node_size)
			kprintf("  %2d. Invalid Offset!!\n", (int)i);
		else {
			kprintf("  %2d. ", (int)i);
			if (header->data_type == BPLUSTREE_STRING_TYPE)
				kprintf("\"%s\"", buffer);
			else if (header->data_type == BPLUSTREE_INT32_TYPE) {
				kprintf("int32 = %d (0x%x)", (int)*(int32 *)&buffer,
					(int)*(int32 *)&buffer);
			} else if (header->data_type == BPLUSTREE_UINT32_TYPE) {
				kprintf("uint32 = %u (0x%x)", (unsigned)*(uint32 *)&buffer,
					(unsigned)*(uint32 *)&buffer);
			} else if (header->data_type == BPLUSTREE_INT64_TYPE) {
				kprintf("int64 = %" B_PRId64 " (%#" B_PRIx64 ")",
					*(int64 *)&buffer, *(int64 *)&buffer);
			} else
				kprintf("???");

			off_t offset = *value & 0x3fffffffffffffffLL;
			kprintf(" (%d bytes) -> %" B_PRIdOFF, length, offset);
			if (volume != NULL) {
				block_run run = volume->ToBlockRun(offset);
				kprintf(" (%d, %d)", (int)run.allocation_group, run.start);
			}
			if (bplustree_node::LinkType(*value)
					== BPLUSTREE_DUPLICATE_FRAGMENT) {
				kprintf(" (duplicate fragment %" B_PRIdOFF ")\n",
					*value & 0x3ff);
			} else if (bplustree_node::LinkType(*value)
					== BPLUSTREE_DUPLICATE_NODE) {
				kprintf(" (duplicate node)\n");
			} else
				kprintf("\n");
		}
	}
}


//	#pragma mark - debugger commands


#ifdef BFS_DEBUGGER_COMMANDS


static int
dump_inode(int argc, char** argv)
{
	bool block = false;
	if (argc >= 3 && !strcmp(argv[1], "-b"))
		block = true;

	if (argc > 3 || !strcmp(argv[1], "--help")) {
		kprintf("usage: bfsinode [-b] <ptr-to-inode> [<offset>]\n"
			"  -b the address is regarded as pointer to a block instead of one "
			"to an inode.\n"
			"  If <offset> is given, the block_run containing the offset in "
			"the data stream is printed -- this does not work with -b.\n");
		return 0;
	}

	addr_t address = parse_expression(argv[block ? 2 : 1]);
	bfs_inode* node;
	if (block)
		node = (bfs_inode*)address;
	else {
		Inode* inode = (Inode*)address;

		if (argc == 3) {
			off_t offset = parse_expression(argv[2]);
			if (offset < 0) {
				kprintf("invalid offset %" B_PRIdOFF "\n", offset);
				return 0;
			}
			off_t baseOffset;
			block_run run;
			status_t status = inode->FindBlockRun(offset, run, baseOffset);
			if (status != B_OK) {
				kprintf("find block run failed: %s\n", strerror(status));
				return 0;
			}

			dump_block_run("block run:   ", run);
			kprintf("base offset: %" B_PRIdOFF "\n", baseOffset);
			return 0;
		}

		kprintf("INODE %p\n", inode);
		kprintf("  rw lock:           %p\n", &inode->Lock());
		kprintf("  tree:              %p\n", inode->Tree());
		kprintf("  file cache:        %p\n", inode->FileCache());
		kprintf("  file map:          %p\n", inode->Map());
		kprintf("  old size:          %Ld\n", inode->OldSize());
		kprintf("  old last modified: %Ld\n", inode->OldLastModified());

		node = &inode->Node();
	}

	dump_inode(node);
	return 0;
}


static int
dump_volume(int argc, char** argv)
{
	if (argc < 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: bfs <ptr-to-volume> [<block-run>]\n"
			"Dumps a BFS volume - <block-run> is given, it is converted to a "
			"block offset instead (and vice versa).\n");
		return 0;
	}

	Volume* volume = (Volume*)parse_expression(argv[1]);

	if (argc > 2) {
		// convert block_runs/offsets
		for (int i = 2; i < argc; i++) {
			char* arg = argv[i];
			if (strchr(arg, '.') != NULL || strchr(arg, ',') != NULL) {
				// block_run to offset
				block_run run;
				run.allocation_group = HOST_ENDIAN_TO_BFS_INT32(
					strtoul(arg, &arg, 0));
				run.start = HOST_ENDIAN_TO_BFS_INT16(strtoul(arg + 1, NULL, 0));
				run.length = 0;

				kprintf("%ld.%u -> block %Ld, bitmap block %ld\n",
					run.AllocationGroup(), run.Start(), volume->ToBlock(run),
					volume->SuperBlock().BlocksPerAllocationGroup()
						* run.AllocationGroup() + 1);
			} else {
				// offset to block_run
				off_t offset = parse_expression(arg);
				block_run run = volume->ToBlockRun(offset);

				kprintf("block %Ld -> %ld.%u, bitmap block %ld\n", offset,
					run.AllocationGroup(), run.Start(),
					volume->SuperBlock().BlocksPerAllocationGroup()
						* run.AllocationGroup() + 1);
			}
		}
		return 0;
	}

	kprintf("id:           %ld\n", volume->ID());
	kprintf("block cache:  %p\n", volume->BlockCache());
	kprintf("journal:      %p\n", volume->GetJournal(0));
	kprintf("allocator:    %p\n", &volume->Allocator());
	kprintf("root node:    %p\n", volume->RootNode());
	kprintf("indices node: %p\n\n", volume->IndicesNode());

	dump_super_block(&volume->SuperBlock());

	set_debug_variable("_cache", (addr_t)volume->BlockCache());
	set_debug_variable("_root", (addr_t)volume->RootNode());
	set_debug_variable("_indices", (addr_t)volume->IndicesNode());

	return 0;
}


static int
dump_block_run_array(int argc, char** argv)
{
	if (argc < 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s <ptr-to-array> [number-of-runs]\n", argv[0]);
		return 0;
	}

	const block_run* runs = (const block_run*)parse_expression(argv[1]);
	uint32 count = 16;
	if (argc > 2)
		count = parse_expression(argv[2]);

	for (uint32 i = 0; i < count; i++) {
		dprintf("[%3lu]  ", i);
		dump_block_run("", runs[i]);
	}

	return 0;
}


static int
dump_bplustree_node(int argc, char** argv)
{
	if (argc < 2 || argc > 4 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s <ptr-to-node> [ptr-to-header] [ptr-to-volume]\n",
			argv[0]);
		return 0;
	}

	bplustree_node* node = (bplustree_node*)parse_expression(argv[1]);
	bplustree_header* header = NULL;
	Volume* volume = NULL;

	if (argc > 2)
		header = (bplustree_header*)parse_expression(argv[2]);
	if (argc > 3)
		volume = (Volume*)parse_expression(argv[3]);

	dump_bplustree_node(node, header, volume);

	return 0;
}


static int
dump_bplustree_header(int argc, char** argv)
{
	if (argc != 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s <ptr-to-header>\n", argv[0]);
		return 0;
	}

	bplustree_header* header = (bplustree_header*)parse_expression(argv[1]);
	dump_bplustree_header(header);

	return 0;
}


void
remove_debugger_commands()
{
	remove_debugger_command("bfs_inode", dump_inode);
	remove_debugger_command("bfs_allocator", dump_block_allocator);
#if BFS_TRACING
	remove_debugger_command("bfs_allocator_blocks",
		dump_block_allocator_blocks);
#endif
	remove_debugger_command("bfs_journal", dump_journal);
	remove_debugger_command("bfs_btree_header", dump_bplustree_header);
	remove_debugger_command("bfs_btree_node", dump_bplustree_node);
	remove_debugger_command("bfs", dump_volume);
	remove_debugger_command("bfs_block_runs", dump_block_run_array);
}


void
add_debugger_commands()
{
	add_debugger_command("bfs_inode", dump_inode, "dump an Inode object");
	add_debugger_command("bfs_allocator", dump_block_allocator,
		"dump a BFS block allocator");
#if BFS_TRACING
	add_debugger_command("bfs_allocator_blocks", dump_block_allocator_blocks,
		"dump a BFS block allocator actions that affected a certain block");
#endif
	add_debugger_command("bfs_journal", dump_journal,
		"dump the journal log entries");
	add_debugger_command("bfs_btree_header", dump_bplustree_header,
		"dump a BFS B+tree header");
	add_debugger_command("bfs_btree_node", dump_bplustree_node,
		"dump a BFS B+tree node");
	add_debugger_command("bfs", dump_volume, "dump a BFS volume");
	add_debugger_command("bfs_block_runs", dump_block_run_array,
		"dump a block run array");
}


#endif	// BFS_DEBUGGER_COMMANDS
