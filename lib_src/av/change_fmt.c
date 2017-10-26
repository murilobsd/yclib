

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "av.h"

int change_fmt(uchar_t* src, uchar_t* dst)
{
    av_register_all();

    int i;
    int ret;
    int videoStream = -1;
    AVFormatContext *pFormatCtx = NULL;
    AVCodecContext *pCodecCtx = NULL;
    AVCodec *pCodec = NULL;
    AVFrame* pFrameYUV = NULL;
    AVPacket packet;
    

    AVFormatContext *oFmtCtx = NULL;
    avformat_alloc_output_context2(&oFmtCtx, NULL, NULL, (char*)dst);
    if(oFmtCtx == NULL)
    {
        return -100;
    }

    ret = avio_open(&oFmtCtx->pb, (char*)dst, AVIO_FLAG_WRITE);
    if(ret < 0)
    {
        return -110;
    }
    
    ret = avformat_open_input(&pFormatCtx, (char*)src, NULL, NULL);
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
        AVStream* ostream = avformat_new_stream(oFmtCtx, pFormatCtx->streams[i]->codec->codec);
        if(ostream == NULL)
        {
            return -120;
        }

        ostream->start_time = 0;

        ret = avcodec_parameters_copy(ostream->codecpar, pFormatCtx->streams[i]->codecpar);
        if(ret < 0)
        {
            return -130;
        }
    }

    ret = avformat_write_header(oFmtCtx, NULL);
    if(ret < 0)
    {
        return -140;
    }
    
    
    int frameFinished;

    i = 0;
    int pts_base = 0;
    int dts_base = 0;
    while(av_read_frame(pFormatCtx, &packet) >= 0)
    {
        AVStream* iStream, *oStream;
        iStream = pFormatCtx->streams[packet.stream_index];
        oStream = oFmtCtx->streams[packet.stream_index];

        

        packet.pts = av_rescale_q_rnd(packet.pts, iStream->time_base, oStream->time_base, (enum AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        packet.dts = av_rescale_q_rnd(packet.dts, iStream->time_base, oStream->time_base, (enum AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        packet.duration = av_rescale_q(packet.duration, iStream->time_base, oStream->time_base);
        packet.pos = -1;


        if(i == 0)
        {
            pts_base = packet.pts;
            dts_base = packet.dts;
            printf("base : %d - %d\n", pts_base, dts_base);
            packet.pts = 0;
            packet.dts = 0;
        }
        ++i;

        printf("%d - %d\n", packet.pts, packet.dts);
        
        ret = av_interleaved_write_frame(oFmtCtx, &packet);
        if(ret < 0)
        {
            return -150;
        }
        
        av_free_packet(&packet);
    }

    av_write_trailer(oFmtCtx);

    return 0;
}



