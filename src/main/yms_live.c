#include <sys/ipc.h>
#include <ctype.h>
#include "yms_live.h"
#include <stdio.h>
#include <sys/timerfd.h>
#include <stdlib.h>
#include <errno.h>

static void yms_channel_req_ts(void* data);
void yms_channel_req_m3u8(void* data);

static void yms_channel_recd(channel_t* channel, block_t* block, uint64_t offset)
{
    LOG_INFO(lp, "recd a block, channel id : %s, offset : %llu", channel->content_id, offset);
    rns_finfo_t* finfo = rns_fmgr_search(channel->fmgr, offset);
    
    if(finfo == NULL || rns_file_size(finfo->fp) < 0 || (uint32_t)rns_file_size(finfo->fp) > channel->recdsize || offset > (uint32_t)rns_file_size(finfo->fp) + rns_fmgr_key(finfo))
    {
        uchar_t buf[128];
        snprintf((char_t*)buf, 128, "%s/%s-%llu.ts", channel->recddir, channel->content_id, offset);
        finfo = rns_fmgr_add(channel->fmgr, buf, offset);
    }
    
    if(finfo == NULL)
    {
        LOG_ERROR(lp, "something is wrong with recd file, channel id : %s, offset : %llu", channel->content_id, offset);
        return;
    }

    int32_t ret = rns_file_write(finfo->fp, block->buf, BLOCK_SIZE);
    if(ret != BLOCK_SIZE)
    {
        LOG_ERROR(lp, "write file failed, file name : %s, ret : %d", finfo->file, ret);
        return;
    }

    LOG_DEBUG(lp, "recd to a block success, channe id : %s, offset : %llu, file : %s, file key : %llu", channel->content_id, offset, finfo->file, rns_fmgr_key(finfo));
    
    block->state = YMS_BLOCK_STATE_SAVED;
    return;
}

static void gop_write(channel_t* channel, gop_t* gop, uchar_t* buf, uint32_t size)
{
    uchar_t* buffer = NULL;
    block_t* block = NULL;

    uint32_t writebytes = 0;
    
    while(writebytes < size)
    {
        uint64_t offset = gop->info.offset + gop->info.len;
        uint64_t offset_align = offset & BLOCK_MASK;
        rbt_node_t* node = rbt_search_int(&channel->blocks, offset_align);

        /* LOG_DEBUG(lp, "gop offset : %d, gop len : %d, offset : %d, offset align : %d ", gop->info.offset, gop->info.len, offset, offset_align); */
        
        if(node == NULL)
        {
            rbt_node_t* lnode = rbt_last(&channel->blocks);
            if(lnode != NULL)
            {
                block_t* lb = rbt_entry(lnode, block_t, node);
                lb->state = YMS_BLOCK_STATE_FULL;
                yms_channel_recd(channel, lb, lnode->key.idx);
            }
            
            buffer = rns_mpool_alloc(channel->mp);
            if(buffer == NULL)
            {
                LOG_WARN(lp, "mpool malloc failed");
                rbt_node_t* fnode = rbt_first(&channel->blocks);
                if(fnode == NULL)
                {
                    return;
                }
                block_t* fb = rbt_entry(fnode, block_t, node);
                while(fb->state != YMS_BLOCK_STATE_SAVED)
                {
                    fnode = rbt_next(fnode);
                    if(fnode == NULL)
                    {
                        LOG_ERROR(lp, "mpool malloc failed and no suitable block can be freed, channel id : %s", channel->content_id);
                        return;
                    }
                    fb = rbt_entry(fnode, block_t, node);
                }

                LOG_DEBUG(lp, "free a old block for new content, channel id : %s, old block : %llu, new block : %llu", channel->content_id, fb->node.key.idx, offset_align);
                rbt_delete(&channel->blocks, &fb->node);
                rbt_set_key_int(&fb->node, offset_align);
                rbt_insert(&channel->blocks, &fb->node);
                block = fb;
                block->state = YMS_BLOCK_STATE_WRITING;
                block->count = 0;
                
            }
            else
            {
                block = (block_t*)rns_malloc(sizeof(block_t));
                if(block == NULL)
                {
                    LOG_ERROR(lp, "malloc failed, size : %d", sizeof(block_t));
                    return;
                }
                block->buf = buffer;
                block->state = YMS_BLOCK_STATE_WRITING;
                block->count = 0;
                rbt_set_key_int(&block->node, offset_align);
                rbt_insert(&channel->blocks, &block->node);
            }
        }
        else
        {
            block = rbt_entry(node, block_t, node);
            if(block->buf == NULL)
            {
                LOG_ERROR(lp, "block searched, but the block buf is null, something may be really wrong");
                return;
            }
        }
        
        uint64_t len = size - writebytes < BLOCK_SIZE - offset + offset_align ? size - writebytes : BLOCK_SIZE - offset + offset_align;
        /* LOG_DEBUG(lp, "write gop buf, buf size : %d, wirte len : %d, offset : %u, offset_align : %u, writebytes : %d", size, len, offset, offset_align, writebytes); */
        rns_memcpy(block->buf + offset - offset_align, buf + writebytes, len);
        gop->info.len += len;
        writebytes += len;
    }

}

