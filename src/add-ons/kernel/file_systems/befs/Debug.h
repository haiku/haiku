#ifndef DEBUG_H
#define DEBUG_H
/* Debug - debug stuff
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/


#include <KernelExport.h>
#ifdef USER
#	include <stdio.h>
#	define __out printf
#else
#	include <null.h>
#	define __out dprintf
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

#ifdef DEBUG
	#define PRINT(x) { __out("bfs: "); __out x; }
	#define REPORT_ERROR(status) __out("bfs: %s:%ld: %s\n",__FUNCTION__,__LINE__,strerror(status));
	#define RETURN_ERROR(err) { status_t _status = err; if (_status < B_OK) REPORT_ERROR(_status); return _status;}
	#define FATAL(x) { __out("bfs: "); __out x; }
	#define INFORM(x) { __out("bfs: "); __out x; }
//	#define FUNCTION() __out("bfs: %s()\n",__FUNCTION__);
	#define FUNCTION_START(x) { __out("bfs: %s() ",__FUNCTION__); __out x; }
	#define FUNCTION() ;
//	#define FUNCTION_START(x) ;
	#define D(x) {x;};
#else
	#define PRINT(x) ;
	#define REPORT_ERROR(status) ;
	#define RETURN_ERROR(status) return status;
	#define FATAL(x) { __out("bfs: "); __out x; }
	#define INFORM(x) { __out("bfs: "); __out x; }
	#define FUNCTION() ;
	#define FUNCTION_START(x) ;
	#define D(x) ;
#endif

#ifdef DEBUG
	struct block_run;
	struct bplustree_header;
	struct bplustree_node;
	struct data_stream;
	struct bfs_inode;
	struct disk_super_block;
	class Volume;
	
	// some structure dump functions
	extern void dump_block_run(const char *prefix, block_run &run);
	extern void dump_super_block(disk_super_block *superBlock);
	extern void dump_data_stream(data_stream *stream);
	extern void dump_inode(bfs_inode *inode);
	extern void dump_bplustree_header(bplustree_header *header);
	extern void dump_bplustree_node(bplustree_node *node,bplustree_header *header = NULL,Volume *volume = NULL);
	extern void dump_block(const char *buffer, int size);
#endif

#endif	/* DEBUG_H */
