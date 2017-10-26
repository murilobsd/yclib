#include "rns_rbt.h"
#include "rns_memory.h"

static inline void rbt_set_black(rbt_node_t *rb)
{
    rb->__rbt_parent_color |= RBT_BLACK;
}

static inline rbt_node_t *rbt_red_parent(rbt_node_t *red)
{
    return (rbt_node_t *)red->__rbt_parent_color;
}

/*
 * Helper function for rotations:
 * - old's parent and color get assigned to new
 * - old gets assigned new as a parent and 'color' as a color.
 */
static inline void
__rbt_rotate_set_parents(rbt_node_t *old, rbt_node_t *cnew,
			rbt_root_t *root, int color)
{
    rbt_node_t *parent = rbt_parent(old);
    cnew->__rbt_parent_color = old->__rbt_parent_color;
    rbt_set_parent_color(old, cnew, color);
    __rbt_change_child(old, cnew, parent, root);
}

static __always_inline void
__rbt_insert(rbt_node_t *node, rbt_root_t *root,
	    void (*augment_rotate)(rbt_node_t *old, rbt_node_t *cnew))
{
    rbt_node_t *parent = rbt_red_parent(node), *gparent, *tmp;
    
    while (true) {
        /*
         * Loop invariant: node is red
         *
         * If there is a black parent, we are done.
         * Otherwise, take some corrective action as we don't
         * want a red root or two consecutive red nodes.
         */
        if (!parent) {
            rbt_set_parent_color(node, NULL, RBT_BLACK);
            break;
        } else if (rbt_is_black(parent))
            break;
        
        gparent = rbt_red_parent(parent);
        
        tmp = gparent->rbt_right;
        if (parent != tmp) {	/* parent == gparent->rbt_left */
            if (tmp && rbt_is_red(tmp)) {
                /*
                 * Case 1 - color flips
                 *
                 *       G            g
                 *      / \          /                          \
                 *     p   u  -->   P   U
                 *    /            /
                 *   n            n
                 *
                 * However, since g's parent might be red, and
                 * 4) does not allow this, we need to recurse
                 * at g.
                 */
                rbt_set_parent_color(tmp, gparent, RBT_BLACK);
                rbt_set_parent_color(parent, gparent, RBT_BLACK);
                node = gparent;
                parent = rbt_parent(node);
                rbt_set_parent_color(node, parent, RBT_RED);
                continue;
            }
            
            tmp = parent->rbt_right;
            if (node == tmp) {
                /*
                 * Case 2 - left rotate at parent
                 *
                 *      G             G
                 *     / \           /                          \
                 *    p   U  -->    n   U
                 *     \           /
                 *      n         p
                 *
                 * This still leaves us in violation of 4), the
                 * continuation into Case 3 will fix that.
                 */
                tmp = node->rbt_left;
                WRITE_ONCE(parent->rbt_right, tmp);
                WRITE_ONCE(node->rbt_left, parent);
                if (tmp)
                    rbt_set_parent_color(tmp, parent,
                                        RBT_BLACK);
                rbt_set_parent_color(parent, node, RBT_RED);
                augment_rotate(parent, node);
                parent = node;
                tmp = node->rbt_right;
            }
            
            /*
             * Case 3 - right rotate at gparent
             *
             *        G           P
             *       / \         /                      \
             *      p   U  -->  n   g
             *     /                                    \
             *    n                   U
             */
            WRITE_ONCE(gparent->rbt_left, tmp); /* == parent->rbt_right */
            WRITE_ONCE(parent->rbt_right, gparent);
            if (tmp)
                rbt_set_parent_color(tmp, gparent, RBT_BLACK);
            __rbt_rotate_set_parents(gparent, parent, root, RBT_RED);
            augment_rotate(gparent, parent);
            break;
        } else {
            tmp = gparent->rbt_left;
            if (tmp && rbt_is_red(tmp)) {
                /* Case 1 - color flips */
                rbt_set_parent_color(tmp, gparent, RBT_BLACK);
                rbt_set_parent_color(parent, gparent, RBT_BLACK);
                node = gparent;
                parent = rbt_parent(node);
                rbt_set_parent_color(node, parent, RBT_RED);
                continue;
            }
            
            tmp = parent->rbt_left;
            if (node == tmp) {
                /* Case 2 - right rotate at parent */
                tmp = node->rbt_right;
                WRITE_ONCE(parent->rbt_left, tmp);
                WRITE_ONCE(node->rbt_right, parent);
                if (tmp)
                    rbt_set_parent_color(tmp, parent,
                                        RBT_BLACK);
                rbt_set_parent_color(parent, node, RBT_RED);
                augment_rotate(parent, node);
                parent = node;
                tmp = node->rbt_left;
            }
            
            /* Case 3 - left rotate at gparent */
            WRITE_ONCE(gparent->rbt_right, tmp); /* == parent->rbt_left */
            WRITE_ONCE(parent->rbt_left, gparent);
            if (tmp)
                rbt_set_parent_color(tmp, gparent, RBT_BLACK);
            __rbt_rotate_set_parents(gparent, parent, root, RBT_RED);
            augment_rotate(gparent, parent);
            break;
        }
    }
}

