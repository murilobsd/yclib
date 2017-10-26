/* #include "rns_heap.h" */
/* #include "rns_memory.h" */

/* void heap_swap(rns_heap_node_t* f, rns_heap_node_t* s) */
/* { */
/*     uint32_t key = s->key; */
/*     s->key = f->key; */
/*     f->key = key; */
/* } */

/* rns_heap_t* rns_heap_create() */
/* { */
/*     rns_heap_t* heap = (rns_heap_t*)rns_malloc(sizeof(rns_heap_t)); */
/*     return heap; */
/* } */

/* int32_t rns_heap_insert(rns_heap_t* heap, rns_heap_node_t* node) */
/* { */
/*     uint32_t mask = (1 << (heap->height - 1)) - 1; */
/*     uint32_t num = (heap->total & mask) + 1; */
/*     if(num == (1 << (heap->height - 1))) */
/*     { */
/*         num = 0; */
/*         ++heap->height; */
/*     } */

/*     uint32_t h = heap->height - 2; */
/*     rns_heap_node_t* defender = heap->root; */
/*     rns_heap_node_t* attacker = node; */

/*     for(h = heap->height - 1; h > 0; --h) */
/*     { */
/*         if(attacker->key < defender->key) */
/*         { */
/*             attacker->left = defender->left; */
/*             attacker->right = defender->right; */
/*             attacker = defender; */
/*         } */
        
/*         if(num & (1 << (h - 1))) */
/*         { */
/*             if(defender->right == NULL) */
/*             { */
/*                 defender->right = attacker; */
/*                 break; */
/*             } */
/*             else */
/*             { */
/*                 defender = defender->right; */
/*             } */
/*         } */
/*         else */
/*         { */
/*             if(defender->left == NULL) */
/*             { */
/*                 defender->left = attacker; */
/*                 break; */
/*             } */
/*             else */
/*             { */
/*                 defender = defender->left; */
/*             } */

/*         } */
/*     } */
/*     ++heap->total; */
    
/*     return 0; */
/* } */

/* rns_heap_node_t* rns_heap_delete(rns_heap_t* heap) */
/* { */
/*     rns_heap_node_t* node = heap->root; */

/*     uint32_t mask = (1 << (heap->height - 1)) - 1; */
/*     uint32_t num = (heap->total & mask); */
/*     if(num == (1 << (heap->height - 1))) */
/*     { */
/*         num = 0; */
/*     } */

/*     rns_heap_node_t* walker = heap->root; */
/*     rns_heap_node_t* last_parent = NULL; */
/*     uint32_t h = 0; */
    
/*     for(h = heap->height - 1; h > 0; --h) */
/*     { */
/*         if(h == 2) */
/*         { */
/*             last_parent = walker; */
/*         } */
        
/*         if(num & (1 << (h - 1))) */
/*         { */
/*             walker = walker->right; */
/*             if(h == 1) */
/*             { */
/*                 last_parent->right = NULL; */
/*             } */
/*         } */
/*         else */
/*         { */
/*             walker = walker->left; */
/*             if(h == 1) */
/*             { */
/*                 last_parent->left = NULL; */
/*             } */
/*         } */
        
        
/*     } */

/*     walker->left = heap->root->left; */
/*     walker->right = heap->root->right; */
/*     heap->root = walker; */


/*     rns_heap_node_t* attacker = walker; */
/*     while(attacker != NULL) */
/*     { */
/*         if(attacker->left && attacker->key > attacker->left->key) */
/*         { */
/*             if(attacker->right && attacker->left->key > attacker->right->key) */
/*             { */
/*                 heap_swap(attacker, attacker->right); */
/*                 attacker = attacker->right; */
/*             } */
/*             else */
/*             { */
/*                 heap_swap(attacker, attacker->left); */
/*                 attacker = attacker->left; */
/*             } */
/*         } */
/*         else if(attacker->right && attacker->key > attacker->right->key) */
/*         { */
/*             if(attacker->left && attacker->right->key > attacker->left->key) */
/*             { */
/*                 heap_swap(attacker, attacker->left); */
/*                 attacker = attacker->left; */
/*             } */
/*             else */
/*             { */
/*                 heap_swap(attacker, attacker->right); */
/*                 attacker = attacker->right; */
/*             } */
/*         } */
/*         else */
/*         { */
/*             break; */
/*         } */
/*     } */
        
/*     return node; */
/* } */