static int32_t gop_recd(channel_t* channel, gop_t* gop)
{
    if(channel->gfp == NULL)
    {
        channel->gfp = rns_file_open(channel->gopfile, RNS_FILE_OPEN | RNS_FILE_RW);
        if(channel->gfp == NULL)
        {
            LOG_ERROR(lp, "open gop file failed, channel id : %s, file : %s", channel->content_id, channel->gopfile);
            return -1;
        }
    }

    uchar_t buf[28];
    rns_memset(buf, 28);
    uchar_t* walker = buf;
    
    rns_htob32(gop->info.idx, walker);
    walker += 4;

    rns_htob64(gop->info.offset, walker);
    walker += 8;

    rns_htobd(gop->info.len, walker);
    walker += 8;

    rns_htobd(gop->info.time_start, walker);

    int32_t ret = rns_file_write(channel->gfp, buf, 28);
    if(ret != 28)
    {
        LOG_ERROR(lp, "write to gop file failed, channel id : %s, file : %s", channel->content_id, channel->gopfile);
        return -2;
    }
    
    return 0;
}

static void gop_load_cb(void* data, uchar_t* buffer, uint32_t size)
{
    return;
}

static int32_t gop_load(channel_t* channel)
{
    if(channel->gfp == NULL)
    {
        channel->gfp = rns_file_open(channel->gopfile, RNS_FILE_OPEN | RNS_FILE_READ);
        if(channel->gfp == NULL)
        {
            LOG_ERROR(lp, "open gop file failed, channel id : %s, file : %s", channel->content_id, channel->gopfile);
            return -1;
        }
    }

    int32_t size = rns_file_size(channel->gfp);
    if(size < 0)
    {
        LOG_ERROR(lp, "gop file may has something wrong, channel id : %s, file : %s", channel->content_id, channel->gopfile);
        return -3;
    }

    void* buffer;
    uint32_t buffer_size = ((size >> 9) + 1) << 9;
    int32_t ret = posix_memalign(&buffer, 512, buffer_size);
    if(ret < 0)
    {
        return -4;
    }

    ret = rns_file_read2(channel->gfp, 0, buffer, buffer_size, gop_load_cb, channel);
    if(ret < 0)
    {
        LOG_ERROR(lp, "read file %s failed, content id : %s", channel->gopfile, channel->content_id);
        return -3;
    }
    
    return 0;
}

media_m3u8_t* channel_m3u8_create(channel_t* channel, gop_t* end)
{
    media_m3u8_t* m3u8 = (media_m3u8_t*)rns_malloc(sizeof(media_m3u8_t));
    if(m3u8 == NULL)
    {
        return NULL;
    }
    
    m3u8->version = 3;
    m3u8->max_duration = channel->gopsize;
    m3u8->seg_size = channel->tfragsize;
    
    m3u8->seg = (media_seg_t*)rns_calloc(m3u8->seg_size, sizeof(media_seg_t));
    if(m3u8->seg == NULL)
    {
        rns_free(m3u8);
        return NULL;
    }
    
    gop_t* gop = end;
    for(int32_t i = channel->tfragsize - 1; i >= 0; --i)
    {
        snprintf((char_t*)m3u8->seg[i].uri, 128, "%s-frag%d.ts", channel->content_id, (gop->info.idx / channel->gopsize));
        LOG_DEBUG(lp, "create ts id, ts id : %s, gop id : %d, gop time len : %f, i : %d", m3u8->seg[i].uri, gop->info.idx, gop->info.time_len, i);

        if(i == 0)
        {
            m3u8->media_sequence = (gop->info.idx / channel->gopsize);
        }
        
        for(uint32_t j = 0; j < channel->gopsize; ++j)
        {
            if(gop->info.time_len == 0)
            {
                m3u8->seg[i].time_len += 1;
            }
            else
            {
                m3u8->seg[i].time_len += gop->info.time_len;
            }

            gop = rns_list_prev(&gop->list, &channel->gops, gop_t, list);
            if(gop == NULL)
            {
                rns_free(m3u8->seg);
                rns_free(m3u8);
                return NULL;
            }
        }
    }
    return m3u8;
   
}

gop_t* channel_m3u8_gop_align(channel_t* channel, gop_t* end)
{
    gop_t* gop = end;
    gop_t* end_gop = NULL;

    do
    {
        if(gop->info.idx % channel->gopsize == channel->gopsize - 1)
        {
            break;
        }
    }
    while(gop = rns_list_prev(&gop->list, &channel->gops, gop_t, list));

    if(gop == NULL)
    {
        LOG_WARN(lp, "gop is null, channel id : %s", channel->content_id);
        return NULL;
    }

    end_gop = gop;

    uint32_t number = channel->gopsize * channel->tfragsize;
    for(uint32_t i = 0; i < number; ++i)
    {
        if(gop == NULL)
        {
            LOG_WARN(lp, "gop is null, channel id : %s", channel->content_id);
            return NULL;
        }
        gop = rns_list_prev(&gop->list, &channel->gops, gop_t, list);
    }

    return end_gop;
}