/*
 * Inline version for rbt_erase() use - we want to be able to inline
 * and eliminate the dummy_rotate callback there
 */
static __always_inline void
____rbt_erase_color(rbt_node_t *parent, rbt_root_t *root,
                   void (*augment_rotate)(rbt_node_t *old, rbt_node_t *cnew))
{
    rbt_node_t *node = NULL, *sibling, *tmp1, *tmp2;
    
    while (true) {
        /*
         * Loop invariants:
         * - node is black (or NULL on first iteration)
         * - node is not the root (parent is not NULL)
         * - All leaf paths going through parent and node have a
         *   black node count that is 1 lower than other leaf paths.
         */
        sibling = parent->rbt_right;
        if (node != sibling) {	/* node == parent->rbt_left */
            if (rbt_is_red(sibling)) {
                /*
                 * Case 1 - left rotate at parent
                 *
                 *     P               S
                 *    / \             /                         \
                 *   N   s    -->    p   Sr
                 *      / \         /                           \
                 *     Sl  Sr      N   Sl
                 */
                tmp1 = sibling->rbt_left;
                WRITE_ONCE(parent->rbt_right, tmp1);
                WRITE_ONCE(sibling->rbt_left, parent);
                rbt_set_parent_color(tmp1, parent, RBT_BLACK);
                __rbt_rotate_set_parents(parent, sibling, root,
                                        RBT_RED);
                augment_rotate(parent, sibling);
                sibling = tmp1;
            }
            tmp1 = sibling->rbt_right;
            if (!tmp1 || rbt_is_black(tmp1)) {
                tmp2 = sibling->rbt_left;
                if (!tmp2 || rbt_is_black(tmp2)) {
                    /*
                     * Case 2 - sibling color flip
                     * (p could be either color here)
                     *
                     *    (p)           (p)
                     *    / \           /                               \
                     *   N   S    -->  N   s
                     *      / \           /                             \
                     *     Sl  Sr        Sl  Sr
                     *
                     * This leaves us violating 5) which
                     * can be fixed by flipping p to black
                     * if it was red, or by recursing at p.
                     * p is red when coming from Case 1.
                     */
                    rbt_set_parent_color(sibling, parent,
                                        RBT_RED);
                    if (rbt_is_red(parent))
                        rbt_set_black(parent);
                    else {
                        node = parent;
                        parent = rbt_parent(node);
                        if (parent)
                            continue;
                    }
                    break;
                }
                /*
                 * Case 3 - right rotate at sibling
                 * (p could be either color here)
                 *
                 *   (p)           (p)
                 *   / \           /                            \
                 *  N   S    -->  N   sl
                 *     / \                                      \
                 *    sl  Sr            S
                 *                                              \
                 *                        Sr
                 *
                 * Note: p might be red, and then both
                 * p and sl are red after rotation(which
                 * breaks property 4). This is fixed in
                 * Case 4 (in __rbt_rotate_set_parents()
                 *         which set sl the color of p
                 *         and set p RBT_BLACK)
                 *
                 *   (p)            (sl)
                 *   / \            /                           \
                 *  N   sl   -->   P    S
                 *       \        /                             \
                 *        S      N        Sr
                 *                              \
                 *          Sr
                 */
                tmp1 = tmp2->rbt_right;
                WRITE_ONCE(sibling->rbt_left, tmp1);
                WRITE_ONCE(tmp2->rbt_right, sibling);
                WRITE_ONCE(parent->rbt_right, tmp2);
                if (tmp1)
                    rbt_set_parent_color(tmp1, sibling,
                                        RBT_BLACK);
                augment_rotate(sibling, tmp2);
                tmp1 = sibling;
                sibling = tmp2;
            }
            /*
             * Case 4 - left rotate at parent + color flips
             * (p and sl could be either color here.
             *  After rotation, p becomes black, s acquires
             *  p's color, and sl keeps its color)
             *
             *      (p)             (s)
             *      / \             /                   \
             *     N   S     -->   P   Sr
             *        / \         /                     \
             *      (sl) sr      N  (sl)
             */
            tmp2 = sibling->rbt_left;
            WRITE_ONCE(parent->rbt_right, tmp2);
            WRITE_ONCE(sibling->rbt_left, parent);
            rbt_set_parent_color(tmp1, sibling, RBT_BLACK);
            if (tmp2)
                rbt_set_parent(tmp2, parent);
            __rbt_rotate_set_parents(parent, sibling, root,
                                    RBT_BLACK);
            augment_rotate(parent, sibling);
            break;
        } else {
            sibling = parent->rbt_left;
            if (rbt_is_red(sibling)) {
                /* Case 1 - right rotate at parent */
                tmp1 = sibling->rbt_right;
                WRITE_ONCE(parent->rbt_left, tmp1);
                WRITE_ONCE(sibling->rbt_right, parent);
                rbt_set_parent_color(tmp1, parent, RBT_BLACK);
                __rbt_rotate_set_parents(parent, sibling, root,
                                        RBT_RED);
                augment_rotate(parent, sibling);
                sibling = tmp1;
            }
            tmp1 = sibling->rbt_left;
            if (!tmp1 || rbt_is_black(tmp1)) {
                tmp2 = sibling->rbt_right;
                if (!tmp2 || rbt_is_black(tmp2)) {
                    /* Case 2 - sibling color flip */
                    rbt_set_parent_color(sibling, parent,
                                        RBT_RED);
                    if (rbt_is_red(parent))
                        rbt_set_black(parent);
                    else {
                        node = parent;
                        parent = rbt_parent(node);
                        if (parent)
                            continue;
                    }
                    break;
                }
                /* Case 3 - left rotate at sibling */
                tmp1 = tmp2->rbt_left;
                WRITE_ONCE(sibling->rbt_right, tmp1);
                WRITE_ONCE(tmp2->rbt_left, sibling);
                WRITE_ONCE(parent->rbt_left, tmp2);
                if (tmp1)
                    rbt_set_parent_color(tmp1, sibling,
                                        RBT_BLACK);
                augment_rotate(sibling, tmp2);
                tmp1 = sibling;
                sibling = tmp2;
            }
            /* Case 4 - right rotate at parent + color flips */
            tmp2 = sibling->rbt_right;
            WRITE_ONCE(parent->rbt_left, tmp2);
            WRITE_ONCE(sibling->rbt_right, parent);
            rbt_set_parent_color(tmp1, sibling, RBT_BLACK);
            if (tmp2)
                rbt_set_parent(tmp2, parent);
            __rbt_rotate_set_parents(parent, sibling, root,
                                    RBT_BLACK);
            augment_rotate(parent, sibling);
            break;
        }
    }
}

