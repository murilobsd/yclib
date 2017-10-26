#include "hls.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

uchar_t* get_line(uchar_t* buf, uchar_t* buf_end, uchar_t* t_buf)
{
    uchar_t* walker = buf;
    uchar_t* line_start = walker;
    uchar_t* line_end = walker;
    while((*walker == '\n' || *walker == ' ') && walker != buf_end)++walker;
    line_start = walker;
    while(*walker != '\n' && walker != buf_end)++walker;
    line_end = walker;
    if(line_start < line_end)
    {
        rns_strncpy(t_buf, line_start, line_end - line_start);
        return line_end;
    }

    return NULL;
}

uchar_t* line_substr(uchar_t* t_buf, uchar_t* substr)
{
    uchar_t* start = rns_strstr(t_buf, substr);
    if(start == NULL)
    {
        return NULL;
    }
    return start + rns_strlen(substr);
}

void m3u8_destroy(media_m3u8_t** m3u8)
{
    if(*m3u8 == NULL)
    {
        return;
    }

    rns_free((*m3u8)->seg);
    rns_free(*m3u8);
    *m3u8 = NULL;
    return;
}


media_m3u8_t* m3u8_read(uchar_t* buf, uint32_t size)
{
    media_m3u8_t* m3u8 = (media_m3u8_t*)rns_malloc(sizeof(media_m3u8_t));
    if(m3u8 == NULL)
    {
        return NULL;
    }
    
    uchar_t* walker = buf;
    uchar_t* end = buf + size;
    uchar_t* value = NULL;
    uint32_t count = 0;
    
    m3u8->seg_size = 0;
	
	
    uchar_t t_buf[256];

    while(walker < end)
    {
        walker = rns_strstr(walker, "#EXTINF:");
        if(walker != NULL)
        {
            ++m3u8->seg_size;
            walker += rns_strlen("#EXTINF:");
        }
        else
        {
            break;
        }
    }
    
    m3u8->seg = (media_seg_t*)rns_calloc(m3u8->seg_size, sizeof(media_seg_t));
    if(m3u8->seg == NULL)
    {
        rns_free(m3u8);
        return NULL;
    }
    
    walker = buf;
    while(walker < end)
    {
        rns_memset(t_buf, 256);
        walker = get_line(walker, end, t_buf);
        if(walker == NULL)
        {
            break;
        }
        value = line_substr(t_buf, (uchar_t*)"#EXTINF:");
        if(value != NULL)
        {
            m3u8->seg[count].time_len = atof((char_t*)value);
            rns_memset(t_buf, 256);
            walker = get_line(walker, end, t_buf);
            
            rns_strncpy(m3u8->seg[count].uri, t_buf, rns_strlen(t_buf));
            ++count;
            continue;
        }
		
        value = line_substr(t_buf, (uchar_t*)"#EXT-X-TARGETDURATION:");
        if(value != NULL)
        {
            m3u8->max_duration = atol((char_t*)value);
            continue;
        }
		
        value = line_substr(t_buf, (uchar_t*)"#EXT-X-DISCONTINUITY:");
        if(value != NULL)
        {
            m3u8->discontinuity = 1;
            continue;
        }
		
        value = line_substr(t_buf, (uchar_t*)"#EXT-X-VERSION:");
        if(value != NULL)
        {
            m3u8->version = atol((char_t*)value);
            continue;
        }
        value = line_substr(t_buf, (uchar_t*)"#EXT-X-MEDIA-SEQUENCE:");
        if(value != NULL)
        {
            m3u8->media_sequence = atol((char_t*)value);
            continue;
        }
        value = line_substr(t_buf, (uchar_t*)"#EXT-X-ENDLIST");
        if(value != NULL)
        {
            m3u8->end = 1;
            continue;
        }
    }
	
    return m3u8;
}

uint32_t m3u8_write(media_m3u8_t* m3u8, uchar_t* buf, uint32_t size)
{
    int32_t count = 0;
    uint32_t i = 0;
    memset(buf, 0, size);
	
    snprintf((char_t*)buf + count, size - count - 1, "#EXTM3U\n");
    count = rns_strlen(buf);
	
    snprintf((char_t*)buf + count, size - count - 1, "#EXT-X-VERSION:%u\n", M3U8_VERSION);
    count = rns_strlen(buf);
	
    snprintf((char_t*)buf + count, size - count - 1, "#EXT-X-MEDIA-SEQUENCE:%u\n", m3u8->media_sequence);
    count = rns_strlen(buf);
	
    snprintf((char_t*)buf + count, size - count - 1, "#EXT-X-TARGETDURATION:%f\n", m3u8->max_duration);
    count = rns_strlen(buf);
	
    for(i = 0; i < m3u8->seg_size; ++i)
    {
        snprintf((char_t*)buf + count, size - count - 1, "#EXTINF:%f,\n", m3u8->seg[i].time_len);
        count = rns_strlen(buf);
        snprintf((char_t*)buf + count, size - count - 1, "%s\n", m3u8->seg[i].uri);
        count = rns_strlen(buf);
    }

    if(m3u8->end)
    {
        snprintf((char_t*)buf + count, size - count - 1, "#EXT-X-ENDLIST\n");
    }
    
    return rns_strlen(buf);
}