void yms_channel_destroy_m3u8(media_m3u8_t** m3u8)
{
    
}
static void yms_channel_recv_ts_error(rns_hconn_t* hconn, void* data)
{
    LOG_ERROR(lp, "recv ts failed");
}

static int32_t yms_channel_recv_ts(rns_hconn_t* hconn, void* data)
{
    channel_t* channel = (channel_t*)data;
    rns_http_info_t* info = rns_hconn_resp_info(hconn);
    
    LOG_INFO(lp, "recv ts, channel id : %s, ts size : %d", channel->content_id, info->body_size);
    
    uchar_t* walker = info->body;
    uchar_t* end = info->body + info->body_size;

    gop_t* gop = rns_list_last(&channel->gops, gop_t, list);
    
    while(walker < end)
    {
        if(is_pat(walker))
        {
            rns_memcpy(channel->pat, walker, 188);

            ts_pat_t* pat = parse_pat2(channel->pat);
            if(pat == NULL)
            {
                goto CONTINUE;
            }
            list_head_t* p;
            rns_list_for_each(p, &pat->program_head)
            {
                ts_pat_program_t* program = rns_list_entry(p, ts_pat_program_t, list);
                if(program->program_number == 1)
                {
                    channel->pmt_pid = program->program_map_pid;
                }
            }
            pat_destroy(&pat);

            goto CONTINUE;
        }
        
        if(is_pid(walker, channel->pmt_pid))
        {
            rns_memcpy(channel->pmt, walker, 188);

            ts_pmt_t* pmt = parse_pmt2(channel->pmt);
            channel->pcr_pid =  pmt->pcr_pid;

            pmt_destroy(&pmt);

            goto CONTINUE;
        }
        
        if(is_idr(walker))
        {
            gop = (gop_t*)rns_malloc(sizeof(gop_t));
            if(gop == NULL)
            {
                goto EXIT;
            }

            gop->info.idx = channel->gop_id;
            ++channel->gop_id;
            
            gop->info.time_start = (double)get_pcr(walker) / 90000;
            
            gop_t* g = rns_list_last(&channel->gops, gop_t, list);
            if(g != NULL)
            {
                gop->info.offset = g->info.offset + g->info.len;
                g->info.time_len = gop->info.time_start - g->info.time_start;

                int32_t ret = gop_recd(channel, g);
                if(ret < 0)
                {
                    LOG_ERROR(lp, "recd gop failed, channel id : %s, gop file : %s", channel->content_id, channel->gopfile);
                }
                LOG_DEBUG(lp, "add gop to channel, gop id : %d, gop time start : %f, last time len : %f", gop->info.idx, gop->info.time_start, g->info.time_len);
            }

            gop_write(channel, gop, channel->pat, 188);
            gop_write(channel, gop, channel->pmt, 188);
            
            rns_list_add(&gop->list, &channel->gops);

        }

        if(gop != NULL)
        {
            gop_write(channel, gop, walker, 188);
        }
    CONTINUE:
        walker += 188;
    }
    
    yms_channel_req_ts(channel);
    
    gop = rns_list_last(&channel->gops, gop_t, list);
    if(gop == NULL)
    {
        goto EXIT;
    }

    gop = channel_m3u8_gop_align(channel, gop);
    if(gop == NULL)
    {
        LOG_WARN(lp, "gop align falied, channel id : %s", channel->content_id);
        goto EXIT;
    }
    
    media_m3u8_t* m3u8 = channel_m3u8_create(channel, gop);
    if(m3u8 == NULL)
    {
        LOG_WARN(lp, "m3u8 create falied, channel id : %s", channel->content_id);
        goto EXIT;
    }
    
    int32_t ret = m3u8_write(m3u8, channel->m3u8_buf, 1024);
    if(ret <= 0)
    {
        LOG_WARN(lp, "m3u8 write to buf falied, channel id : %s", channel->content_id);
        goto EXIT;
    }
    else
    {
        channel->m3u8_size = ret;
        LOG_DEBUG(lp, "create m3u8 buf success, m3u8 size : %d, \n%s", channel->m3u8_size, channel->m3u8_buf);
    }
    
EXIT:
    return info->body_size;
}

static int32_t str_compare(uchar_t* src, uchar_t* dst)
{
    uint32_t slen = rns_strlen(src);
    uint32_t dlen = rns_strlen(dst);
    if(slen < dlen)
    {
        return -1;
    }
    else if(slen > dlen)
    {
        return 1;
    }
    else
    {
        return rns_strcmp(src, dst);
    }
    
}