/* Non-inline version for rbt_erase_augmented() use */
void __rbt_erase_color(rbt_node_t *parent, rbt_root_t *root,
                      void (*augment_rotate)(rbt_node_t *old, rbt_node_t *cnew))
{
    ____rbt_erase_color(parent, root, augment_rotate);
}

/*
 * Non-augmented rbtree manipulation functions.
 *
 * We use dummy augmented callbacks here, and have the compiler optimize them
 * out of the rbt_insert_color() and rbt_erase() function definitions.
 */

static inline void dummy_propagate(rbt_node_t *node, rbt_node_t *stop) {}
static inline void dummy_copy(rbt_node_t *old, rbt_node_t *cnew) {}
static inline void dummy_rotate(rbt_node_t *old, rbt_node_t *cnew) {}

static const struct rbt_augment_callbacks dummy_callbacks = {
dummy_propagate, dummy_copy, dummy_rotate
};

void rbt_insert_color(rbt_node_t *node, rbt_root_t *root)
{
    __rbt_insert(node, root, dummy_rotate);
}

void rbt_erase(rbt_node_t *node, rbt_root_t *root)
{
    rbt_node_t *rebalance;
    rebalance = __rbt_erase_augmented(node, root, &dummy_callbacks);
    if (rebalance)
        ____rbt_erase_color(rebalance, root, dummy_rotate);
}

