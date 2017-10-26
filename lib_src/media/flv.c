#include "flv.h"


uint32_t flv_read_header(flv_header_t* header, uchar_t* buf, uchar_t* end)
{
    uchar_t* walker = buf;

    walker += 3;
    
    header->version = *walker;
    walker += 1;

    header->audio = ((*walker) | 0x04) > 2;
    header->video = (*walker) | 0x01;
    walker += 1;

    header->data_offset = rns_btoh32(walker);

    return walker - buf;
}

uint32_t flv_write_header(flv_header_t* header, uchar_t* buf, uchar_t* end)
{
    uchar_t* walker = buf;

    *walker = 0x46;
    walker += 1;
    
    *walker = 0x4C;
    walker += 1;
    
    *walker = 0x56;
    walker += 1;

    *walker = header->version;
    walker += 1;

    *walker = (header->audio << 2) | (header->video);
    walker += 1;

    rns_htob32(header->data_offset, walker);

    return walker - buf;
}

uint32_t flv_read_tag_header(flv_tag_header_t* tag_header, uchar_t* buf, uchar_t* end)
{
    uchar_t* walker = buf;
    
    tag_header->filter = *walker & 0x20;
    tag_header->tag_type = *walker & 0x1F;
    walker += 1;
    
    tag_header->data_size = rns_btoh24(walker);
    walker += 3;
    
    tag_header->time_stamp = rns_btoh24(walker);
    walker +=3;
    
    tag_header->time_stamp += (*walker << 24);
    walker += 1;
    
    tag_header->stream_id = rns_btoh32(walker);
    walker += 4;

    return walker - buf;
}

uint32_t flv_write_tag_header(flv_tag_header_t* tag_header, uchar_t* buf, uchar_t* end)
{
    uchar_t* walker = buf;
    
    
    *walker = (tag_header->filter << 5) | tag_header->tag_type;
    ++walker;
    
    rns_htob24(tag_header->data_size, walker);
    walker += 3;
    
    rns_htob24(tag_header->time_stamp, walker);
    walker += 3;
    
    *walker = tag_header->time_stamp >> 24;
    ++walker;
    
    rns_htob24(tag_header->stream_id, walker);
    walker += 3;

    return walker - buf;
}

// a whole tag
uint32_t flv_read_tag(flv_tag_t* tag, uchar_t* buf, uchar_t* end)
{
    uint32_t data_size = 0;
    
    uchar_t* walker = buf;
    uint32_t done = 0;
    
    while(!done)
    {
        switch (tag->state)
        {
            case FLV_STATE_TAG_HEADER :
            {
                walker += flv_read_tag_header(&tag->tag_header, walker, end);
                data_size = tag->tag_header.data_size;
                
                if(tag->tag_header.tag_type == FLV_PACKET_TYPE_AUDIO)
                {
                    tag->state = FLV_STATE_AUDIO_HEADER;
                }
                else if(tag->tag_header.tag_type == FLV_PACKET_TYPE_VIDEO)
                {
                    tag->state = FLV_STATE_VIDEO_HEADER;
                }
                else if(tag->tag_header.tag_type == FLV_PACKET_TYPE_SCRIPT)
                {
                    tag->state = FLV_STATE_DATA_ATTR;
                }
                break;
            }
            case FLV_STATE_AUDIO_HEADER :
            {
                tag->data_header.audio_header.sound_format = (*walker) & 0xF0;
                tag->data_header.audio_header.rate = (*walker) & 0x0C;
                tag->data_header.audio_header.sample_size = (*walker) & 0x02;
                tag->data_header.audio_header.type = (*walker) & 0x01;
                walker += 1;
                
                data_size -= 1; 
                
                if(tag->data_header.audio_header.sound_format == 10)
                {
                    tag->state = FLV_STATE_AUDIO_PACKET_TYPE;
                }
                else
                {
                    tag->state = FLV_STATE_DATA_ATTR;
                }
                break;
            }
            case FLV_STATE_AUDIO_PACKET_TYPE :
            {
                tag->data_header.audio_header.packet_type = *walker;;
                ++walker;
                
                data_size -= 1;
                tag->state = FLV_STATE_DATA_ATTR;
                break;
            }
            case FLV_STATE_VIDEO_HEADER :
            {
                tag->data_header.video_header.frame_type = (*walker) & 0xF0;
                tag->data_header.video_header.codec_id = (*walker) & 0x0F;
                ++walker;
                
                data_size -= 1;
                
                if(tag->data_header.video_header.codec_id == 7)
                {
                    tag->state = FLV_STATE_VIDEO_PACKET_TYPE;
                }
                else
                {
                    tag->state = FLV_STATE_DATA_ATTR;
                }
                break;
            }
            case FLV_STATE_VIDEO_PACKET_TYPE :
            {
                tag->data_header.video_header.packet_type = *walker;
                ++walker;
                
                tag->data_header.video_header.composition_time = rns_btoh24(walker);
                walker += 3;
                
                data_size -= 4;
                
                tag->state = FLV_STATE_DATA_ATTR;
                break;
            }
            case FLV_STATE_DATA_ATTR:
            {
                if(tag->tag_header.filter)
                {
                    
                }
                else
                {
                    tag->state = FLV_STATE_DATA;
                }
                break;
            }
            case FLV_STATE_DATA :
            {
                tag->data = (uchar_t*)rns_malloc(data_size);
                memcpy(tag->data, walker, data_size);
                walker += data_size;
                
                tag->state = FLV_STATE_DONE;
                break;
            }
            case FLV_STATE_DONE :
            {
                done = 1;
                break;
            }
            default :
            {
                return 0;
            }
        }
    }

    return walker - buf;
}

