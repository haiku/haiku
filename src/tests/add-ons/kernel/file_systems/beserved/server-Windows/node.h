#ifndef _NODE_H_
#define _NODE_H_

#include "beCompat.h"
#include "betalk.h"

typedef struct btnode
{
	vnode_id vnid;
	char name[B_FILE_NAME_LENGTH];
	int refCount;
	bool invalid;
	struct btnode *next;
	struct btnode *prev;
	struct btnode *parent;
} bt_node;

typedef bt_node caddr_t;

int compareNodeVnid(caddr_t node1, caddr_t node2);
int compareNodeName(caddr_t node1, caddr_t node2);

#endif