/*
 * Augmented rbtree manipulation functions.
 *
 * This instantiates the same __always_inline functions as in the non-augmented
 * case, but this time with user-defined callbacks.
 */

void __rbt_insert_augmented(rbt_node_t *node, rbt_root_t *root,
                           void (*augment_rotate)(rbt_node_t *old, rbt_node_t *cnew))
{
    __rbt_insert(node, root, augment_rotate);
}

/*
 * This function returns the first node (in sort order) of the tree.
 */
rbt_node_t *rbt_first(const rbt_root_t *root)
{
    rbt_node_t	*n;
    
    n = root->rbt_node;
    if (!n)
        return NULL;
    while (n->rbt_left)
        n = n->rbt_left;
    return n;
}

rbt_node_t *rbt_last(const rbt_root_t *root)
{
    rbt_node_t	*n;
    
    n = root->rbt_node;
    if (!n)
        return NULL;
    while (n->rbt_right)
        n = n->rbt_right;
    return n;
}

rbt_node_t *rbt_next(const rbt_node_t *node)
{
    rbt_node_t *parent;
    
    if (RBT_EMPTY_NODE(node))
        return NULL;
    
    /*
     * If we have a right-hand child, go down and then left as far
     * as we can.
     */
    if (node->rbt_right) {
        node = node->rbt_right; 
        while (node->rbt_left)
            node=node->rbt_left;
        return (rbt_node_t *)node;
    }
    
    /*
     * No right-hand children. Everything down and left is smaller than us,
     * so any 'next' node must be in the general direction of our parent.
     * Go up the tree; any time the ancestor is a right-hand child of its
     * parent, keep going up. First time it's a left-hand child of its
     * parent, said parent is our 'next' node.
     */
    while ((parent = rbt_parent(node)) && node == parent->rbt_right)
        node = parent;
    
    return parent;
}

rbt_node_t *rbt_prev(const rbt_node_t *node)
{
    rbt_node_t *parent;
    
    if (RBT_EMPTY_NODE(node))
        return NULL;
    
    /*
     * If we have a left-hand child, go down and then right as far
     * as we can.
     */
    if (node->rbt_left) {
        node = node->rbt_left; 
        while (node->rbt_right)
            node=node->rbt_right;
        return (rbt_node_t *)node;
    }
    
    /*
     * No left-hand children. Go up till we find an ancestor which
     * is a right-hand child of its parent.
     */
    while ((parent = rbt_parent(node)) && node == parent->rbt_left)
        node = parent;
    
    return parent;
}

void rbt_replace_node(rbt_node_t *victim, rbt_node_t *cnew,
		     rbt_root_t *root)
{
    rbt_node_t *parent = rbt_parent(victim);
    
    /* Copy the pointers/colour from the victim to the replacement */
    *cnew = *victim;
    
    /* Set the surrounding nodes to point to the replacement */
    if (victim->rbt_left)
        rbt_set_parent(victim->rbt_left, cnew);
    if (victim->rbt_right)
        rbt_set_parent(victim->rbt_right, cnew);
    __rbt_change_child(victim, cnew, parent, root);
}

static rbt_node_t *rbt_left_deepest_node(const rbt_node_t *node)
{
    for (;;) {
        if (node->rbt_left)
            node = node->rbt_left;
        else if (node->rbt_right)
            node = node->rbt_right;
        else
            return (rbt_node_t *)node;
    }
}

rbt_node_t *rbt_next_postorder(const rbt_node_t *node)
{
    const rbt_node_t *parent;
    if (!node)
        return NULL;
    parent = rbt_parent(node);
    
    /* If we're sitting on node, we've already seen our children */
    if (parent && node == parent->rbt_left && parent->rbt_right) {
        /* If we are the parent's left node, go to the parent's right
         * node then all the way down to the left */
        return rbt_left_deepest_node(parent->rbt_right);
    } else
        /* Otherwise we are the parent's right node, and the parent
         * should be next */
        return (rbt_node_t *)parent;
}

rbt_node_t *rbt_first_postorder(const rbt_root_t *root)
{
    if (!root->rbt_node)
        return NULL;
    
    return rbt_left_deepest_node(root->rbt_node);
}



void rbt_init(rbt_root_t* root, uint32_t type)
{
    root->type = type;
    root->rbt_node = NULL;
}

void rbt_set_key_int(rbt_node_t* node, uint64_t key)
{
    node->key.idx = key;
}

