/**
* Copyright(c) 2020, 合肥祥云汇智科技有限公司
* All rights reserved.
*
* 文件名称：PacketToFile.cpp
* 摘要:
*
* 作者: 梁元琨
* 完成日期: 2020-12-23
**/

#include "PacketToFile.h"
#include "Logger.h"

extern "C" {
#include "libavutil/opt.h"
#include "libavutil/channel_layout.h"
#include "libavutil/common.h"
#include "libavutil/imgutils.h"
#include "libavutil/mathematics.h"
#include "libavutil/samplefmt.h"
#include "libavutil/time.h"
#include "libavutil/fifo.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}

int PacketToFile::openOutput(string outUrl) {

    int ret  = avformat_alloc_output_context2(&mOutputContext, nullptr, nullptr, outUrl.c_str());

    if(ret < 0) {
        LOG_ERROR << "open output context failed!";
        return ret;
    }

    ret = avio_open2(&mOutputContext->pb, outUrl.c_str(), AVIO_FLAG_WRITE,nullptr, nullptr);
    if(ret < 0) {
        LOG_ERROR << "open avio failed";
        return ret;
    }

    // i< nb_streams 替换
    for(int i = 0; i < 1; i++) {
        AVStream* stream = avformat_new_stream(mOutputContext, nullptr);

        //inputContext->streams[i]->codecpar 替换
        AVCodecParameters* codePar = new AVCodecParameters;
        codePar->codec_type = AVMEDIA_TYPE_VIDEO;
        codePar->codec_id = AV_CODEC_ID_MJPEG;
        codePar->codec_tag = 0;
        codePar->bit_rate = 0;
        codePar->format = 13;
        codePar->bits_per_coded_sample = 0;
        codePar->bits_per_raw_sample = 8;
        codePar->profile = 192;
        codePar->level = -99;
        codePar->width = 1280;
        codePar->height = 720;

        avcodec_parameters_copy(stream->codecpar, codePar);
        if(ret < 0) {
            LOG_ERROR << "copy coddec context failed";
            return ret;
        }
    }

    ret = avformat_write_header(mOutputContext, nullptr);
    if(ret < 0) {
        LOG_ERROR << "format write header failed";
        return ret;
    }

    return ret ;
}

void PacketToFile::avPacketRescaleTs(AVPacket* pkt, AVRational srcTb, AVRational dstTb)
{
    if (pkt->pts != AV_NOPTS_VALUE) {
        pkt->pts = av_rescale_q(pkt->pts, srcTb, dstTb);
    }
    if (pkt->dts != AV_NOPTS_VALUE) {
        pkt->dts = av_rescale_q(pkt->dts, srcTb, dstTb);
    }
    if (pkt->duration > 0) {
        pkt->duration = av_rescale_q(pkt->duration, srcTb, dstTb);
    }
}

int PacketToFile::writePacket(shared_ptr<AVPacket> packet) {
    //inputStream->time_base 替换
    AVRational inTimeBase;
    inTimeBase.den = 1000000;
    inTimeBase.num = 1;
    auto outputStream = mOutputContext->streams[packet->stream_index];
    avPacketRescaleTs(packet.get(),inTimeBase,outputStream->time_base);
    return av_interleaved_write_frame(mOutputContext, packet.get());
}

void PacketToFile::closeOutput() {
    if(mOutputContext != nullptr) {
        av_write_trailer(mOutputContext);
        for(int i = 0 ; i < mOutputContext->nb_streams; i++) {
            AVCodecContext* codecContext = avcodec_alloc_context3(NULL);
            avcodec_parameters_to_context(codecContext, mOutputContext->streams[i]->codecpar);
            avcodec_close(codecContext);
        }
        avformat_free_context(mOutputContext);
    }
}