#include "fts.h"

int32_t fts_videoinfo(uchar_t* input, uint64_t* duration, uint64_t* size)
{
    int32_t retcode = 0;
    uint32_t i = 0;
    int32_t videoStream = -1;
    AVFormatContext *ifmt = NULL;
    
    *size = rns_file_size2(input);
    /* uchar_t* buffer = NULL; */
    
    /* uint32_t s = *size; */
    
    /* int32_t ret = av_file_map((char_t*)input, &buffer, (size_t*)&s, 0, NULL); */
    /* if (ret < 0) */
    /* { */
    /*     retcode = -2; */
    /*     goto EXIT; */
    /* } */
    
    /* rns_data_t* rdata = (rns_data_t*)rns_malloc(sizeof(rns_data_t)); */
    /* if(rdata == NULL) */
    /* { */
    /*     retcode = -1; */
    /*     goto EXIT; */
    /* } */
    
    /* rdata->data  = buffer; */
    /* rdata->size = *size; */
    /* rdata->offset = 0; */
    
    /* ifmt = avformat_alloc_context(); */
    /* uchar_t * ibuffer =(uchar_t*)av_malloc(32768); */
    /* AVIOContext *iavio = avio_alloc_context(ibuffer, 32768, 0, rdata, readcb, NULL, NULL); */
    /* ifmt->pb = iavio; */
    
    AVDictionary * dict = NULL;
    /* int32_t ret = av_dict_set_int(&dict, "probesize", s, 0); */
    /* if(ret < 0) */
    /* { */
    /*     retcode = -2; */
    /*     goto EXIT; */
    /* } */
    
    /* ret = av_dict_set_int(&dict, "analyzeduration", 5428661000, 0); */
    /* if(ret < 0) */
    /* { */
    /*     retcode = -2; */
    /*     goto EXIT; */
    /* } */
    
    int32_t ret = avformat_open_input(&ifmt, (char_t*)input, NULL, &dict);
    if(ret != 0)
    {
        retcode = -1;
        goto EXIT;
    }
    
    
    ret = avformat_find_stream_info(ifmt, NULL);
    if(ret < 0)
    {
        retcode = -2;
        goto EXIT;
    }
    
    
    for(i = 0; i < ifmt->nb_streams; i++)
    {
        if(ifmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStream = i;
            break;
        } 
    }
    if(videoStream == -1)
    {
        retcode = -3;
        goto EXIT;
    }
    
    *duration = ifmt->duration / 1000;
    
EXIT:
    return retcode;
}
