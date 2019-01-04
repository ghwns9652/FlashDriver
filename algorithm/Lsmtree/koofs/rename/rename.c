#include <stdio.h>
#include <stdlib.h>
#include "rename.h"
#include "vsizekey.h"

/*
 * /a/b/c/d
 * / => depth: 0
 * a => depth: 1
 * b => depth: 2
 * c => depth: 3
 * d => depth: 4
 */

rnode *rmap_root;

/* return value must be freed after used */
int get_path_array (const char *path, char **path_array) {
	int i,j=0;
	int len = strlen(path);

	path_array[2] = path;
	if (len <= 3) {
		return 0;
	}

	for (i=2; i < len; i++) {
		if (path[i] == '/') {
			path_array[++j] = path+i+1;
		}
	}

	return j;
}

/* depth > 0 */
char *get_path (const char *path, int depth) {
	int i,j=0;
	int len = strlen(path);

	if (len <= 3) {
		return NULL;;
	}

	for (i=2; i < len; i++) {
		if (path[i] == '/' && --depth == 0) {
				break;
			}
		}
	}

	return path+i+1;
}

int rnode_cmp (const struct rbtree_node *a, const struct rbtree_node *b) {
	rnode *p = rbtree_container_of(a, rnode, rb_node);
	rnode *q = rbtree_container_of(b, rnode, rb_node);

	return vsizekey_cmp(p->newpath, q->newpath);
}

void alloc_rnode (rnode *node) {
	node = (rnode*)calloc(1,sizeof(rnode));
	tree = (struct rbtree*)malloc(sizeof(struct rbtree));
	rbtree_init(node->tree, rnode_cmp, 0);
}

void free_rnode (rnode *node) {
	if (node->tree) 
		free(node->tree);
	if (node) 
		free(node);
}

/* oldpath, newpath must not be '/' */
int renameMap_new2old (const char *newpath) {
	int newpath_depth;
	char **newpath_array = (char**)malloc(sizeof(char*) * MAXDEPTH);

	newpath_depth = get_path_array(newpath, newpath_array);

	if (newpath_depth == 0) {
		printf("depth must be non zero\n");
		return -1;
	}

	rnode *input = NULL;
	rnode *parent = rmap_root;
	struct rbtree_node *res;
	for (int i=1; i <= newpath_depth; i++) {
		if (!input) {
			alloc_rnode(input);
		}
		if (newpath_depth != i) {
			memcpy(input->newpath, newpath_array[i], newpath_array[i+1] - newpath_array[i] - 1);
		} else {
			memcpy(input->newpath, newpath_array[i], newpath + strlen(newpath) - &newpath_array[i]);
		}

		res = rbtree_lookup(input->rb_node, parent->tree);
		if (res) {	// found

		} else {	// not found

		}
	}
}

/* oldpath, newpath must not be '/' */
int renameMap_insert (const char *oldpath, const char *newpath) {
	int oldpath_depth, newpath_depth, depth_diff;
	char **oldpath_array = (char**)malloc(sizeof(char*) * MAXDEPTH);
	char **newpath_array = (char**)malloc(sizeof(char*) * MAXDEPTH);

	oldpath_depth = get_path_array(oldpath, oldpath_array);
	newpath_depth = get_path_array(newpath, newpath_array);
	depth_diff = oldpath_depth > newpath_depth ? oldpath_depth - newpath_depth : newpath_depth - oldpath_depth;

	if (oldpath_depth == 0 || newpath_depth == 0) {
		printf("depth must be non zero\n");
		return -1;
	}

	rnode *input = NULL;
	rnode *parent = rmap_root;
	struct rbtree_node *res;
	for (int i=1; i <= newpath_depth; i++) {
		if (!input) {
			alloc_rnode(input);
		}
		if (newpath_depth != i) {
			memcpy(input->newpath, newpath_array[i], newpath_array[i+1] - newpath_array[i] - 1);
		} else {
			memcpy(input->newpath, newpath_array[i], newpath + strlen(newpath) - &newpath_array[i]);
		}

		res = rbtree_insert(input->rb_node, parent->tree);
		if (res) {	// found

		} else {	// not found

		}
	}
}

int renameMap_init() {
	alloc_node(rmap_root);
	rmap_root->newpath[0] = '/';
	for (int i=0; i < MAXLEVEL; i++) {
		rmap_root->oldpath[i][0] = '/';
	}
}

int renameMap_destory(rnode *root) {
	struct rbtree_node *first = rbtree_first(root->tree);
	while (first = rbtree_first(root->tree)) {
		
	}

}