// a whole tag
uint32_t flv_write_tag(flv_tag_t* tag, uchar_t* buf, uchar_t* end)
{
    uchar_t* walker = buf;
    uint32_t data_size = 0;
    uint32_t done = 0;
    
    while(!done)
    {
        switch (tag->state)
        {
            case FLV_STATE_TAG_HEADER :
            {
                walker += flv_write_tag_header(&tag->tag_header, walker, end);
                data_size = tag->tag_header.data_size;
                
                if(tag->tag_header.tag_type == FLV_PACKET_TYPE_AUDIO)
                {
                    tag->state = FLV_STATE_AUDIO_HEADER;
                }
                else if(tag->tag_header.tag_type == FLV_PACKET_TYPE_VIDEO)
                {
                    tag->state = FLV_STATE_VIDEO_HEADER;
                }
                else if(tag->tag_header.tag_type == FLV_PACKET_TYPE_SCRIPT)
                {
                    tag->state = FLV_STATE_DATA_ATTR;
                }
                break;
            }
            case FLV_STATE_AUDIO_HEADER :
            {
                *walker = (tag->data_header.audio_header.sound_format << 4) | (tag->data_header.audio_header.rate << 2) | (tag->data_header.audio_header.sample_size << 1) | (tag->data_header.audio_header.type);
                ++walker;
                --data_size;
                
                if(tag->data_header.audio_header.sound_format == 10)
                {
                    tag->state = FLV_STATE_AUDIO_PACKET_TYPE;
                }
                else
                {
                    tag->state = FLV_STATE_DATA_ATTR;
                }
                break;
            }
            case FLV_STATE_AUDIO_PACKET_TYPE :
            {
                *walker = tag->data_header.audio_header.packet_type;
                ++walker;
                --data_size;
                
                tag->state = FLV_STATE_DATA_ATTR;
                break;
            }
            case FLV_STATE_VIDEO_HEADER :
            {
                *walker = (tag->data_header.video_header.frame_type << 4) | tag->data_header.video_header.codec_id;
                ++walker;
                --data_size;
                if(tag->data_header.video_header.codec_id == 7)
                {
                    tag->state = FLV_STATE_VIDEO_PACKET_TYPE;
                }
                else
                {
                    tag->state = FLV_STATE_DATA_ATTR;
                }
                break;
            }
            case FLV_STATE_VIDEO_PACKET_TYPE :
            {
                *walker = tag->data_header.video_header.packet_type;
                ++walker;
                
                rns_htob24(tag->data_header.video_header.composition_time, walker);
                walker += 3;
                data_size -= 4;
                
                tag->state = FLV_STATE_DATA_ATTR;
                break;
            }
            case FLV_STATE_DATA_ATTR:
            {
                if(tag->tag_header.filter)
                {
                    
                }
                else
                {
                    tag->state = FLV_STATE_DATA;
                }
                break;
            }
            case FLV_STATE_DATA :
            {
                memcpy(walker, tag->data, data_size);
                walker += data_size;
                
                tag->state = FLV_STATE_DONE;
                break;
            }
            case FLV_STATE_DONE :
            {
                done = 1;
                break;
            }
            default :
            {
                return 0;
            }
        }
    }

    return walker - buf;
}

