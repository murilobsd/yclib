/* #include "yms_vod.h" */

/* #include <stdio.h> */
/* #include <string.h> */

/* static void gop_write(gop_t* gop, uchar_t* buf, uint32_t size) */
/* { */
/*     rns_memcpy(gop->buf + gop->info.len, buf, size); */
/*     gop->info.len += size; */
/* } */

/* static uchar_t* gop_alloc(resource_t* resources, gop_t* gop) */
/* { */
/*     gop->buf = rns_mpool_alloc(resources->vod->block_mp); */
    
/*     if(gop->buf == NULL) */
/*     { */
/*         list_head_t* p; */
/*         rns_list_for_each(p, &resources->gop_list) */
/*         { */
/*             gop_t* q = rns_list_entry(p, gop_t, list); */
/*             if(q->buf != NULL) */
/*             { */
/*                 gop->buf = q->buf; */
/*                 q->buf = NULL; */
/*             } */
/*         } */
/*     } */
/*     return gop->buf; */
/* } */

/* /\** */
/* in: */
/* @param pos:ts file position */

/* out: */
/* @param ts_size:ts file size */
/* @param ts_buffer_size:ts buffer size */
/* @param ts_ptr: ts buffer pointer */
/* **\/ */
/* static int32_t yms_get_ts(rns_hconn_t *hconn, int64_t pos, int64_t *ts_size, int32_t *ts_buffer_size, uchar_t **ts_ptr) */
/* {  */
/*     //\*ts_ptr = (uchar_t*)malloc(TS_READER_BUFFER_SIZE); */
/*     *ts_ptr = rns_malloc(TS_READER_BUFFER_SIZE); */
/*     //FILE *file = fopen("cctv1_hd-1_193000000000.ts","rb+"); */
/*     FILE *file = fopen("I_frame.ts","rb+"); */
/*     if (NULL == file) */
/*     { */
/*         printf("acquire ts file failed!\n"); */
/*         return -1; */
/*     } */

/*     int32_t ret = fseek(file, 0, SEEK_END); */
/*     if (0 != ret) */
/*     { */
/*         printf("seek location filed!\n"); */
/*         return -1; */
/*     } */

/*     *ts_size = ftell(file) + 1; */

/*     pos = pos*TS_READER_BUFFER_SIZE; */

/*     if (pos < *ts_size) */
/*     { */
/*         ret = fseek(file, pos, SEEK_SET); */
/*         if (0 != ret) */
/*         { */
/*             printf("seek location filed!\n"); */
/*             return -1; */
/*         } */
/*     } */
/*     else */
/*     { */
/*         printf("position beyond!\n"); */
/*         fclose(file); */

/*         return 0;  */
/*     } */

/*     if ((pos+1)*TS_READER_BUFFER_SIZE < *ts_size) */
/*     { */
/*         fread(*ts_ptr, 1, TS_READER_BUFFER_SIZE, file); */
/*         *ts_buffer_size = TS_READER_BUFFER_SIZE; */
/*     } */
/*     else */
/*     { */
/*         fread(*ts_ptr, 1, *ts_size - pos*TS_READER_BUFFER_SIZE + 1, file); */
/*         *ts_buffer_size = *ts_size - pos*TS_READER_BUFFER_SIZE + 1; */
/*     }     */
/* } */

/* static int32_t yms_vod_gop(void *data, int32_t dataSize, resource_t *resource) */
/* { */
/*     if (NULL == data) */
/*     { */
/*         printf("empty data!\n"); */
/*         return -1; */
/*     } */

/*     uchar_t *ts_ptr = (uchar_t*)data; */
/*     gop_t* gop      = rns_list_last(&resource->gop_list, gop_t, list); */

/*     int32_t count = dataSize/TS_PACKET_LENG; */

/*     for (int32_t i = 0; i < count; ++i) */
/*     { */
/*         if (is_pat(ts_ptr)) */
/*         { */
/*             rns_memcpy(resource->pat, ts_ptr, 188); */
/*             ts_pat_t pat; */
/*             parse_pat2(resource->pat, &pat); */
/*             list_head_t* p; */
/*             rns_list_for_each(p,&pat.program_head) */
/*             { */
/*                 ts_pat_program_t* program = rns_list_entry(p, ts_pat_program_t, list); */
/*                 if(program->program_number == 1) */
/*                 { */
/*                     resource->pmt_pid = program->program_map_pid; */
/*                 } */
/*             } */

/*             continue;            */
/*         } */

/*         if(is_pid(ts_ptr, resource->pmt_pid)) */
/*         { */
/*             rns_memcpy(resource->pmt, ts_ptr, 188); */
/*             ts_pmt_t pmt; */
/*             parse_pmt2(ts_ptr, &pmt); */
/*             resource->pcr_pid =  pmt.pcr_pid; */

/*             continue; */
/*         } */

/*         if(is_idr(ts_ptr) || gop == NULL) */
/*         { */
/*             // gop_t* rgop = rns_list_last(&resource->gop_list, gop_t, list); */
/*             // if(rgop != NULL) */
/*             // { */
/*             //     rns_http_save_buffer(hconn); */
/*             // } */
            
/*             gop = (gop_t*)rns_malloc(sizeof(gop_t)); */
/*             if(gop_alloc(resource, gop) == NULL) */
/*             { */
/*                 //goto EXIT; */
/*                 return -1; */
/*             } */
/*             gop->info.time_len = 0; */
/*             gop->info.time_start = time(0); */
            
/*             gop_write(gop, resource->pat, 188); */
/*             gop_write(gop, resource->pmt, 188); */

/*             // ++resource->gop_idx; */
/*             // gop->id = resource->gop_idx; */
/*             rns_list_add(&gop->list, &resource->gop_list); */
/*         } */

/*         gop_write(gop, ts_ptr, 188); */
/*         ts_ptr += 188;         */
/*     } */
/* } */

/* int32_t main() */
/* { */
/*     /\* code *\/ */
/*     printf("yms begin!\n"); */

/*     rns_hconn_t *hconn; */
/*     resource_t  resource; */
/*     memset(&resource, 0, sizeof(resource_t)); */

/*     int64_t pos         = 0; */
/*     int64_t ts_size     = 0; */
/*     int32_t buffer_size = 0; */
/*     uchar_t *ts_ptr     = NULL; */

/*     yms_get_ts(hconn, pos, &ts_size, &buffer_size, &ts_ptr); */

/*     printf("size = %d\n", buffer_size); */

/*     yms_vod_gop((void*)ts_ptr, buffer_size, &resource); */

/*     return 0; */
/* } */