void rbt_set_key_str(rbt_node_t* node, uchar_t* key)
{
    uint32_t len = rns_strlen(key);
    node->key.str = (uchar_t*)rns_malloc(len + 1);
    if(node->key.str == NULL)
    {
        return;
    }
    rns_strncpy(node->key.str, key, len);
}

void rbt_set_key_nstr(rbt_node_t* node, uchar_t* key, uint32_t size)
{
    node->key.str = (uchar_t*)rns_malloc(size + 1);
    if(node->key.str == NULL)
    {
        return;
    }
    rns_strncpy(node->key.str, key, size);
}

void rbt_insert_int(rbt_root_t* root, rbt_node_t* node)
{
    rbt_node_t **walker = &(root->rbt_node); 
    rbt_node_t *parent = NULL; 
    while (*walker)
    {              
        parent = *walker; 
        if (node->key.idx < (*walker)->key.idx) 
        {
            walker = &((*walker)->rbt_left); 
        }             
        else
        {
            walker = &((*walker)->rbt_right); 
        }             
    }          
    
    rbt_link_node(node, parent, walker);
    rbt_insert_color(node, root);
    return; 
}

void rbt_insert_str(rbt_root_t* root, rbt_node_t* node)                     
{
    rbt_node_t **walker = &(root->rbt_node); 
    rbt_node_t *parent = NULL;
    
    while (*walker)
    {              
        parent = *walker; 
        if (rns_strcmp(node->key.str, (*walker)->key.str) < 0) 
        {
            walker = &((*walker)->rbt_left); 
        }             
        else
        {
            walker = &((*walker)->rbt_right); 
        }             
    }          
    
    rbt_link_node(node, parent, walker);
    rbt_insert_color(node, root);
    return;
}

void rbt_insert(rbt_root_t* root, rbt_node_t* node)
{
    if(root->type == RBT_TYPE_INT)
    {
        rbt_insert_int(root, node);
    }
    else if(root->type == RBT_TYPE_STR)
    {
        rbt_insert_str(root, node);
    }
    return;
}

rbt_node_t* rbt_search_int(rbt_root_t* root, uint64_t key)     
{
    if(root == NULL)
    {
        return NULL;
    }
    
    rbt_node_t *walker = root->rbt_node;            
    while (walker)                                    
    {                                                  
        if (key < walker->key.idx)                              
        {                                                 
            walker = walker->rbt_left;                            
        }                                                
        else if (key > walker->key.idx)                         
        {                                                 
            walker = walker->rbt_right;                           
        }                                                
        else                                              
        {
            while(walker->rbt_left  && key == walker->rbt_left->key.idx)
            {
                walker = walker->rbt_left;
            }
            return walker;
        }                                                
    }
    return NULL;   
}

rbt_node_t* rbt_search_str(rbt_root_t* root, uchar_t* key)
{
    if(root == NULL)
    {
        return NULL;
    }
    
    rbt_node_t *walker = root->rbt_node;
    while (walker)                                    
    {                                                  
        if (rns_strcmp(key, walker->key.str) < 0)  
        {                                                 
            walker = walker->rbt_left;                            
        }                                                
        else if (rns_strcmp(key, walker->key.str) > 0)                         
        {                                                 
            walker = walker->rbt_right;                           
        }                                                
        else                                              
        {
            while(walker->rbt_left && !rns_strcmp(key, walker->rbt_left->key.str))
            {
                walker = walker->rbt_left;
            }
            return walker;
        }                                                
    }
    return NULL; 
}

rbt_node_t* rbt_lsearch_int(rbt_root_t* root, uint64_t key)     
{
    if(root == NULL)
    {
        return NULL;
    }
    
    rbt_node_t *walker = root->rbt_node;
    rbt_node_t* parent = NULL;
    while (walker)                                    
    {
        if (key < walker->key.idx)                              
        {                                                 
            walker = walker->rbt_left;                            
        }                                                
        else if (key > walker->key.idx)                         
        {
            parent = walker;
            walker = walker->rbt_right;                           
        }                                                
        else                                              
        {
            while(walker->rbt_left && key == walker->rbt_left->key.idx)
            {
                walker = walker->rbt_left;
            }
            return walker;
        }                                                
    }
    return parent;   
}