int32_t flv_read_file(flv_t* flv, uchar_t* file)
{
    rns_file_t* fp = rns_file_open(file, RNS_FILE_OPEN | RNS_FILE_RW);
    if(fp == NULL)
    {
        goto ERR_EXIT;
    }

    rns_strncpy(flv->file, file, sizeof(flv->file));

    uint32_t buf_size = 10240;
    uchar_t* buf = (uchar_t*)rns_malloc(buf_size);
    if(buf == NULL)
    {
        goto ERR_EXIT;
    }
    uint32_t size = 0;
    uint32_t data_size = 0;

    flv_mod_t* mod = NULL;
    uchar_t* walker = buf;
    uchar_t* end = buf;

    uint32_t state_save = FLV_STATE_BUF_NEED;

    int32_t ret = 0;

    uint32_t done = 0;
    
    while(!done)
    {
        switch (flv->state)
        {
            case FLV_STATE_BUF_NEED :
            {
                ret = rns_file_sseek_backward(fp, end - walker);
                if(ret < 0)
                {
                    goto ERR_EXIT;
                }
                size = rns_file_read(fp, buf, buf_size);
                if(size < 0)
                {
                    goto ERR_EXIT;
                }
                walker = buf;
                end = buf + size;

                break;
            }
            case FLV_STATE_FILE_HEADER :
            {
                if(end - walker < 9)
                {
                    flv->state = FLV_STATE_BUF_NEED;
                    state_save = FLV_STATE_FILE_HEADER;
                    break;
                }
                
                walker += flv_read_header(&flv->header, walker, end);
                flv->state = FLV_STATE_PREV_SIZE;
                break;
            }
            case FLV_STATE_PREV_SIZE :
            {
                if(end - walker < 4)
                {
                    flv->state = FLV_STATE_BUF_NEED;
                    state_save = FLV_STATE_PREV_SIZE;
                    break;
                }
                mod = (flv_mod_t*)rns_malloc(sizeof(flv_mod_t));
                if(mod == NULL)
                {
                    goto ERR_EXIT;
                }
                mod->prev_tag_size = rns_btoh32(walker);
                walker += 4;

                flv->state = FLV_STATE_TAG_HEADER;
                break;
            }
            case FLV_STATE_TAG_HEADER :
            {
                if(end - walker < 11)
                {
                    flv->state = FLV_STATE_BUF_NEED;
                    state_save = FLV_STATE_TAG_HEADER;
                    break;
                }
                
                walker += flv_read_tag_header(&mod->tag.tag_header, walker, end);
                data_size = mod->tag.tag_header.data_size;
                
                if(mod->tag.tag_header.tag_type == FLV_PACKET_TYPE_AUDIO)
                {
                    flv->state = FLV_STATE_AUDIO_HEADER;
                }
                else if(mod->tag.tag_header.tag_type == FLV_PACKET_TYPE_VIDEO)
                {
                    flv->state = FLV_STATE_VIDEO_HEADER;
                }
                else if(mod->tag.tag_header.tag_type == FLV_PACKET_TYPE_SCRIPT)
                {
                    flv->state = FLV_STATE_DATA_ATTR;
                }
                break;
            }
            case FLV_STATE_AUDIO_HEADER :
            {
                if(end - walker < 1)
                {
                    flv->state = FLV_STATE_BUF_NEED;
                    state_save = FLV_STATE_AUDIO_HEADER;
                    break;
                }
                
                mod->tag.data_header.audio_header.sound_format = (*walker) & 0xF0;
                mod->tag.data_header.audio_header.rate = (*walker) & 0x0C;
                mod->tag.data_header.audio_header.sample_size = (*walker) & 0x02;
                mod->tag.data_header.audio_header.type = (*walker) & 0x01;
                walker += 1;

                data_size -= 1; 
                
                if(mod->tag.data_header.audio_header.sound_format == 10)
                {
                    flv->state = FLV_STATE_AUDIO_PACKET_TYPE;
                }
                else
                {
                    flv->state = FLV_STATE_DATA_ATTR;
                }
                break;
            }
            case FLV_STATE_AUDIO_PACKET_TYPE :
            {
                if(end - walker < 1)
                {
                    flv->state = FLV_STATE_BUF_NEED;
                    state_save = FLV_STATE_AUDIO_PACKET_TYPE;
                    break;
                }
                mod->tag.data_header.audio_header.packet_type = *walker;;
                ++walker;

                data_size -= 1;
                flv->state = FLV_STATE_DATA_ATTR;
                break;
            }
            case FLV_STATE_VIDEO_HEADER :
            {
                if(end - walker < 1)
                {
                    flv->state = FLV_STATE_BUF_NEED;
                    state_save = FLV_STATE_VIDEO_HEADER;
                    break;
                }
                mod->tag.data_header.video_header.frame_type = (*walker) & 0xF0;
                mod->tag.data_header.video_header.codec_id = (*walker) & 0x0F;
                ++walker;

                data_size -= 1;
                
                if(mod->tag.data_header.video_header.codec_id == 7)
                {
                    flv->state = FLV_STATE_VIDEO_PACKET_TYPE;
                }
                else
                {
                    flv->state = FLV_STATE_DATA_ATTR;
                }
                break;
            }
            case FLV_STATE_VIDEO_PACKET_TYPE :
            {
                if(end - walker < 4)
                {
                    flv->state = FLV_STATE_BUF_NEED;
                    state_save = FLV_STATE_VIDEO_PACKET_TYPE;
                    break;
                }
                mod->tag.data_header.video_header.packet_type = *walker;
                ++walker;
                
                mod->tag.data_header.video_header.composition_time = rns_btoh24(walker);
                walker += 3;

                data_size -= 4;
                
                flv->state = FLV_STATE_DATA_ATTR;
                break;
            }
            case FLV_STATE_DATA_ATTR:
            {
                if(mod->tag.tag_header.filter)
                {
                    
                }
                else
                {
                    flv->state = FLV_STATE_DATA;
                }
                break;
            }
            case FLV_STATE_DATA :
            {
                if(end < data_size + walker)
                {
                    flv->state = FLV_STATE_BUF_NEED;
                    state_save = FLV_STATE_DATA;
                    break;
                }
                
                mod->tag.data = (uchar_t*)rns_malloc(data_size);
                memcpy(mod->tag.data, walker, data_size);
                walker += data_size;

                flv->state = FLV_STATE_DONE;
                break;
            }
            case FLV_STATE_DONE :
            {
                rns_list_add(&mod->mod_list, &flv->mod_list);
                flv->state = FLV_STATE_PREV_SIZE;
                break;
            }
            default :
            {
                
            }
        }
    }

    rns_free(buf);
    return 0;

ERR_EXIT:

    rns_free(mod);
    rns_free(buf);
    rns_file_close(&fp);
    
    return -1;
}

