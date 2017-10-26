/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : rns_rbt.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-02-18 17:58:46
 * -------------------------
**************************************************************************/

#ifndef _RNS_RBT_H_
#define _RNS_RBT_H_

#include "comm.h"

typedef union rbt_node_key_s
{
    uchar_t *str;
    uint64_t idx;  
} rbt_node_key_t;

#define RBT_TYPE_INT 0
#define RBT_TYPE_STR 1

#define RBT_RED      0  
#define RBT_BLACK    1

#define WRITE_ONCE(x, val) x=(val)

typedef struct rbt_node_s
{
    unsigned long  __rbt_parent_color;
    struct rbt_node_s *rbt_right;
    struct rbt_node_s *rbt_left;
    rbt_node_key_t key; 
} __attribute__((aligned(sizeof(long))))rbt_node_t;
/* The alignment might seem pointless, but allegedly CRIS needs it */

typedef struct rbt_root_s
{
    rbt_node_t *rbt_node;
    uint32_t type;
}rbt_root_t;

struct rbt_augment_callbacks {
    void (*propagate)(rbt_node_t *node, rbt_node_t *stop);
    void (*copy)(rbt_node_t *old, rbt_node_t *cnew);
    void (*rotate)(rbt_node_t *old, rbt_node_t *cnew);
};

#define __rbt_parent(pc)    ((rbt_node_t *)(pc & ~3))


#define __rbt_color(pc)     ((pc) & 1)
#define __rbt_is_black(pc)  __rbt_color(pc)
#define __rbt_is_red(pc)    (!__rbt_color(pc))
#define rbt_color(rb)       __rbt_color((rb)->__rbt_parent_color)
#define rbt_is_red(rb)      __rbt_is_red((rb)->__rbt_parent_color)
#define rbt_is_black(rb) __rbt_is_black((rb)->__rbt_parent_color)

static inline void rbt_set_parent(rbt_node_t *rbt, rbt_node_t *p)
{
    rbt->__rbt_parent_color = rbt_color(rbt) | (unsigned long)p;
}

static inline void rbt_set_parent_color(rbt_node_t *rbt,
				       rbt_node_t *p, int color)
{
    rbt->__rbt_parent_color = (unsigned long)p | color;
}

static inline void __rbt_change_child(rbt_node_t *old, rbt_node_t *cnew,
		  rbt_node_t *parent, rbt_root_t *root)
{
    if (parent) {
        if (parent->rbt_left == old)
            WRITE_ONCE(parent->rbt_left, cnew);
        else
            WRITE_ONCE(parent->rbt_right, cnew);
    } else
        WRITE_ONCE(root->rbt_node, cnew);
}


#define rbt_parent(r)   ((rbt_node_t *)((r)->__rbt_parent_color & ~3))

#define	rbt_entry(ptr, type, member) container_of(ptr, type, member)

#define RBT_EMPTY_ROOT(root)  (READ_ONCE((root)->rbt_node) == NULL)

/* 'empty' nodes are nodes that are known not to be inserted in an rbtree */
#define RBT_EMPTY_NODE(node)                                     \
    ((node)->__rbt_parent_color == (unsigned long)(node))
#define RBT_CLEAR_NODE(node)                                     \
    ((node)->__rbt_parent_color = (unsigned long)(node))


void rbt_insert_color(rbt_node_t *, rbt_root_t *);
void rbt_erase(rbt_node_t *, rbt_root_t *);

/* Find logical next and previous nodes in a tree */
rbt_node_t *rbt_next(const rbt_node_t *);
rbt_node_t *rbt_prev(const rbt_node_t *);
rbt_node_t *rbt_first(const rbt_root_t *);
rbt_node_t *rbt_last(const rbt_root_t *);

#define rbt_for_each(root, node) \
    for((node) = (rbt_first(root)); (node) != NULL; (node) = (rbt_next(node)))

#define rbt_for_each_r(root, node)                                        \
    for((node) = (rbt_last(root)); (node) != NULL; (node) = (rbt_prev(node)))

/* Postorder iteration - always visit the parent after its children */
rbt_node_t *rbt_first_postorder(const rbt_root_t *);
rbt_node_t *rbt_next_postorder(const rbt_node_t *);

void rbt_replace_node(rbt_node_t *victim, rbt_node_t *cnew, rbt_root_t *root);

static inline void rbt_link_node(rbt_node_t *node, rbt_node_t *parent,
				rbt_node_t **rbt_link)
{
    node->__rbt_parent_color = (unsigned long)parent;
    node->rbt_left = node->rbt_right = NULL;
    
    *rbt_link = node;
}

#define rbt_entry_safe(ptr, type, member)        \
    ({ typeof(ptr) ____ptr = (ptr);                          \
        ____ptr ? rbt_entry(____ptr, type, member) : NULL;    \
    })


