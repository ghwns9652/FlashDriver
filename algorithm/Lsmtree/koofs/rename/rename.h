#ifndef __RENAME_H__
#define __RENAME_H__

#include "libtree.h"

#define MAXDEPTH	10
#define MAXLEVEL	7
#define MAXPATH		256

int rnode_cmp (const struct avltree_node *, const struct avltree_node *);

typedef struct rnode {
	char newpath[MAXPATH];
	char oldpath[MAXLEVEL][MAXPATH];
	bool ismodified;
	struct rbtree_node rb_node;
	struct rbtree *tree
} rnode;

typedef struct renameMap {
	int minDepth;
	struct rbtree tree;
} renameMap;

#endif
