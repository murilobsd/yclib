#include "fts.h"

int32_t readcb(void* data, uchar_t* buf, int32_t size)
{
    rns_data_t *rdata = (rns_data_t*)data;
    
    uint32_t s = (uint32_t)size < rdata->size - rdata->offset ? (uint32_t)size : rdata->size - rdata->offset;
    
    rns_memcpy(buf, (uchar_t*)rdata->data + rdata->offset, s);
    rdata->offset += s;
    
    return (int32_t)s;
}  




list_head_t* fts_file2img(uchar_t* input, uchar_t* outtype, uint32_t start, uint32_t interval, uint32_t number)
{
    
    uint32_t i = 0;
    int32_t videoStream = -1;
    AVFormatContext *ifmt = NULL;
    AVCodecContext *icdctx = NULL;
    AVCodec *icd = NULL;
    AVFrame *iframe = NULL;
    AVFrame* oframe = NULL;
    AVPacket ipacket;
    int32_t ret = 0;
    AVCodecContext* ocdctx = NULL;
    AVCodec* ocd = NULL;
    AVPacket opacket;
    struct SwsContext *img_convert_ctx = NULL;
    
    /* uint32_t size = rns_file_size2(input); */
    /* uchar_t* buffer = NULL; */
    
    list_head_t* head = (list_head_t*)rns_malloc(sizeof(list_head_t));
    if(head == NULL)
    {
        goto EXIT;
    }
    INIT_LIST_HEAD(head);
    
    /* rns_data_t* rdata = (rns_data_t*)rns_malloc(sizeof(rns_data_t)); */
    /* if(rdata == NULL) */
    /* { */
    /*     goto EXIT; */
    /* } */
    
    /* ret = av_file_map((char_t*)input, &buffer, (size_t*)&size, 0, NULL); */
    /* if (ret < 0) */
    /* { */
    /*     goto EXIT; */
    /* } */
    
    /* rdata->data  = buffer; */
    /* rdata->size = size; */
    /* rdata->offset = 0; */
    
    /* ifmt = avformat_alloc_context(); */
    /* uchar_t * ibuffer=(uchar_t*)av_malloc(5000000); */
    /* AVIOContext *iavio = avio_alloc_context(ibuffer, 32768, 0, rdata, readcb, NULL, NULL); */
    /* ifmt->pb = iavio; */
    
    
    /* AVInputFormat* ipfmt = av_find_input_format("mp4"); */
    /* if(ipfmt == NULL) */
    /* { */
    /*     goto EXIT; */
    /* } */
    
    AVDictionary * dict = NULL;
    /* int32_t ret = av_dict_set_int(&dict, "probesize", size, 0); */
    /* if(ret < 0) */
    /* { */
    /*     goto EXIT; */
    /* } */
    
    ret = avformat_open_input(&ifmt, (char_t*)input, NULL, &dict);
    if(ret != 0)
    {
        goto EXIT;
    }
    
    ret = avformat_find_stream_info(ifmt, NULL);
    if(ret < 0)
    {
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
        goto EXIT;
    }
    
    icd = avcodec_find_decoder( ifmt->streams[videoStream]->codecpar->codec_id);
    if(icd == NULL)
    {
        goto EXIT;
    }
    
    icdctx =  avcodec_alloc_context3(icd);
    if(icdctx == NULL)
    {
        goto EXIT;
    }
    ret = avcodec_parameters_to_context(icdctx, ifmt->streams[videoStream]->codecpar);
    if(ret < 0)
    {
        goto EXIT;
    }
    ret = avcodec_open2(icdctx, icd, NULL);
    if(ret < 0)
    {
        goto EXIT;
    }
    
    iframe = av_frame_alloc();
    if(iframe == NULL)
    {
        goto BREAK;
    }
    
    
    enum AVPixelFormat pixFormat;
    switch (ifmt->streams[videoStream]->codecpar->format)
    {
        case AV_PIX_FMT_YUVJ420P :
        {
            pixFormat = AV_PIX_FMT_YUV420P;
            break;
        }
        case AV_PIX_FMT_YUVJ422P  :
        {
            pixFormat = AV_PIX_FMT_YUV422P;
            break;
        }
        case AV_PIX_FMT_YUVJ444P   :
        {
            pixFormat = AV_PIX_FMT_YUV444P;
            break;
        }
        case AV_PIX_FMT_YUVJ440P :
        {
            pixFormat = AV_PIX_FMT_YUV440P;
            break;
        }
        default:
        {
            pixFormat = (enum AVPixelFormat)ifmt->streams[videoStream]->codecpar->format;
            break;
        }
    }
    
    if(pixFormat == AV_PIX_FMT_NONE)
    {
        goto EXIT;
    }
    
    uint32_t numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, ifmt->streams[videoStream]->codecpar->width, ifmt->streams[videoStream]->codecpar->height, 1024);
    uchar_t* b = (uchar_t*)av_malloc(numBytes * sizeof(uchar_t));
    oframe = av_frame_alloc();
    if(oframe == NULL)
    {
        
        goto EXIT;
    }
    ret = av_image_fill_arrays(oframe->data, oframe->linesize, b,  AV_PIX_FMT_YUV420P, icdctx->width, icdctx->height, 1024);
    oframe->format = AV_PIX_FMT_YUVJ420P;
    oframe->width  = icdctx->width;
    oframe->height = icdctx->height;
    
    img_convert_ctx = sws_getContext(ifmt->streams[videoStream]->codecpar->width,  ifmt->streams[videoStream]->codecpar->height, pixFormat, ifmt->streams[videoStream]->codecpar->width, ifmt->streams[videoStream]->codecpar->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    if(img_convert_ctx == NULL)
    {
        goto EXIT;
    }
    
    ocd = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if(ocd == NULL)
    {
        goto EXIT;
    }
    
    ocdctx = avcodec_alloc_context3(ocd);
    ocdctx->bit_rate = 400000;
    ocdctx->pix_fmt = AV_PIX_FMT_YUVJ420P;
    ocdctx->width = icdctx->width;
    ocdctx->height = icdctx->height;
    ocdctx->time_base.num = 1;
    ocdctx->time_base.den = 25;
    
    ret = avcodec_open2(ocdctx, ocd, NULL);
    if(ret < 0)
    {
        goto EXIT;
    }
    
    uint32_t count = 0;
    int64_t timestamp = start;
    
    rns_file_t* fp = NULL;
    
    while(av_read_frame(ifmt, &ipacket) >= 0)
    {
        if(count >= number)
        {
            goto BREAK;
        }
        
        if(ipacket.pts * ifmt->streams[videoStream]->time_base.num / ifmt->streams[videoStream]->time_base.den < timestamp || (ipacket.flags & 0x01) == 0 || ipacket.stream_index != videoStream)
        {
            goto CONTINUE;
        }
        
        while(true)
        {
            
            ret = avcodec_send_packet(icdctx, &ipacket);
            if(ret < 0)
            {
                goto BREAK;
            }
            
            ret = avcodec_receive_frame(icdctx, iframe);
            if(ret < 0 && ret != AVERROR(EAGAIN))
            {
                goto BREAK;
            }
            if(ret < 0)
            {
                goto CONTINUE;
            }
            
            ret = sws_scale(img_convert_ctx, (const uint8_t * const*)iframe->data, iframe->linesize, 0,  icdctx->height, oframe->data, oframe->linesize);
            if(ret < 0)
            {
                goto BREAK;
            }
            
            ret = avcodec_send_frame(ocdctx, oframe);
            if(ret < 0)
            {
                goto BREAK;
            }
            av_init_packet(&opacket);
            opacket.data = NULL;
            opacket.size = 0;
            
            ret = avcodec_receive_packet(ocdctx, &opacket);
            if(ret < 0)
            {
                goto BREAK;
            }
            
            rns_list_t* rlist = (rns_list_t*)rns_malloc(sizeof(rns_list_t));
            if(rlist == NULL)
            {
                goto BREAK;
            }
            
            rlist->data = rns_malloc(rns_strlen(input) + 10 + rns_strlen(outtype) + 2);
            if(rlist->data == NULL)
            {
                goto BREAK;
            }
            snprintf((char_t*)rlist->data, rns_strlen(input) + rns_strlen(outtype) + 12, "%s-%d.%s", input, ipacket.pts * ifmt->streams[videoStream]->time_base.num / ifmt->streams[videoStream]->time_base.den , outtype);
            
            fp = rns_file_open(rlist->data, RNS_FILE_CREAT | RNS_FILE_RW);
            if(fp == NULL)
            {
                goto BREAK;
            }
            
            rns_list_add(&rlist->list, head);
            
            ret = rns_file_write(fp, opacket.data, opacket.size);
            if(ret != opacket.size)
            {
                LOG_ERROR(lp, "");
                goto BREAK;
            }
            
            timestamp = ipacket.pts * ifmt->streams[videoStream]->time_base.num / ifmt->streams[videoStream]->time_base.den + interval;
            ++count;
            
            ret = av_seek_frame(ifmt, videoStream, timestamp * (ifmt->streams[videoStream]->time_base.den / ifmt->streams[videoStream]->time_base.num), 0);
            if(ret < 0)
            {
                goto BREAK;
            }
            
            while(true)
            {
                ret = avcodec_receive_frame(icdctx, iframe);
                if(ret == AVERROR(EAGAIN))
                {
                    break;
                }
            }
            
            while(true)
            {
                ret = avcodec_receive_packet(ocdctx, &opacket);
                if(ret ==  AVERROR(EAGAIN))
                {
                    break;
                }
            }
            av_packet_unref(&opacket);
            
            break;
        }
        
        
    CONTINUE:
        av_packet_unref(&ipacket);
        
        rns_file_close(&fp);
        continue;
        
    BREAK:
        av_packet_unref(&ipacket);
        
        rns_file_close(&fp);
        break;
    }
    
    
EXIT :
    
    sws_freeContext(img_convert_ctx);
    avformat_close_input(&ifmt);
    av_frame_free(&iframe);
    av_free(b);
    ret = avcodec_close(ocdctx);
    if(ret < 0)
    {
        
    }
    avcodec_free_context(&ocdctx);
    ret = avcodec_close(icdctx);
    if(ret < 0)
    {
        
    }
    avcodec_free_context(&icdctx);
    /* av_frame_free(&iframe); */
    
    return head;
}
