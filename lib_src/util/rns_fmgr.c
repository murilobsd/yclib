#include "rns_fmgr.h"
#include "rns_memory.h"
#include "rns_network.h"

rns_fmgr_t* rns_fmgr_load(uchar_t* mgr_file)
{
    rns_fmgr_t* fmgr = (rns_fmgr_t*)rns_malloc(sizeof(rns_fmgr_t));
    if(fmgr == NULL)
    {
        return NULL;
    }

    
    rbt_init(&fmgr->root, RBT_TYPE_INT);

    fmgr->fp = rns_file_open(mgr_file, RNS_FILE_OPEN | RNS_FILE_RW);
    if(fmgr->fp == NULL)
    {
        rns_free(fmgr);
        return NULL;
    }

    int32_t size = rns_file_size(fmgr->fp);
    if(size < 0)
    {
        rns_file_close(&fmgr->fp);
        rns_free(fmgr);
        
        return NULL;
    }

    uchar_t* buf = (uchar_t*)rns_malloc(size);
    if(buf == NULL)
    {
        rns_file_close(&fmgr->fp);
        rns_free(fmgr);

        return NULL;
    }

    int32_t ret = rns_file_read(fmgr->fp, buf, size);
    if(ret != size)
    {
        rns_free(buf);
        rns_file_close(&fmgr->fp);
        rns_free(fmgr);
        return NULL;
    }

    uchar_t* walker = buf;
    uchar_t* end = buf + size;
    
    while(walker < end)
    {
        rns_finfo_t* finfo = (rns_finfo_t*)rns_malloc(sizeof(rns_finfo_t));
        if(finfo == NULL)
        {
            continue;
        }

        rbt_set_key_int(&finfo->node, rns_btoh64(walker));
        walker += 8;
        uint32_t len = rns_btoh32(walker);
        walker += 4;
        finfo->file = rns_malloc(len + 1);
        if(finfo->file == NULL)
        {
            rns_free(finfo);
        }
        else
        {
            rns_strncpy(finfo->file, walker, len);
            rbt_insert(&fmgr->root, &finfo->node);
        }
        walker += len;
    }

    rns_free(buf);
    return fmgr;
}

void rns_fmgr_close(rns_fmgr_t** fmgr)
{
    
}

rns_finfo_t* rns_fmgr_add(rns_fmgr_t* fmgr, uchar_t* file, uint64_t key)
{
    if(fmgr == NULL || file == NULL)
    {
        return NULL;
    }
    
    rns_finfo_t* finfo = (rns_finfo_t*)rns_malloc(sizeof(rns_finfo_t));
    if(finfo == NULL)
    {
        return NULL;
    }
    rbt_set_key_int(&finfo->node, key);
    
    finfo->fp = rns_file_open(file, RNS_FILE_CREAT | RNS_FILE_RW);
    if(finfo->fp == NULL)
    {
        rns_free(finfo);
        return NULL;
    }

    finfo->file = rns_malloc(rns_strlen(file) + 1);
    if(finfo->file == NULL)
    {
        rns_file_close(&finfo->fp);
        rns_free(finfo);
        return NULL;
    }
    rns_strncpy(finfo->file, file, rns_strlen(file));

    uchar_t buf[8];
    rns_htob64(key, buf);
    int32_t ret = rns_file_write(fmgr->fp, buf, sizeof(uint64_t));
    if(ret != sizeof(uint64_t))
    {
        rns_free(finfo);
        return NULL;
    }

    uint32_t len = rns_strlen(file);
    rns_htob32(len, buf);
    ret = rns_file_write(fmgr->fp, buf, 4);
    if(ret != sizeof(uint32_t))
    {
        rns_free(finfo);
        return NULL;
    }
    
    ret = rns_file_write(fmgr->fp, file, len);
    if(ret != len)
    {
        rns_free(finfo);
        return NULL;
    }

    rbt_insert(&fmgr->root, &finfo->node);

    return finfo;
    
}

int32_t rns_fmgr_del(rns_fmgr_t* fmgr, uint64_t key)
{
    rbt_node_t* node = rbt_search_int(&fmgr->root, key);
    if(node)
    {
        rbt_delete(&fmgr->root, node);
    }

    return 0;
}

rns_finfo_t* rns_fmgr_search(rns_fmgr_t* fmgr, uint64_t key)
{
    rbt_node_t* node = rbt_lsearch_int(&fmgr->root, key);
    if(node == NULL)
    {
        return NULL;
    }

    rns_finfo_t* finfo = rbt_entry(node, rns_finfo_t, node);
    finfo->fp = rns_file_open(finfo->file, RNS_FILE_OPEN | RNS_FILE_RW);
    if(finfo->fp == NULL)
    {
        return NULL;
    }
    return finfo;
}

rns_finfo_t* rns_fmgr_next(rns_finfo_t* finfo)
{
    rbt_node_t* node = rbt_next(&finfo->node);
    if(node)
    {
        rns_finfo_t* f = rbt_entry(node, rns_finfo_t, node);
        return f;
    }
    
    return NULL;
}

uint64_t rns_fmgr_key(rns_finfo_t* finfo)
{
    return finfo->node.key.idx;
}
