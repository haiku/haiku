/*
 * Copyright 2001-2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef DEBUG_H
#define DEBUG_H


#include "system_dependencies.h"

#ifdef USER
#	define __out printf
#else
#	define __out dprintf
#endif

// Which debugger should be used when?
// The DEBUGGER() macro actually has no effect if DEBUG is not defined,
// use the DIE() macro if you really want to die.
#ifdef DEBUG
#	ifdef USER
#		define DEBUGGER(x) debugger x
#	else
#		define DEBUGGER(x) kernel_debugger x
#	endif
#else
#	define DEBUGGER(x) ;
#endif

#ifdef USER
#	define DIE(x) debugger x
#else
#	define DIE(x) kernel_debugger x
#endif

// Short overview over the debug output macros:
//	PRINT()
//		is for general messages that very unlikely should appear in a release build
//	FATAL()
//		this is for fatal messages, when something has really gone wrong
//	INFORM()
//		general information, as disk size, etc.
//	REPORT_ERROR(status_t)
//		prints out error information
//	RETURN_ERROR(status_t)
//		calls REPORT_ERROR() and return the value
//	D()
//		the statements in D() are only included if DEBUG is defined

#if DEBUG
	#define PRINT(x) { __out("bfs: "); __out x; }
	#define REPORT_ERROR(status) \
		__out("bfs: %s:%d: %s\n", __FUNCTION__, __LINE__, strerror(status));
	#define RETURN_ERROR(err) { status_t _status = err; if (_status < B_OK) REPORT_ERROR(_status); return _status;}
	#define FATAL(x) { __out("bfs: "); __out x; }
	#define INFORM(x) { __out("bfs: "); __out x; }
//	#define FUNCTION() __out("bfs: %s()\n",__FUNCTION__);
	#define FUNCTION_START(x) { __out("bfs: %s() ",__FUNCTION__); __out x; }
	#define FUNCTION() ;
//	#define FUNCTION_START(x) ;
	#define D(x) {x;};
	#ifndef ASSERT
	#	define ASSERT(x) { if (!(x)) DEBUGGER(("bfs: assert failed: " #x "\n")); }
	#endif
#else
	#define PRINT(x) ;
	#define REPORT_ERROR(status) \
		__out("bfs: %s:%d: %s\n", __FUNCTION__, __LINE__, strerror(status));
	#define RETURN_ERROR(err) { return err; }
//	#define FATAL(x) { panic x; }
	#define FATAL(x) { __out("bfs: "); __out x; }
	#define INFORM(x) { __out("bfs: "); __out x; }
	#define FUNCTION() ;
	#define FUNCTION_START(x) ;
	#define D(x) ;
	#ifndef ASSERT
	#	define ASSERT(x) { if (!(x)) DEBUGGER(("bfs: assert failed: " #x "\n")); }
//	#	define ASSERT(x) ;
	#endif
#endif

#ifdef DEBUG
	struct block_run;
	struct bplustree_header;
	struct bplustree_node;
	struct data_stream;
	struct bfs_inode;
	struct disk_super_block;
	class Inode;
	class Volume;

	// some structure dump functions
	extern void dump_block_run(const char *prefix, const block_run &run);
	extern void dump_super_block(const disk_super_block *superBlock);
	extern void dump_data_stream(const data_stream *stream);
	extern void dump_inode(const bfs_inode *inode);
	extern void dump_bplustree_header(const bplustree_header *header);
	extern void dump_bplustree_node(const bplustree_node *node,
					const bplustree_header *header = NULL, Volume *volume = NULL);
	extern void dump_block(const char *buffer, int size);
#endif
#ifdef BFS_DEBUGGER_COMMANDS
	extern void remove_debugger_commands();
	extern void add_debugger_commands();
#endif

#endif	/* DEBUG_H */