static void yms_channel_req_ts(void* data)
{
    channel_t* channel = (channel_t*)data;
    uchar_t ip[32];
    int32_t ret = 0;

    uchar_t* req_buf = rns_malloc(1024);
    if(req_buf == NULL)
    {
        return;
    }
    
    rns_list_t* rlist = rns_list_first(&channel->sfrags, rns_list_t, list);
    if(rlist == NULL)
    {
        return;
    }
    
    uint32_t size = snprintf((char_t*)req_buf, 1024,
                             "GET %s/%s HTTP/1.1\r\n"
                             "Accept: */*\r\n"
                             "Accept-Language: zh-cn\r\n"
                             "Content-Type: application/json\r\n"
                             "Content-Length: %d\r\n"
                             "Connection: keepalive\r\n"
                             "Host: %s:%d\r\n\r\n",
                             channel->suri_ts, rlist->data, 0, rns_ip_int2str(channel->addr.ip, ip, 32), channel->addr.port);

    LOG_DEBUG(lp, "req ts buf, channel id : %s, req : \n%s, %d", channel->content_id, req_buf, size);

    rns_http_cb_t cb;
    rns_memset(&cb, sizeof(rns_http_cb_t));
    cb.work = yms_channel_recv_ts;
    cb.close = yms_channel_recv_ts_error;
    cb.error = yms_channel_recv_ts_error;
    cb.data = channel;
    
    rns_hconn_t* hconn = rns_hconn_get(channel->chttp, &cb);
    if(hconn == NULL)
    {
        LOG_ERROR(lp, "req ts failed,  channel id : %s, sfrag id : %s\n", channel->content_id, channel->sfrag_id);
        return;
    }
    
    ret = rns_hconn_req(hconn, req_buf, size, 1);
    if(ret < 0)
    {
        LOG_ERROR(lp, "req ts failed,  channel id : %s, sfrag id : %s, ret : %d,\n", channel->content_id, channel->sfrag_id, ret);
        rns_hconn_free(hconn);
        return;
    }

    rns_list_del(&rlist->list);
    rns_free(rlist);

    return;
}

static int32_t yms_channel_recv_m3u8(rns_hconn_t* hconn, void* data)
{
    channel_t* channel = (channel_t*)data;
    rns_http_info_t* info = rns_hconn_resp_info(hconn);
    
    LOG_DEBUG(lp, "recv %s, size %d", channel->content_id, info->body_size);


    uint32_t i = 0;
    uint32_t flag = 0;
    int32_t ret = 0;
    
    media_m3u8_t* m3u8 = m3u8_read(info->body, info->body_size);
    
    if(channel->sfrag_id[0] == 0)
    {
        rns_strncpy(channel->sfrag_id, m3u8->seg[0].uri, rns_strlen(m3u8->seg[0].uri));
    }
    
    for(i = 0; i < m3u8->seg_size; ++i)
    {
        if(str_compare(m3u8->seg[i].uri, channel->sfrag_id) <= 0)
        {
            continue;
        }

        flag = 1;

        rns_strncpy(channel->sfrag_id, m3u8->seg[i].uri, rns_strlen(m3u8->seg[i].uri));

        rns_list_t* rlist = (rns_list_t*)rns_malloc(sizeof(rns_list_t));
        if(rlist == NULL)
        {
            return info->body_size;
        }
        rlist->size = rns_strlen(m3u8->seg[i].uri) + 1;
        rlist->data = rns_malloc(rlist->size);
        rns_strncpy(rlist->data, m3u8->seg[i].uri, rlist->size - 1);
        rns_list_add(&rlist->list, &channel->sfrags);
    }
    
    yms_channel_req_ts(channel);

    
    if(flag == 1)
    {
        ret = rns_timer_setdelay(channel->timer, 4000);
        if(ret < 0)
        {
            LOG_ERROR(lp, "set timer failed, channel id : %s", channel->content_id);
        }
        
    }
    else
    {
        ret = rns_timer_setdelay(channel->timer, 500);
        if(ret < 0)
        {
            LOG_ERROR(lp, "set timer failed, channel id : %s", channel->content_id);
        }
    }
    
    m3u8_destroy(&m3u8);
    return info->body_size;
}

static void yms_channel_recv_m3u8_error(rns_hconn_t* hconn, void* data)
{
    LOG_ERROR(lp, "recv m3u8 failed");
    channel_t* channel = (channel_t*)data;

    yms_channel_req_m3u8(channel);
}

void yms_channel_req_m3u8(void* data)
{
    channel_t* channel = (channel_t*)data;
    uchar_t ip[32];

    uchar_t* req_buf = rns_malloc(1024);
    if(req_buf == NULL)
    {
        return;
    }
    uint32_t size = snprintf((char_t*)req_buf, 1024,
                             "GET %s HTTP/1.1\r\n"
                             "Accept: */*\r\n"
                             "Accept-Language: zh-cn\r\n"
                             "Content-Type: application/json\r\n"
                             "Content-Length: %d\r\n"
                             "Connection: keepalive\r\n"
                             "Host: %s:%d\r\n\r\n",
                             channel->suri, 0, rns_ip_int2str(channel->addr.ip, ip, 32), channel->addr.port);

    rns_http_cb_t cb;
    rns_memset(&cb, sizeof(rns_http_cb_t));
    cb.work = yms_channel_recv_m3u8;
    cb.close = yms_channel_recv_m3u8_error;
    cb.error = yms_channel_recv_m3u8_error;
    cb.data = channel;

    rns_hconn_t* hconn = rns_hconn_get(channel->chttp, &cb);
    if(hconn == NULL)
    {
        LOG_ERROR(lp, "req ts failed,  channel id : %s, sfrag id : %s\n", channel->content_id, channel->sfrag_id);
        return;
    }

    LOG_DEBUG(lp, "req m3u8 success, channel id : %s, req ip : %u, req port : %d, req buf : \n%s", channel->content_id, channel->addr.ip, channel->addr.port, req_buf);
    
    int32_t ret = rns_hconn_req(hconn, req_buf, size, 1);
    if(ret < 0)
    {
        LOG_ERROR(lp, "req m3u8 failed, channel id : %s, req ip : %u, req port : %d, ret : %d", channel->content_id, channel->addr.ip, channel->addr.port, ret);
        rns_hconn_free(hconn);
        return;
    }
    

    return;
}