rbt_node_t* rbt_lsearch_str(rbt_root_t* root, uchar_t* key)
{
    if(root == NULL)
    {
        return NULL;
    }
    
    rbt_node_t *walker = root->rbt_node;
    rbt_node_t* parent = NULL;
    while (walker)                                    
    {
        if (rns_strcmp(key, walker->key.str) < 0)  
        {                                                 
            walker = walker->rbt_left;                            
        }                                                
        else if (rns_strcmp(key, walker->key.str) > 0)                         
        {
            parent = walker;
            walker = walker->rbt_right;                           
        }                                                
        else                                              
        {
            while(walker->rbt_left && !rns_strcmp(key, walker->rbt_left->key.str))
            {
                walker = walker->rbt_left;
            }
            return walker;
        }                                                
    }
    return parent; 
}


rbt_node_t* rbt_gsearch_int(rbt_root_t* root, uint64_t key)     
{
    if(root == NULL)
    {
        return NULL;
    }
    
    rbt_node_t *walker = root->rbt_node;
    rbt_node_t* parent = NULL;
    while (walker)                                    
    {
        if (key < walker->key.idx)                              
        {
            parent = walker;
            walker = walker->rbt_left;                            
        }                                                
        else if (key > walker->key.idx)                         
        {                                                 
            walker = walker->rbt_right;                           
        }                                                
        else                                              
        {
            while(walker->rbt_left && key == walker->rbt_left->key.idx)
            {
                walker = walker->rbt_left;
            }
            return walker;
        }                                                
    }
    return parent;   
}

rbt_node_t* rbt_gsearch_str(rbt_root_t* root, uchar_t* key)
{
    if(root == NULL)
    {
        return NULL;
    }
    
    rbt_node_t *walker = root->rbt_node;
    rbt_node_t* parent = NULL;
    while (walker)                                    
    {
        if (rns_strcmp(key, walker->key.str) < 0)  
        {
            parent = walker;
            walker = walker->rbt_left;                            
        }                                                
        else if (rns_strcmp(key, walker->key.str) > 0)                         
        {                                                 
            walker = walker->rbt_right;                           
        }                                                
        else                                              
        {
            while(walker->rbt_left && !rns_strcmp(key, walker->rbt_left->key.str))
            {
                walker = walker->rbt_left;
            }
            return walker;
        }                                                
    }
    return parent; 
}

rbt_node_t* rbt_delete(rbt_root_t* root, rbt_node_t* node)                    
{
    if(root->type == RBT_TYPE_STR)
    {
        rns_free(node->key.str);
    }
    
    rbt_node_t* next = rbt_next(node);
    rbt_erase(node, root);
    return next;                                                   
}                                                                   


rns_ctx_t* rns_ctx_create()
{
    rns_ctx_t* ctx = (rns_ctx_t*)rns_malloc(sizeof(rns_ctx_t));
    if(ctx == NULL)
    {
        return NULL;
    }

    rbt_init(&ctx->root, RBT_TYPE_STR);
    return ctx;
}
int32_t rns_ctx_add(rns_ctx_t* ctx, char_t* key, void* value)
{
    rbt_node_t* n = rbt_search_str(&ctx->root, (uchar_t*)key);
    if(n != NULL)
    {
        rns_node_t* dn = rbt_entry(n, rns_node_t, node);
        rns_free(dn->data);
        dn->data = value;
    }
    else
    {
        rns_node_t* node = (rns_node_t*)rns_malloc(sizeof(rns_node_t));
        if(node == NULL)
        {
            return -1;
        }
        rbt_set_key_str(&node->node, (uchar_t*)key);
        node->data = value;
        rbt_insert(&ctx->root, &node->node);
    }
    return 0;
}

void* rns_ctx_get(rns_ctx_t* ctx, char_t* key)
{
    rbt_node_t* n = rbt_search_str(&ctx->root, (uchar_t*)key);
    if(n != NULL)
    {
        rns_node_t* dn = rbt_entry(n, rns_node_t, node);
        return dn->data;
    }
    return NULL;
}

void rns_ctx_destroy(rns_ctx_t** ctx)
{
    if(*ctx == NULL)
    {
        return;
    }

    rbt_node_t* node = NULL;
    while((node = rbt_first(&(*ctx)->root)) != NULL)
    {
        rns_node_t* item = rbt_entry(node, rns_node_t, node);
        rbt_delete(&(*ctx)->root, &item->node);
        rns_free(item);
    }

    rns_free(*ctx);
    *ctx = NULL;
    
    return;
}
