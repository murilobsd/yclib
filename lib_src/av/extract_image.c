

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "av.h"

int extract_image(uchar_t* src, uchar_t* dst)
{
    av_register_all();

    int i;
    int videoStream = -1;
    AVFormatContext *pFormatCtx = NULL;
    AVCodecContext *pCodecCtx = NULL;
    AVCodec *pCodec = NULL;
    AVFrame *pFrame = NULL;
    AVFrame* pFrameYUV = NULL;
    AVPacket packet;
    
    AVCodecContext* eCodecCtx = NULL;
    AVCodec* eCodec = NULL;
    AVPacket ePacket;  
    int efd = open((char*)dst, O_CREAT | O_TRUNC | O_RDWR, 0600);
    if(efd < 0)
    {
        return -1;
    }
    
    int ret = avformat_open_input(&pFormatCtx, (char*)src, NULL, NULL);
    if(ret != 0)
    {
        return -10;
    }
    
    ret = avformat_find_stream_info(pFormatCtx, NULL);
    if(ret < 0)
    {
        return -20;
    }
    
    
    for(i = 0; i < pFormatCtx->nb_streams; i++)
    {
        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStream = i;
            break;
        } 
    }
    if(videoStream==-1)
    {
        return -30; 
    }
    
    pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec == NULL)
    {
        return -40; 
    }
    ret = avcodec_open2(pCodecCtx, pCodec, NULL);
    if(ret < 0)
    {
        return -50;
    }
    
    pFrame = av_frame_alloc();
    int numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
    uchar_t* buffer= (uchar_t*)av_malloc(numBytes * sizeof(uchar_t));
    pFrameYUV = av_frame_alloc();
    if(pFrameYUV == NULL)
    {
        av_frame_free(&pFrame);
        av_free(buffer);
        return -60;
    }
    avpicture_fill((AVPicture*)pFrameYUV, buffer,  AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
    
    int frameFinished;

    enum AVPixelFormat pixFormat;
    switch (pCodecCtx->pix_fmt) {
        case AV_PIX_FMT_YUVJ420P :
            pixFormat = AV_PIX_FMT_YUV420P;
            break;
        case AV_PIX_FMT_YUVJ422P  :
            pixFormat = AV_PIX_FMT_YUV422P;
            break;
        case AV_PIX_FMT_YUVJ444P   :
            pixFormat = AV_PIX_FMT_YUV444P;
            break;
        case AV_PIX_FMT_YUVJ440P :
            pixFormat = AV_PIX_FMT_YUV440P;
        default:
            pixFormat = pCodecCtx->pix_fmt;
            break;
    }

    if(pixFormat == AV_PIX_FMT_NONE)
    {
        return -65;
    }
    
    while(av_read_frame(pFormatCtx, &packet) >= 0)
    {
        if(packet.stream_index == videoStream)
        {
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
            struct SwsContext *img_convert_ctx = sws_getContext(pCodecCtx->width,  pCodecCtx->height, pixFormat, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
            if(img_convert_ctx != NULL)
            {
                ret = sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0,  pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
                sws_freeContext(img_convert_ctx);
            }
            break;
        }
        av_packet_unref(&packet);
    }
    
    pFrameYUV->format = AV_PIX_FMT_YUVJ420P;
    pFrameYUV->width  = pCodecCtx->width;
    pFrameYUV->height = pCodecCtx->height;
    
    
    eCodec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if(eCodec == NULL)
    {
        av_frame_free(&pFrame);
        av_free(buffer);
        return -70;
    }
    
    eCodecCtx = avcodec_alloc_context3(eCodec);
    eCodecCtx->bit_rate = 400000;
    eCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
    eCodecCtx->width = pCodecCtx->width;
    eCodecCtx->height = pCodecCtx->height;
    eCodecCtx->time_base.num = 1;
    eCodecCtx->time_base.den = 25;
    
    
    ret = avcodec_open2(eCodecCtx, eCodec, NULL);
    if(ret < 0)
    {
        av_frame_free(&pFrame);
        av_free(buffer);
        return -80;
    }
    
    av_init_packet(&ePacket);
    ePacket.data = NULL;    // packet data will be allocated by the encoder
    ePacket.size = 0;
    
    int got_packet = 0;
    ret = avcodec_encode_video2(eCodecCtx, &ePacket, pFrameYUV, &got_packet);
    if(ret < 0)
    {
        av_frame_free(&pFrame);
        av_free(buffer);
        av_packet_unref(&ePacket);
        return -90;
    }
    
    if (got_packet == 1)
    {
        write(efd, ePacket.data, ePacket.size);
    }
    close(efd);

    avformat_close_input(&pFormatCtx);
    av_packet_unref(&ePacket);
    av_frame_free(&pFrame);
    av_free(buffer);
    return 0;
}



