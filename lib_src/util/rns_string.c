/* #include "rns_string.h" */

/* rns_str_t* rns_str_create(uint32_t size) */
/* { */
/*     rns_str_t* str = (rns_str_t*)rns_malloc(sizeof(rns_str_t)); */
/*     if(str == NULL) */
/*     { */
/*         return NULL; */
/*     } */

/*     str->str = rns_malloc(size); */
/*     if(str->str == NULL) */
/*     { */
/*         rns_free(str); */
/*         return NULL; */
/*     } */

/*     str->len = size; */
    
/*     return str; */
/* } */

/* void rns_str_destroy(rns_str_t** str) */
/* { */
/*     if(*str == NULL) */
/*     { */
/*         return; */
/*     } */
/*     if((*str)->str != NULL) */
/*     { */
/*         rns_free((*str)->str); */
/*         (*str)->str = NULL; */
/*     } */

/*     rns_free(*str); */
/*     *str = NULL; */
    
/*     return; */
/* } */