int32_t flv_write_file(flv_t* flv, uchar_t* file)
{
    rns_file_t* fp = rns_file_open(file, RNS_FILE_OPEN | RNS_FILE_RW);
    if(fp == NULL)
    {
        return -1;
    }
    
    rns_strncpy(flv->file, file, sizeof(flv->file));
    
    uint32_t buf_size = 10240;
    uchar_t* buf = (uchar_t*)rns_malloc(buf_size);
    if(buf == NULL)
    {
        goto ERR_EXIT;
    }
    uchar_t* end = buf + buf_size;
    uint32_t size = 0;
    
    flv_mod_t* mod = NULL;
    uchar_t* walker = buf;
    uint32_t data_size = 0;
    uint32_t state_save = FLV_STATE_FILE_HEADER;

    uint32_t done = 0;
    
    while(!done)
    {
        switch (flv->state)
        {
            case FLV_STATE_BUF_NEED :
            {
                /* rns_file_seek_backward(file, end - walker); */
                size = rns_file_write(fp, buf, walker - buf);
                walker = buf;
                end = buf + size;

                flv->state = state_save;
                
                break;
            }
            case FLV_STATE_FILE_HEADER :
            {
                if(end - walker < 9)
                {
                    flv->state = FLV_STATE_BUF_NEED;
                    state_save = FLV_STATE_FILE_HEADER;
                    break;
                }
                
                walker += flv_write_header(&flv->header, walker, end);
                mod = rns_list_first(&flv->mod_list, flv_mod_t, mod_list);
                flv->state = FLV_STATE_PREV_SIZE;
                break;
            }
            case FLV_STATE_PREV_SIZE :
            {
                if(end - walker < 4)
                {
                    flv->state = FLV_STATE_BUF_NEED;
                    state_save = FLV_STATE_PREV_SIZE;
                    break;
                }

                
                rns_htob32(mod->prev_tag_size, walker);
                walker += 4;
                
                flv->state = FLV_STATE_TAG_HEADER;
                break;
            }
            case FLV_STATE_TAG_HEADER :
            {
                if(end - walker < 11)
                {
                    flv->state = FLV_STATE_BUF_NEED;
                    state_save = FLV_STATE_TAG_HEADER;
                    break;
                }
                
                walker += flv_write_tag_header(&mod->tag.tag_header, walker, end);
                data_size = mod->tag.tag_header.data_size;
                
                if(mod->tag.tag_header.tag_type == FLV_PACKET_TYPE_AUDIO)
                {
                    flv->state = FLV_STATE_AUDIO_HEADER;
                }
                else if(mod->tag.tag_header.tag_type == FLV_PACKET_TYPE_VIDEO)
                {
                    flv->state = FLV_STATE_VIDEO_HEADER;
                }
                else if(mod->tag.tag_header.tag_type == FLV_PACKET_TYPE_SCRIPT)
                {
                    flv->state = FLV_STATE_DATA_ATTR;
                }
                break;
            }
            case FLV_STATE_AUDIO_HEADER :
            {
                if(end - walker < 1)
                {
                    flv->state = FLV_STATE_BUF_NEED;
                    state_save = FLV_STATE_AUDIO_HEADER;
                    break;
                }

                *walker = (mod->tag.data_header.audio_header.sound_format << 4) | (mod->tag.data_header.audio_header.rate << 2) | (mod->tag.data_header.audio_header.sample_size << 1) | (mod->tag.data_header.audio_header.type);
                ++walker;
                --data_size;
                
                if(mod->tag.data_header.audio_header.sound_format == 10)
                {
                    flv->state = FLV_STATE_AUDIO_PACKET_TYPE;
                }
                else
                {
                    flv->state = FLV_STATE_DATA_ATTR;
                }
                break;
            }
            case FLV_STATE_AUDIO_PACKET_TYPE :
            {
                if(end - walker < 1)
                {
                    flv->state = FLV_STATE_BUF_NEED;
                    state_save = FLV_STATE_AUDIO_PACKET_TYPE;
                    break;
                }

                *walker = mod->tag.data_header.audio_header.packet_type;
                ++walker;
                --data_size;
                
                flv->state = FLV_STATE_DATA_ATTR;
                break;
            }
            case FLV_STATE_VIDEO_HEADER :
            {
                if(end - walker < 1)
                {
                    flv->state = FLV_STATE_BUF_NEED;
                    state_save = FLV_STATE_VIDEO_HEADER;
                    break;
                }
                *walker = (mod->tag.data_header.video_header.frame_type << 4) | mod->tag.data_header.video_header.codec_id;
                ++walker;
                --data_size;
                if(mod->tag.data_header.video_header.codec_id == 7)
                {
                    flv->state = FLV_STATE_VIDEO_PACKET_TYPE;
                }
                else
                {
                    flv->state = FLV_STATE_DATA_ATTR;
                }
                break;
            }
            case FLV_STATE_VIDEO_PACKET_TYPE :
            {
                if(end - walker < 4)
                {
                    flv->state = FLV_STATE_BUF_NEED;
                    state_save = FLV_STATE_VIDEO_PACKET_TYPE;
                    break;
                }

                *walker = mod->tag.data_header.video_header.packet_type;
                ++walker;
                
                rns_htob24(mod->tag.data_header.video_header.composition_time, walker);
                walker += 3;
                data_size -= 4;
                
                flv->state = FLV_STATE_DATA_ATTR;
                break;
            }
            case FLV_STATE_DATA_ATTR:
            {
                if(mod->tag.tag_header.filter)
                {
                    
                }
                else
                {
                    flv->state = FLV_STATE_DATA;
                }
            }
            case FLV_STATE_DATA :
            {
                if(end - walker < data_size)
                {
                    flv->state = FLV_STATE_BUF_NEED;
                    state_save = FLV_STATE_DATA;
                    break;
                }
                
                memcpy(walker, mod->tag.data, data_size);
                walker += data_size;
                
                flv->state = FLV_STATE_DONE;
                break;
            }
            case FLV_STATE_DONE :
            {
                mod = rns_list_next(&mod->mod_list, &flv->mod_list, flv_mod_t, mod_list);
                flv->state = FLV_STATE_PREV_SIZE;
                done = 1;
                break;
            }
            default :
            {
                done = 1;
            }
        }
    }

    rns_free(buf);
    rns_file_close(&fp);
    
    return 0;

ERR_EXIT:
    rns_free(mod);
    rns_free(buf);
    rns_file_close(&fp);
    
    return -1;
}

// tag header + metadata = tag
int32_t flv_write_metadata(flv_meta_t* meta, uchar_t* buf, uchar_t* end)
{
    uchar_t* walker = buf;
    *walker++ = 0x02;

    rns_htob16(10, walker);
    walker += 2;
    
    rns_strncpy(walker, "onMetaData", 10);
    walker += 10;

    *walker++ = 0x08;
    rns_htob32(1, walker);
    walker += 4;

    //---------------------------------------
    rns_htob16(8, walker);
    walker += 2;

    rns_strncpy(walker, "duration", 8);
    walker += 8;

    *walker++ = 0x00;

    rns_htobd(meta->duration, walker);
    walker += 8;
    //-----------------------------------------------
    
    *walker++ = 0x00;
    *walker++ = 0x00;
    *walker++ = 0x09;

    return walker - buf;
}