channel_t* yms_channel_create(rns_epoll_t* ep, rns_pipe_t* pp, uchar_t* content_id, uchar_t* surl, uchar_t* recddir, uint64_t recdsize, uint32_t gopsize, uint32_t tfragsize)
{
    channel_t* channel = (channel_t*)rns_malloc(sizeof(channel_t));
    if(channel == NULL)
    {
        return NULL;
    }

    channel->ep = ep;
    channel->rpipe = pp;

    uchar_t* ptr = NULL;
    int32_t ret = posix_memalign((void**)&ptr, 512, BLOCK_SIZE * BLOCK_NUM);
    if(ptr == NULL)
    {
        LOG_ERROR(lp, "malloc failed, size : %d", BLOCK_SIZE * BLOCK_NUM);
        goto ERR_EXIT;
    }
    
    channel->mp = rns_mpool_init(ptr, BLOCK_SIZE * BLOCK_NUM, BLOCK_SIZE);
    if(channel->mp == NULL)
    {
        LOG_ERROR(lp, "init mpool failed, channel id : %s, totol size : %u, block size : %u", channel->content_id, BLOCK_SIZE * BLOCK_NUM, BLOCK_SIZE);
        goto ERR_EXIT;
    }
    rbt_init(&channel->blocks, RBT_TYPE_INT);

    
    channel->content_id = rns_malloc(rns_strlen(content_id) + 1);
    if(channel->content_id == NULL)
    {
        LOG_ERROR(lp, "malloc falied, content id : %s, url : %s, recddir : %s", rns_strlen(content_id) + 1, channel->content_id, surl, recddir);
        goto ERR_EXIT;
    }
    rns_strncpy(channel->content_id, content_id, rns_strlen(content_id));

    channel->recddir = rns_malloc(rns_strlen(recddir) + rns_strlen(content_id) + 3);
    if(channel->recddir == NULL)
    {
        LOG_ERROR(lp, "malloc falied, size : %d, content id : %s, url : %s, recddir : %s", rns_strlen(recddir) + 1, channel->content_id, surl, recddir);
        goto ERR_EXIT;
    }
    snprintf((char_t*)channel->recddir, rns_strlen(recddir) + rns_strlen(content_id) + 3, "%s/%s/", recddir, content_id);

    channel->recdsize = recdsize;

    ret = rns_dir_create(channel->recddir, rns_strlen(channel->recddir), 0755);
    if(ret < 0)
    {
        LOG_WARN(lp, "create dir failed, channel id : %s, recddir : %s", channel->content_id, channel->recddir);
    }
    
    channel->gopfile = rns_malloc(rns_strlen(channel->recddir) + 5);
    snprintf((char_t*)channel->gopfile, rns_strlen(channel->recddir) + 5, "%s/gop", channel->recddir);
    channel->gfp = rns_file_open(channel->gopfile, RNS_FILE_OPEN | RNS_FILE_RW);
    if(channel->gfp == NULL)
    {
        LOG_ERROR(lp, "open gop file failed, channel id : %s, gop file : %s", channel->content_id, channel->gopfile);
        goto ERR_EXIT;
    }
    
    ret = rns_epoll_add(channel->ep, channel->gfp->eventfd, channel->gfp, RNS_EPOLL_IN);
    if(ret < 0)
    {
        LOG_ERROR(lp, "add file to epoll failed, channel id : %s, gop file : %s, rfd : %d, ret : %d, errno : %d\n", channel->content_id, channel->gopfile, channel->gfp->rfd, ret, errno);
        goto ERR_EXIT;
    }

    channel->gopsize = gopsize;
    channel->tfragsize = tfragsize;
    
    channel->surl = rns_malloc(rns_strlen(surl) + 1);
    if(channel->surl == NULL)
    {
        LOG_ERROR(lp, "malloc failed, size : %d\n", rns_strlen(surl) + 1);
        goto ERR_EXIT;
    }
    
    ret = gop_load(channel);
    if(ret < 0)
    {
        LOG_ERROR(lp, "load gop failed, content id : %s", channel->content_id);
        goto ERR_EXIT;
    }
    
    LOG_INFO(lp, "add channel to yms success, channel id : %s, suri : %s, suri ts : %s, recd : %s, ip : %u, port : %d", channel->content_id, channel->suri, channel->suri_ts, channel->recddir, channel->addr.ip, channel->addr.port);
    
    return channel;

ERR_EXIT:
    if(channel != NULL)
    {
        if(channel->surl == NULL)
        {
            rns_free(channel->surl);
        }
        if(channel->content_id != NULL)
        {
            rns_free(channel->content_id);
        }
        if(channel->recddir != NULL)
        {
            rns_free(channel->recddir);
        }
        if(channel->suri != NULL)
        {
            rns_free(channel->suri);
        }
        if(channel->suri_ts != NULL)
        {
            rns_free(channel->suri_ts);
        }
        rns_free(channel);
    }
    if(ptr != NULL)
    {
        rns_free(ptr);
    }

    return NULL;
}