static __always_inline rbt_node_t *
__rbt_erase_augmented(rbt_node_t *node, rbt_root_t *root,
		     const struct rbt_augment_callbacks *augment)
{
    rbt_node_t *child = node->rbt_right;
    rbt_node_t *tmp = node->rbt_left;
    rbt_node_t *parent, *rebalance;
    unsigned long pc;
    
    if (!tmp) {
        /*
         * Case 1: node to erase has no more than 1 child (easy!)
         *
         * Note that if there is one child it must be red due to 5)
         * and node must be black due to 4). We adjust colors locally
         * so as to bypass __rbt_erase_color() later on.
         */
        pc = node->__rbt_parent_color;
        parent = __rbt_parent(pc);
        __rbt_change_child(node, child, parent, root);
        if (child) {
            child->__rbt_parent_color = pc;
            rebalance = NULL;
        } else
            rebalance = __rbt_is_black(pc) ? parent : NULL;
        tmp = parent;
    } else if (!child) {
        /* Still case 1, but this time the child is node->rbt_left */
        tmp->__rbt_parent_color = pc = node->__rbt_parent_color;
        parent = __rbt_parent(pc);
        __rbt_change_child(node, tmp, parent, root);
        rebalance = NULL;
        tmp = parent;
    } else {
        rbt_node_t *successor = child, *child2;
        
        tmp = child->rbt_left;
        if (!tmp) {
            /*
             * Case 2: node's successor is its right child
             *
             *    (n)          (s)
             *    / \          /                        \
             *  (x) (s)  ->  (x) (c)
             *                                  \
             *        (c)
             */
            parent = successor;
            child2 = successor->rbt_right;
            
            augment->copy(node, successor);
        } else {
            /*
             * Case 3: node's successor is leftmost under
             * node's right child subtree
             *
             *    (n)          (s)
             *    / \          /                        \
             *  (x) (y)  ->  (x) (y)
             *      /            /
             *    (p)          (p)
             *    /            /
             *  (s)          (c)
             *                                  \
             *    (c)
             */
            do {
                parent = successor;
                successor = tmp;
                tmp = tmp->rbt_left;
            } while (tmp);
            child2 = successor->rbt_right;
            WRITE_ONCE(parent->rbt_left, child2);
            WRITE_ONCE(successor->rbt_right, child);
            rbt_set_parent(child, successor);
            
            augment->copy(node, successor);
            augment->propagate(parent, successor);
        }
        
        tmp = node->rbt_left;
        WRITE_ONCE(successor->rbt_left, tmp);
        rbt_set_parent(tmp, successor);
        
        pc = node->__rbt_parent_color;
        tmp = __rbt_parent(pc);
        __rbt_change_child(node, successor, tmp, root);
        
        if (child2) {
            successor->__rbt_parent_color = pc;
            rbt_set_parent_color(child2, parent, RBT_BLACK);
            rebalance = NULL;
        } else {
            unsigned long pc2 = successor->__rbt_parent_color;
            successor->__rbt_parent_color = pc;
            rebalance = __rbt_is_black(pc2) ? parent : NULL;
        }
        tmp = successor;
    }
    
    augment->propagate(tmp, NULL);
    return rebalance;
}

typedef struct rns_node_s
{
    rbt_node_t node;
    uchar_t* data;
    uint32_t size;
}rns_node_t;

typedef struct rns_ctx_s
{
    rbt_root_t root;
}rns_ctx_t;

rns_ctx_t* rns_ctx_create();
int32_t rns_ctx_add(rns_ctx_t* ctx, char_t* key, void* value);
void* rns_ctx_get(rns_ctx_t* ctx, char_t* key);
void rns_ctx_destroy(rns_ctx_t** ctx);

void rbt_init(rbt_root_t* root, uint32_t type);

void rbt_insert(rbt_root_t* root, rbt_node_t* node);                   

rbt_node_t* rbt_search_int(rbt_root_t* root, uint64_t key);
rbt_node_t* rbt_search_str(rbt_root_t* root, uchar_t* key);

rbt_node_t* rbt_nsearch_int(rbt_root_t* root, uint64_t key);
rbt_node_t* rbt_nsearch_str(rbt_root_t* root, uchar_t* key);
rbt_node_t* rbt_lsearch_int(rbt_root_t* root, uint64_t key);
rbt_node_t* rbt_lsearch_str(rbt_root_t* root, uchar_t* key);
rbt_node_t* rbt_gsearch_int(rbt_root_t* root, uint64_t key);
rbt_node_t* rbt_gsearch_str(rbt_root_t* root, uchar_t* key);

rbt_node_t* rbt_delete(rbt_root_t* root, rbt_node_t* node);

void rbt_set_key_int(rbt_node_t* node, uint64_t key);

void rbt_set_key_str(rbt_node_t* node, uchar_t* key);
void rbt_set_key_nstr(rbt_node_t* node, uchar_t* key, uint32_t size);
#endif	/* _LINUX_RBTREE_H */

