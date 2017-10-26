#include "fts.h"


int file2file_copy(fts_input_t** input, uint32_t inputsize, fts_output_t** output, uint32_t outputsize)
{
    uint32_t i = 0;
    int32_t ret = 0;
    AVFormatContext *ifmt = NULL;
    AVPacket ipacket;
    
    
    AVFormatContext *ofmt = NULL;
    ret = avformat_alloc_output_context2(&ofmt, NULL, NULL, (char_t*)output->path);
    if(ofmt == NULL || ret < 0)
    {
        return -100;
    }
    
    ret = avio_open(&ofmt->pb, (char_t*)output->path, AVIO_FLAG_WRITE);
    if(ret < 0)
    {
        return -110;
    }
    
    ret = avformat_open_input(&ifmt, (char_t*)input->path, NULL, NULL);
    if(ret != 0)
    {
        return -10;
    }
    
    ret = avformat_find_stream_info(ifmt, NULL);
    if(ret < 0)
    {
        return -20;
    }
    
    for(i = 0; i < ifmt->nb_streams; i++)
    {
        AVStream* ostream = avformat_new_stream(ofmt, ifmt->streams[i]->codec->codec);
        if(ostream == NULL)
        {
            return -120;
        }
        
        ostream->start_time = 0;
        
        ret = avcodec_parameters_copy(ostream->codecpar, ifmt->streams[i]->codecpar);
        if(ret < 0)
        {
            return -130;
        }
    }
    
    ret = avformat_write_header(ofmt, NULL);
    if(ret < 0)
    {
        return -140;
    }
    
    i = 0;
    int32_t pts_base = 0;
    int32_t dts_base = 0;
    while(av_read_frame(ifmt, &ipacket) >= 0)
    {
        AVStream* istream, *ostream;
        istream = ifmt->streams[ipacket.stream_index];
        ostream = ofmt->streams[ipacket.stream_index];
        
        ipacket.pts = av_rescale_q_rnd(ipacket.pts, istream->time_base, ostream->time_base, (enum AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        ipacket.dts = av_rescale_q_rnd(ipacket.dts, istream->time_base, ostream->time_base, (enum AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        ipacket.duration = av_rescale_q(ipacket.duration, istream->time_base, ostream->time_base);
        ipacket.pos = -1;
        
        if(i == 0)
        {
            pts_base = ipacket.pts;
            dts_base = ipacket.dts;
            printf("base : %d - %d\n", pts_base, dts_base);
            ipacket.pts = 0;
            ipacket.dts = 0;
        }
        ++i;
        
        ret = av_interleaved_write_frame(ofmt, &ipacket);
        if(ret < 0)
        {
            return -150;
        }
        
        av_free_packet(&ipacket);
    }
    
    ret = av_write_trailer(ofmt);
    if(ret < 0)
    {
        
    }
    
    return 0;
}


int file2file_codec(fts_input_t** input, uint32_t inputsize, fts_output_t** output, uint32_t outputsize)
{
    uint32_t i = 0;
    uint32_t j = 0;
    int32_t ret = 0;
    int32_t videostream = -1;
    
    AVFormatContext *ifmt = NULL;
    AVCodecContext* icdctx = NULL;
    AVCodec *icd = NULL;
    AVPacket ipacket;
    AVFrame* iframe = NULL;


    
    AVFormatContext** ofmt = (AVFormatContext**)rns_calloc(outputsize, sizeof(AVFormatContext*));
    if(ofmt == NULL)
    {
        goto EXIT;
    }
    for(i = 0; i < outputsize; ++i)
    {
        ofmt[i] = (AVFormatContext*)rns_malloc(sizeof(AVFormatContext));
        if(ofmt[i] == NULL)
        {
            goto EXIT;
        }
    }
    AVCodecContext** ocdctx = (AVCodecContext**)rns_calloc(outputsize, sizeof(AVCodecContext*));
    if(ocdctx == NULL)
    {
        goto EXIT;
    }
    for(i = 0; i < outputsize; ++i)
    {
        ocdctx[i] = (AVCodecContext*)rns_malloc(sizeof(AVCodecContext));
        if(ocdctx[i] == NULL)
        {
            goto EXIT;
        }
    }
    AVCodec** ocd = (AVCodec**)rns_calloc(outputsize, sizeof(AVCodec*));
    if(ocd == NULL)
    {
        goto EXIT;
    }
    for(i = 0; i < outputsize; ++i)
    {
        ocd[i] = (AVCodec*)rns_malloc(sizeof(AVCodec));
        if(ocd[i] == NULL)
        {
            goto EXIT;
        }
    }
    
    AVFrame** oframe = (AVFrame**)rns_calloc(outputsize, sizeof(AVFrame*));
    if(oframe == NULL)
    {
        goto EXIT;
    }
    for(i = 0; i < outputsize; ++i)
    {
        oframe[i] = (AVFrame*)rns_malloc(sizeof(AVFrame));
        if(oframe[i] == NULL)
        {
            goto EXIT;
        }
    }
    AVPacket** opacket = (AVPacket**)rns_calloc(outputsize, sizeof(AVPacket*));
    if(opacket == NULL)
    {
        goto EXIT;
    }
    for(i = 0; i < outputsize; ++i)
    {
        opacket[i] = (AVPacket*)rns_malloc(sizeof(AVPacket));
        if(opacket[i] == NULL)
        {
            goto EXIT;
        }
    }
    

    struct SwsContext** imgctx = (struct SwsContext**)rns_calloc(outputsize, sizeof(struct SwsContext*));
    if(imgctx == NULL)
    {
        goto EXIT;
    }
    
    ret = avformat_open_input(&ifmt, (char_t*)input[0]->path, NULL, NULL);
    if(ret != 0)
    {
        return -10;
    }
    
    ret = avformat_find_stream_info(ifmt, NULL);
    if(ret < 0)
    {
        return -20;
    }

    icd = avcodec_find_decoder( ifmt->streams[videostream]->codecpar->codec_id);
    if(icd == NULL)
    {
        goto EXIT;
    }
    
    icdctx =  avcodec_alloc_context3(icd);
    if(icdctx == NULL)
    {
        goto EXIT;
    }
    ret = avcodec_parameters_to_context(icdctx, ifmt->streams[videostream]->codecpar);
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
        goto EXIT;
    }
    
    for(i = 0; i < outputsize; ++i)
    {
    
        ret = avformat_alloc_output_context2(&ofmt[i], NULL, NULL, (char_t*)output[i]->path);
        if(ofmt == NULL || ret < 0)
        {
            return -100;
        }
    
        ret = avio_open(&ofmt[i]->pb, (char_t*)output[i]->path, AVIO_FLAG_WRITE);
        if(ret < 0)
        {
            return -110;
        }
    
        enum AVCodecID ocdid = av_guess_codec(ofmt[i]->oformat, NULL, (char_t*)output[i]->path, NULL, AVMEDIA_TYPE_VIDEO);
        ocd[i] = avcodec_find_encoder(ocdid);
        if(ocd[i] == NULL)
        {
            goto EXIT;
        }
    
        for(j = 0; j < ifmt->nb_streams; j++)
        {
            if(ifmt->streams[j]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                videostream = j;
            } 
            AVStream* ostream = avformat_new_stream(ofmt[i], ocd[i]);
            if(ostream == NULL)
            {
                return -120;
            }
        
            ostream->start_time = 0;
        
            /* ret = avcodec_parameters_copy(ostream->codecpar, ifmt->streams[i]->codecpar); */
            /* if(ret < 0) */
            /* { */
            /*     return -130; */
            /* } */
            
        }
    
        ocdctx[i] = avcodec_alloc_context3(ocd[i]);
        ocdctx[i]->bit_rate = output[i]->bitrate;
        ocdctx[i]->pix_fmt = output[i]->pixfmt;
        ocdctx[i]->width = output[i]->width;
        ocdctx[i]->height = output[i]->height;
        ocdctx[i]->time_base.num = output[i]->num;
        ocdctx[i]->time_base.den = output[i]->den;
    
        ret = avcodec_open2(ocdctx[i], ocd[i], NULL);
        if(ret < 0)
        {
            goto EXIT;
        }

        imgctx[i] = sws_getContext(ifmt->streams[videostream]->codecpar->width,  ifmt->streams[videostream]->codecpar->height, (enum AVPixelFormat)ifmt->streams[videostream]->codecpar->format, output[i]->width, output[i]->height, output[i]->pixfmt, SWS_BICUBIC, NULL, NULL, NULL);
        if(imgctx == NULL)
        {
            goto EXIT;
        }

        uint32_t numBytes = av_image_get_buffer_size(output[i]->pixfmt, output[i]->width, output[i]->height, 1024);
        uchar_t* b = (uchar_t*)av_malloc(numBytes * sizeof(uchar_t));
        oframe[i] = av_frame_alloc();
        if(oframe[i] == NULL)
        {
            
            goto EXIT;
        }
        ret = av_image_fill_arrays(oframe[i]->data, oframe[i]->linesize, b,  output[i]->pixfmt, output[i]->width, output[i]->height, 1024);
        oframe[i]->format = output[i]->pixfmt;
        oframe[i]->width  = output[i]->width;
        oframe[i]->height = output[i]->height;

        ret = avformat_write_header(ofmt[i], NULL);
        if(ret < 0)
        {
            goto EXIT;
        }
    }
    
    while(av_read_frame(ifmt, &ipacket) >= 0)
    {
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


            for(i = 0; i < outputsize; ++i)
            {
            
                ret = sws_scale(imgctx[i], (const uint8_t * const*)iframe->data, iframe->linesize, 0,  icdctx->height, oframe[i]->data, oframe[i]->linesize);
                if(ret < 0)
                {
                    goto BREAK;
                }

                ret = avcodec_send_frame(ocdctx[i], oframe[i]);
                if(ret < 0)
                {
                    goto BREAK;
                }
                
                ret = avcodec_receive_packet(ocdctx[i], opacket[i]);
                if(ret < 0)
                {
                    goto BREAK;
                }
                
                ret = av_interleaved_write_frame(ofmt[i], opacket[i]);
                if(ret < 0)
                {
                    goto BREAK;
                }
            }
        }
        


    BREAK:
        av_free_packet(&ipacket);
        break;
    CONTINUE:
        av_free_packet(&ipacket);
        continue;
        
    }


    for(i = 0; i < outputsize; ++i)
    {
        ret = av_write_trailer(ofmt[i]);
        if(ret < 0)
        {
            continue;
        }
    }

EXIT:
    
    return 0;
}