void yms_channel_start2(void* ctx, void* data, uint32_t ip)
{
    channel_t* channel = (channel_t*)ctx;
    
    channel->addr.ip = ip;
    
    uchar_t recd[128];
    snprintf((char_t*)recd, 128, "%s/recd", channel->recddir);
    
    channel->fmgr = rns_fmgr_load(recd);
    if(channel->fmgr == NULL)
    {
        LOG_ERROR(lp, "load file mgr failed, channel id : %s, fmgr : %s", channel->content_id, recd);
        return;
    }
    
    channel->chttp = rns_httpclient_create(channel->ep, &channel->addr, 20, NULL);
    if(channel->chttp == NULL)
    {
        LOG_ERROR(lp, "create cpool failed, channel id : %s, channel ip : %u, channel port : %d", channel->content_id, channel->addr.ip, channel->addr.port);
        rns_fmgr_close(&channel->fmgr);
        return;
    }
    
    channel->timer = rns_timer_create();
    if(channel->timer == NULL)
    {
        LOG_ERROR(lp, "create timer failed, channel id : %s", channel->content_id);
        rns_http_destroy(&channel->chttp);
        rns_fmgr_close(&channel->fmgr);
        return;
    }
    
    rns_cb_t cb;
    cb.func = yms_channel_req_m3u8;
    cb.data = channel;
    int32_t ret = rns_timer_set(channel->timer, 4000, &cb);
    if(ret < 0)
    {
        LOG_ERROR(lp, "create timer failed, channel id : %s", channel->content_id);
        rns_http_destroy(&channel->chttp);
        rns_fmgr_close(&channel->fmgr);
        return;
    }
    ret = rns_epoll_add(channel->ep, channel->timer->timerfd, channel->timer, RNS_EPOLL_IN);
    if(ret < 0)
    {
        LOG_ERROR(lp, "create timer failed, channel id : %s", channel->content_id);
        rns_http_destroy(&channel->chttp);
        rns_fmgr_close(&channel->fmgr);
        return;
    }
    
    
    LOG_INFO(lp, "start channel success, channel id : %s, suri : %s, suri ts : %s, recd : %s, ip : %u, port : %d", channel->content_id, channel->suri, channel->suri_ts, channel->recddir, channel->addr.ip, channel->addr.port);

    return;
}

int32_t yms_channel_start(channel_t* channel)
{
    channel->max_duration = 6;
    
    INIT_LIST_HEAD(&channel->gops);
    INIT_LIST_HEAD(&channel->sfrags);
    int32_t ret = 0;
    
    rns_http_url_t* http_url = rns_http_url_create(channel->surl);
    if(http_url == NULL)
    {
        LOG_ERROR(lp, "url create failed, content id : %s, url : %s, recddir : %s", channel->content_id, channel->surl, channel->recddir);
        return -1;
    }
    
    uint32_t type = rns_ip_type(http_url->addr, rns_strlen(http_url->addr));
    if(type == RNS_URL_TYPE_DOMAIN)
    {
        rns_addr_cb_t cb;
        cb.ctx = channel;
        cb.data = NULL;
        cb.func = yms_channel_start2;
        
        ret = rns_ip_getaddr(http_url->addr, &cb);
        if(ret < 0)
        {
            LOG_ERROR(lp, "resolve domain failed id : %s, url : %s, domain : %s", channel->content_id, channel->surl, http_url->addr);
            rns_free(http_url);
            return -2;
        }
    }
    else
    {
        yms_channel_start2(channel, NULL, rns_ip_str2int(http_url->addr));
    }
    
    channel->suri = rns_malloc(rns_strlen(http_url->uri) + 1);
    rns_strncpy(channel->suri, http_url->uri, rns_strlen(http_url->uri) + 1);

    uchar_t* ptr = rns_strrchr(channel->suri, '/');
    if(ptr == NULL)
    {
        LOG_ERROR(lp, "url may be wrong, content id : %s, url : %s, recddir : %s", channel->content_id, channel->surl, channel->recddir);
        rns_free(channel->suri);
        rns_free(http_url);
        return -3;
    }
    
    channel->suri_ts = rns_malloc(ptr - channel->suri + 1);
    if(channel->suri_ts == NULL)
    {
        LOG_ERROR(lp, "malloc falied, size : %d, content id : %s, url : %s, recddir : %s", ptr - channel->suri + 1, channel->content_id, channel->surl, channel->recddir);
        rns_free(channel->suri);
        rns_free(http_url);
        return -3;
    }
    rns_strncpy(channel->suri_ts, channel->suri, ptr - channel->suri);
    
    channel->addr.port = http_url->port;
    
    return 0;
}

