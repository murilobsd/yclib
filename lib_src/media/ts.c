#include "ts.h"
#include <stdlib.h>

uint32_t is_pat(uchar_t* ts_ptr)
{
    if(ts_ptr[0] != 0x47 || (ts_ptr[1] & 0x40) != 0x40)
    {
        return 0;
    }
    if(( (ts_ptr[1] & 0x1F) << 8 | ts_ptr[2] ) == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

uint32_t is_pid(uchar_t* ts_ptr, uint32_t pid)
{
    if(ts_ptr[0] != 0x47)
    {
        return 0;
    }
    if(((ts_ptr[1] & 0x1F) << 8 | ts_ptr[2]) == pid)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

uint32_t is_pcr(uchar_t* ts_ptr, uint32_t pid)
{

    if(!is_pid(ts_ptr, pid))
    {
        return 0;
    }
    if((ts_ptr[3] & 0x30) == 0x20 || (ts_ptr[3] & 0x30) == 0x30)
    {
        if((ts_ptr[5] & 0x10) != 0)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
	
    return 0;
}

uchar_t* find_pid(uchar_t* ts_ptr, uchar_t* ts_end, uint32_t pid)
{	
    uchar_t* walker = ts_ptr;
    for(walker = ts_ptr; walker < ts_end; walker += 188)
    {
        if(is_pid(walker, pid))
        {
            return walker;
        }
    }
    return NULL;
}
uchar_t* find_pat(uchar_t* ts_ptr, uchar_t* ts_end)
{
    uchar_t* walker = ts_ptr;
    for(walker = ts_ptr; walker < ts_end; walker += 188)
    {
        if(is_pat(walker))
        {
            return walker;
        }
    }
    return NULL;
	
}
uchar_t* find_pat_r(uchar_t* ts_ptr, uchar_t* ts_start)
{
    uchar_t* walker;
    for(walker = ts_ptr - 188; walker >= ts_start; walker -= 188)
    {
        if(is_pat(walker))
        {
            return walker;
        }
    }
    return NULL;
	
}

uchar_t* find_pcr(uchar_t* ts_ptr, uchar_t* ts_end, uint32_t pid)
{
    uchar_t* walker = ts_ptr;
    for(walker = ts_ptr; walker < ts_end; walker += 188)
    {
        if(is_pcr(walker, pid))
        {
            return walker;
        }
    }
    return NULL;
}
uchar_t* find_pcr_r(uchar_t* ts_ptr, uchar_t* ts_start, uint32_t pid)
{
    uchar_t* walker;
    for(walker = ts_ptr - 188; walker >= ts_start; walker -= 188)
    {
        if(is_pcr(walker, pid))
        {
            return walker;
        }
    }
    return NULL;
}

uint32_t get_pcr(uchar_t* ts_ptr)
{
    return ((ts_ptr[6] << 25) | (ts_ptr[7] << 17) | (ts_ptr[8] << 9) | (ts_ptr[9] << 1) | (ts_ptr[10] >> 7));
}

uint32_t parse_ts(uchar_t* ts_ptr, ts_header_t* ts_header)
{
    uchar_t* walker = ts_ptr;
	
    ts_header->sync_byte = walker[0];
    ts_header->transport_error_indicator = walker[1] >> 7;
    ts_header->payload_unit_start_indicator = ((walker[1] & 0x40) >> 6);
    ts_header->transport_priority = ((walker[1] & 0x20) >> 5);
    ts_header->pid = ((walker[1] & 0x1F) << 8 | walker[2] );
    ts_header->transport_scrambling_control = walker[3] >> 6;
    ts_header->adaptation_field_control = walker[3] & 0x30 >> 4;
    ts_header->continuity_counter = walker[3] & 0x0F;

    return 0;
}

ts_pat_t* parse_pat2(uchar_t* pat_ptr)
{
    return parse_pat(pat_ptr + 5);
}

ts_pat_t* parse_pat(uchar_t* pat_ptr)
{
    ts_pat_t* pat = (ts_pat_t*)rns_malloc(sizeof(ts_pat_t));
    if(pat == NULL)
    {
        return NULL;
    }
    
    pat->table_id = pat_ptr[0];
    pat->section_length = pat_ptr[1] & 0x0F << 8 | pat_ptr[2];
    uint32_t program_num = (pat->section_length - 9) / 4;
    uchar_t* walker = pat_ptr + 8;
    uint32_t i = 0;

	
    INIT_LIST_HEAD(&pat->program_head);
	
    for(i = 0; i < program_num; ++i, walker += 4)
    {
        ts_pat_program_t* p = (ts_pat_program_t*)rns_malloc(sizeof(ts_pat_program_t));
        if(p == NULL)
        {
            continue;
        }
        p->program_number = (walker[0] << 8) | walker[1];
        p->program_map_pid = ((walker[2] & 0x1F) << 8) | walker[3];
        rns_list_add(&p->list,&pat->program_head);
    }
    return pat;
}

void pat_destroy(ts_pat_t** pat)
{
    ts_pat_program_t* program = NULL;
    while((program = rns_list_first(&(*pat)->program_head, ts_pat_program_t, list)) != NULL)
    {
        rns_list_del(&program->list);
        rns_free(program);
    }

    rns_free(*pat);
    *pat = NULL;
    return;
}

ts_pmt_t* parse_pmt2(uchar_t* pmt_ptr)
{
    return parse_pmt(pmt_ptr + 5);
}

ts_pmt_t* parse_pmt(uchar_t* pmt_ptr)
{
    ts_pmt_t* pmt = (ts_pmt_t*)rns_malloc(sizeof(ts_pmt_t));
    if(pmt == NULL)
    {
        return NULL;
    }
    pmt->table_id = pmt_ptr[0];
    pmt->section_length = ((pmt_ptr[1] & 0x0F) << 8) | pmt_ptr[2];
    pmt->pcr_pid = ((pmt_ptr[8] & 0x1F) << 8) | pmt_ptr[9];
    uchar_t* walker = pmt_ptr + 12;
    uchar_t* end = pmt_ptr + 3 + pmt->section_length - 4;
    INIT_LIST_HEAD(&pmt->ts_pmt_stream_head);
	
    for(; walker < end; walker += 5)
    {
        ts_pmt_stream_t* p = (ts_pmt_stream_t*)rns_malloc(sizeof(ts_pmt_stream_t));
        if(p == NULL)
        {
            continue;
        }
        p->stream_type = walker[0] ;
        p->stream_pid = ((walker[1] & 0x1F) << 8) | walker[2];
        rns_list_add(&p->list,&pmt->ts_pmt_stream_head);
        //跳过描述信息
        walker += (walker[3] & 0x0F << 8) | walker[4];
    }

    return pmt;
}

void pmt_destroy(ts_pmt_t** pmt)
{
    ts_pmt_stream_t* stream = NULL;
    while((stream = rns_list_first(&(*pmt)->ts_pmt_stream_head, ts_pmt_stream_t, list)) != NULL)
    {
        rns_list_del(&stream->list);
        rns_free(stream);
    }

    rns_free(*pmt);
    *pmt = NULL;
    return;
}

uint32_t parse_pes(uchar_t* walker, ts_pes_header_t* pes_header)
{
    return 0;
}

uint32_t is_idr(uchar_t* ts_ptr)
{
    if((ts_ptr[1] & 0x40) == 0)
    {
        return 0;
    }
    
    uint32_t count = 0;
    while(count<184)
    {
        if( *(ts_ptr + count) == 0x00 && *(ts_ptr + count + 1) == 0x00 && *(ts_ptr + count + 2) == 0x01 && (*(ts_ptr + count + 3) & 0x1F) == 0x05)
        {
            break;
        }
        ++count;
    }
    return count < 184 ? 1 : 0;
}

uchar_t* find_first_pmt(uchar_t* ts_ptr, uchar_t* ts_end)
{
    uchar_t* walker = ts_ptr;
    uchar_t* pat_offset = find_pat(walker, ts_end);
    if(pat_offset == NULL)
    {
        return NULL;
    }
    ts_pat_t* pat_ptr = parse_pat(pat_offset + 5);
	
    uint32_t pmt_pid = 0;
    list_head_t* p;
    ts_pat_program_t* program;


    rns_list_for_each(p,&pat_ptr->program_head)
    {

        program = rns_list_entry(p, ts_pat_program_t, list);
        if(program->program_number == 1)
        {
            pmt_pid = program->program_map_pid;
        }

    }

    walker += 188;
	
    uchar_t* pmt_offset;
    pmt_offset = find_pid(walker, ts_end, pmt_pid);
    if(pmt_offset == NULL)
    {
        pat_destroy(&pat_ptr);
        return NULL;
    }
    pat_destroy(&pat_ptr);
    return pmt_offset;
}

//find the first pmt. improve later;
uchar_t* find_idr(uchar_t* ts_ptr, uchar_t* ts_end, uint32_t pid)
{	
    uchar_t* walker = ts_ptr;
    while(walker < ts_end)
    {
        if(is_pid(walker, pid))
        {
            if(is_idr(walker))
            {
                return walker;
            }
        }
        walker += 188;
    }
	
    return NULL;
}

uint32_t find_pcr_pid(uchar_t* ts_ptr, uchar_t* ts_end)
{
    uchar_t* pmt_offset = find_first_pmt(ts_ptr, ts_end);
    if(pmt_offset == NULL)
    {
        return 0;
    }
    ts_pmt_t* pmt_ptr =  parse_pmt(pmt_offset + 5);
    uint32_t pid = pmt_ptr->pcr_pid;
    pmt_destroy(&pmt_ptr);
    
    return pid;
	
	
}

uchar_t* find_ts_by_pcr(uchar_t* ts_ptr, uchar_t* ts_end, uint32_t tick)
{
    uint32_t pcr_start = 0;
    uint32_t pcr_end = 0;
    uint32_t pcr_middle = 0;
	
    uchar_t* pcr_start_offset;
    uchar_t* pcr_end_offset;
	
    uchar_t* ptr = NULL;
	
    pcr_start_offset = ts_ptr;
    pcr_end_offset = ts_end;

    uint32_t pid = find_pcr_pid(ts_ptr, ts_end);
	
    while(pcr_start_offset < pcr_end_offset)
    {
        pcr_start_offset = find_pcr(pcr_start_offset, ts_end, pid);
        pcr_start = get_pcr(pcr_start_offset);
        pcr_end_offset = find_pcr_r(pcr_end_offset, ts_end, pid);
        pcr_end= get_pcr(pcr_end_offset);
		
        if(tick < (pcr_start + pcr_end) / 2)
        {
            pcr_end_offset = pcr_start_offset + (pcr_end_offset - pcr_start_offset) / 2;
        }
        else
        {
            pcr_start_offset = pcr_start_offset + (pcr_end_offset - pcr_start_offset) / 2;
        }
    }
	
    return pcr_start_offset;
}

uint32_t find_duration_by_offset(uchar_t* ts_ptr, uchar_t* ts_end, uchar_t* start_offset, uchar_t* end_offset)
{
    uint32_t pid = find_pcr_pid(ts_ptr, ts_end);
    uchar_t* pcr_start_offset = find_pcr(start_offset, ts_end, pid);
    uchar_t* pcr_end_offset = find_pcr_r(end_offset, ts_ptr, pid);	
	
    uint32_t pcr_start = get_pcr(pcr_start_offset);
    uint32_t pcr_end = get_pcr(pcr_end_offset);
	
    return pcr_end - pcr_start;
}
