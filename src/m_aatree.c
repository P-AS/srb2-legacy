// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2018 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_aatree.h
/// \brief AA trees code

#include "m_aatree.h"
#include "z_zone.h"

// A partial implementation of AA trees,
// according to the algorithms given on Wikipedia.
// http://en.wikipedia.org/wiki/AA_tree

typedef struct aatree_node_s
{
	INT32 key;
	INT32 depth;
	void *value;

	struct aatree_node_s *left, *right;
} aatree_node_t;

struct aatree_s
{
	aatree_node_t *root;
	UINT32 flags;
};

aatree_t *M_AATreeAlloc(UINT32 flags)
{
	aatree_t *aatree = Z_Malloc(sizeof (aatree_t), PU_STATIC, NULL);
	aatree->root = NULL;
	aatree->flags = flags;
	return aatree;
}

static void M_AATreeFree_Node(aatree_node_t *node)
{
	if (node->left) M_AATreeFree_Node(node->left);
	if (node->right) M_AATreeFree_Node(node->right);
	Z_Free(node);
}

void M_AATreeFree(aatree_t *aatree)
{
	if (aatree->root)
		M_AATreeFree_Node(aatree->root);

	Z_Free(aatree);
}

static aatree_node_t *M_AATreeRotateRight(aatree_node_t *node)
{
	aatree_node_t *newnode = node->left;
	aatree_node_t *tmp = newnode->right;
	newnode->right = node;
	node->left = tmp;
	return newnode;
}

static aatree_node_t *M_AATreeRotateLeft(aatree_node_t *node)
{
	aatree_node_t *newnode = node->right;
	aatree_node_t *tmp = newnode->left;
	newnode->left = node;
	node->right = tmp;
	return newnode;
}

static aatree_node_t *M_AATreeRebalance(aatree_node_t *node)
{
	INT32 balance;
	if (node->left == NULL && node->right == NULL)
		return node;
	if (node->left == NULL)
		balance = -node->right->depth;
	else if (node->right == NULL)
		balance = node->left->depth;
	else
	{
		assert(node->left->key < node->right->key);
		balance = node->left->depth - node->right->depth;
	}

	if (balance > 1)
	{
		return M_AATreeRotateRight(node);
	}
	else if (balance < -1)
	{
		return M_AATreeRotateLeft(node);
	}

	return node;
}

static aatree_node_t *M_AATreeSet_Node(aatree_node_t *node, UINT32 flags, INT32 key, void* value)
{
	if (!node)
	{
		// Nothing here, so just add where we are
		node = Z_Malloc(sizeof (aatree_node_t), PU_STATIC, NULL);
		node->key = key;
		if (value && (flags & AATREE_ZUSER)) Z_SetUser(value, &node->value);
		else node->value = value;
		node->left = node->right = NULL;
		node->depth = 1;
	}
	else
	{
		if (key < node->key)
			node->left = M_AATreeSet_Node(node->left, flags, key, value);
		else if (key > node->key)
			node->right = M_AATreeSet_Node(node->right, flags, key, value);
		else
		{
			if (value && (flags & AATREE_ZUSER)) Z_SetUser(value, &node->value);
			else node->value = value;
		}

		node = M_AATreeRebalance(node);
		if (node->left == NULL && node->right == NULL)
			node->depth = 1;
		else if (node->left == NULL)
			node->depth = node->right->depth + 1;
		else if (node->right == NULL)
			node->depth = node->left->depth + 1;
		else if (node->left->depth > node->right->depth)
			node->depth = node->left->depth + 1;
		else
			node->depth = node->right->depth + 1;
	}

	return node;
}

void M_AATreeSet(aatree_t *aatree, INT32 key, void* value)
{
	aatree->root = M_AATreeSet_Node(aatree->root, aatree->flags, key, value);
}

// Caveat: we don't distinguish between nodes that don't exists
// and nodes with value == NULL.
static void *M_AATreeGet_Node(aatree_node_t *node, INT32 key)
{
	if (node)
	{
		if (node->key == key)
			return node->value;
		else if(node->key < key)
			return M_AATreeGet_Node(node->right, key);
		else
			return M_AATreeGet_Node(node->left, key);
	}

	return NULL;
}

void *M_AATreeGet(aatree_t *aatree, INT32 key)
{
	return M_AATreeGet_Node(aatree->root, key);
}

static void M_AATreeIterate_Node(aatree_node_t *node, aatree_iter_t callback)
{
	if (node->left) M_AATreeIterate_Node(node->left, callback);
	callback(node->key, node->value);
	if (node->right) M_AATreeIterate_Node(node->right, callback);
}

void M_AATreeIterate(aatree_t *aatree, aatree_iter_t callback)
{
	if (aatree->root)
		M_AATreeIterate_Node(aatree->root, callback);
}