void yms_channel_destroy(channel_t** channel)
{
    list_head_t* p;
    rns_list_for_each(p, &(*channel)->gops)
    {
        gop_t* gop = rns_list_entry(p, gop_t, list);
        rns_free(gop);
    }
    rns_fmgr_close(&(*channel)->fmgr);
    rns_free((*channel)->mp);

    rns_epoll_delete((*channel)->ep, (*channel)->gfp->rfd);
    rns_file_close(&(*channel)->gfp);
    
    rns_free(*channel);
    *channel = NULL;
    
    return;
}

void yms_channel_stop(channel_t* channel)
{
    return;
}

static int32_t req_parse(rns_hconn_t *hconn, yms_live_req_t* req)
{
    rns_http_info_t* info = rns_hconn_req_info(hconn);
    uint32_t len = rns_strlen(info->uri);
    
    if(!rns_strncmp(&(info->uri[len - 5]), ".m3u8", 5))
    {
        req->is_m3u8 = 1;
        uchar_t* p = rns_strrchr(info->uri, '.');
        uchar_t* q = rns_strrchr(info->uri, '/');
        if(p == NULL || q == NULL)
        {
            LOG_ERROR(lp, "uri parse failed, uri : %s", info->uri);
            return -1;
        }
        
        rns_strncpy(req->content_id, q + 1, p - q - 1);

        rbt_node_t* node = rbt_search_str(&info->args, (uchar_t*)"playseek");
        if(node != NULL)
        {
            rns_node_t* item = rbt_entry(node, rns_node_t, node);
            req->playseek = rns_atoi(item->data);
        }
    }
    else
    {
        req->is_m3u8 = 0;
        
        uchar_t* p = rns_strrchr(info->uri, '-');
        uchar_t* q = rns_strrchr(info->uri, '/');
        uchar_t* o = rns_strrchr(info->uri, '.');
        if(p == NULL || q == NULL || o == NULL)
        {
            LOG_ERROR(lp, "uri parse failed, uri : %s", info->uri);
            return -1;
        }
        
        rns_strncpy(req->content_id, q + 1, p - q - 1);
        uchar_t* buf = rns_malloc(o - p - 5 + 1);
        if(buf == NULL)
        {
            return -2;
        }
        rns_strncpy(buf, p + 5, o - p - 5);
        req->frag_id = rns_atoi(buf);
        
    }

    return 0;
}

static void yms_attach_buffer(rns_hconn_t* hconn, channel_t* channel, gop_t* gop)
{
    uint32_t sendbytes = 0;
    while(sendbytes < gop->info.len)
    {
        uint64_t offset = gop->info.offset + sendbytes;
        uint64_t offset_align = offset & BLOCK_MASK;
        rbt_node_t* node = rbt_search_int(&channel->blocks, offset_align);
        if(node == NULL)
        {
            continue;
        }
        block_t* block = rbt_entry(node, block_t, node);
        uint32_t len = BLOCK_SIZE - (offset - offset_align) < gop->info.len - sendbytes ? BLOCK_SIZE - (offset - offset_align) : gop->info.len - sendbytes;
        
        int32_t ret = rns_http_attach_buffer(hconn, block->buf + offset - offset_align, len, 0);
        if(ret < 0)
        {
            LOG_ERROR(lp, "attach buffer failed, ret : %d, gop id : %d, gop len : %d, attach buf len : %d, sendbytes : %d", ret, gop->info.idx, gop->info.len, len, sendbytes);
        }

        sendbytes += len;
        LOG_DEBUG(lp, "attach buffer, gop id : %d, gop len : %d, attach buf len : %d, sendbytes : %d", gop->info.idx, gop->info.len, len, sendbytes);
    }
}


