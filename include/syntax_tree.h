#ifndef __SYNTAXTREE_H__
#define __SYNTAXTREE_H__

#include <stdio.h>

#define SYNTAX_TREE_NODE_NAME_MAX 30

// the node in syntax tree
struct _syntax_tree_node {
	struct _syntax_tree_node * parent;
	struct _syntax_tree_node * children[10];
	int children_num;

	char name[SYNTAX_TREE_NODE_NAME_MAX];
};
typedef struct _syntax_tree_node syntax_tree_node;

// allocate and return an anonymous syntax tree node
syntax_tree_node * new_anon_syntax_tree_node();

// allocate and return a syntax tree node whose name is assigned
// @param 	name		the name of the syntax tree node
// @return				a syntax tree node whose name is assigned
syntax_tree_node * new_syntax_tree_node(const char * name);

// append a child node into a parent node
// @param 	parent		the parent node where a child node is inserted
// @param	child		the child node to be inserted into the parent node
// @return 				the number of children nodes in parent.
//						If parent or child is null, return -1
int syntax_tree_add_child(syntax_tree_node * parent, syntax_tree_node * child);

// delete a tree node and its children node recursively
// @param	node		the node who and whose children nodes will be deleted
// @param	recursive	flag to denote whether to just delete the current node(0)
//						or to delete all children nodes recursively
void del_syntax_tree_node(syntax_tree_node * node, int recursive);

// A syntax tree
struct _syntax_tree {
	syntax_tree_node * root;
};
typedef struct _syntax_tree syntax_tree;

// allocate and return a new syntax tree
syntax_tree* new_syntax_tree();

// delete a syntax tree
// @param	tree		the syntax tree to be deleted
void del_syntax_tree(syntax_tree * tree);

// print the syntax tree into a file
// @param 	fout		the file to store output
// @param	tree		the syntax tree to be printed
void print_syntax_tree(FILE * fout, syntax_tree * tree);

#endif /* SyntaxTree.h */