int32_t yms_live_req(rns_hconn_t *hconn, channel_t* channel) 
{
    if(channel == NULL)
    {
        LOG_ERROR(lp, "channel is null");
        goto ERR_EXIT;
    }

    rns_http_info_t* info = rns_hconn_req_info(hconn);
    
    yms_live_req_t req;
    memset(&req, 0, sizeof(yms_live_req_t));
    int32_t ret = req_parse(hconn, &req);
    if(ret < 0)
    {
        LOG_ERROR(lp, "client req parse failed, req uri : %s", info->uri);
        goto ERR_EXIT;
    }

    LOG_INFO(lp, "req info, uri : %s, client ip : %u, port : %u, content id : %s, frag id : %d, is m3u8 : %d, playseek : %d", info->uri, hconn->addr.ip, hconn->addr.port, req.content_id, req.frag_id, req.is_m3u8, req.playseek);

    if(!req.is_m3u8)
    {
        uint32_t id = 0;
        id = req.frag_id * channel->gopsize;
        
        list_head_t* p;
        gop_t* gop = NULL;
        rns_rns_list_for_each_r(p, &channel->gops)
        {
            gop = rns_list_entry(p, gop_t, list);
            if(gop->info.idx == id)
            {
                break;
            }
            gop = NULL;
        }
        
        if(gop == NULL)
        {
            LOG_WARN(lp, "find gop failed, channel id : %s, gop id : %d", channel->content_id, id);
            goto ERR_EXIT;
        }
        
        LOG_DEBUG(lp, "client req ts, content id : %s, frag id : %d, start gop id : %d", channel->content_id, req.frag_id, gop->info.idx);
        uint32_t ts_size = 0;

        gop_t* g = gop;
        for(uint32_t i = 0; i < channel->gopsize; ++i)
        {
            if(g == NULL)
            {
                LOG_WARN(lp, "attach buffer may has something wrong, only %d gop attached", i);
                goto ERR_EXIT;
            }
            ts_size += g->info.len;
            g = rns_list_next(&g->list, &channel->gops, gop_t, list);
            
        }
        uchar_t str[128];
        snprintf((char_t*)str, 128, "%d", ts_size);
        ret = rns_hconn_resp_header_insert(hconn, (uchar_t*)"Content-Length", str);
        if(ret < 0)
        {
            LOG_ERROR(lp, "insert respose header  failed, content length : %s", str);
            goto ERR_EXIT;
        }
        
        ret = rns_hconn_resp_header(hconn, 200);
        if(ret < 0)
        {
            LOG_ERROR(lp, "respose header to client failed, retcode : 200, body size : 0");
            goto ERR_EXIT;
        }

        for(uint32_t i = 0; i < channel->gopsize; ++i)
        {
            if(gop == NULL)
            {
                LOG_WARN(lp, "attach buffer may has something wrong, only %d gop attached", i);
                goto ERR_EXIT;
            }
            yms_attach_buffer(hconn, channel, gop);
            gop = rns_list_next(&gop->list, &channel->gops, gop_t, list);
        }
        ret = rns_hconn_resp(hconn);
        if(ret < 0)
        {
            LOG_ERROR(lp, "resp done failed, ret : %d", ret);
        }
    }
    else 
    {
        if(req.playseek == 0)
        {
            if(channel->m3u8_size == 0)
            {
                LOG_ERROR(lp, "channel %s are not ready", channel->content_id);
                goto ERR_EXIT;
            }

            uchar_t str[128];
            snprintf((char_t*)str, 128, "%d", channel->m3u8_size);
            ret = rns_hconn_resp_header_insert(hconn, (uchar_t*)"Content-Length", str);
            if(ret < 0)
            {
                LOG_ERROR(lp, "insert respose header  failed, content length : %s", str);
                goto ERR_EXIT;
            }
            
            ret = rns_hconn_resp_header(hconn, 200);
            if(ret < 0)
            {
                LOG_ERROR(lp, "respose header to client failed, retcode : 200, body size : 0");
            }
            
            ret = rns_http_attach_buffer(hconn, channel->m3u8_buf, channel->m3u8_size, 0);
            if(ret < 0)
            {
                LOG_ERROR(lp, "attach buffer to client failed, ret : %d", ret);
            }
            ret = rns_hconn_resp(hconn);
            if(ret < 0)
            {
                LOG_ERROR(lp, "resp done failed, ret : %d", ret);
            }
           
            LOG_DEBUG(lp, "send m3u8 buffer success, size : %d, \n%s", channel->m3u8_size, channel->m3u8_buf);
        }
        else
        {
            uint32_t utc = time(NULL) - req.playseek;
            list_head_t* p;
            gop_t* gop = NULL;
            rns_list_for_each(p, &channel->gops)
            {
                gop = rns_list_entry(p, gop_t, list);
                if(gop->info.time_start < utc)break;
            }
            
            if(gop == NULL)
            {
                goto ERR_EXIT;
            }
            
            gop = channel_m3u8_gop_align(channel, gop);
            if(gop == NULL)
            {
                LOG_WARN(lp, "gop align falied, channel id : %s", channel->content_id);
                goto ERR_EXIT;
            }
            
            media_m3u8_t* m3u8 = channel_m3u8_create(channel, gop);
            if(m3u8 == NULL)
            {
                LOG_WARN(lp, "m3u8 create falied, channel id : %s", channel->content_id);
                goto ERR_EXIT;
            }
            
            ret = m3u8_write(m3u8, channel->resp_buf, 1024);
            if(ret < 0)
            {
                LOG_WARN(lp, "m3u8 write to buf falied, channel id : %s", channel->content_id);
                goto ERR_EXIT;
            }
            
            uint32_t m3u8_size = ret;
            uchar_t str[128];
            snprintf((char_t*)str, 128, "%d", m3u8_size);
            ret = rns_hconn_resp_header_insert(hconn, (uchar_t*)"Content-Length", str);
            if(ret < 0)
            {
                LOG_ERROR(lp, "insert respose header  failed, content length : %s", str);
                goto ERR_EXIT;
            }
            
            ret = rns_hconn_resp_header(hconn, 200);
            if(ret < 0)
            {
                LOG_ERROR(lp, "respose header to client failed, retcode : 200, body size : %d", m3u8_size);
            }
            
            ret = rns_http_attach_buffer(hconn, channel->resp_buf, m3u8_size, 0);
            if(ret < 0)
            {
                LOG_ERROR(lp, "attach buf to client failed, body size : %d", m3u8_size);
            }
            ret = rns_hconn_resp(hconn);
            if(ret < 0)
            {
                LOG_ERROR(lp, "resp done failed, ret : %d", ret);
            }
        }
    }
   
    
    return 0;
	
ERR_EXIT:

    LOG_ERROR(lp, "respose header to client, retcode : 404, body size : 0");
    return rns_hconn_resp_header(hconn, 404);

}

